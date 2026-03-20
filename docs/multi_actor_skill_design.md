# Multi-Actor Skill Design Reference

How NeuralGambit exposed and shaped the architecture for skills that require input from multiple players across network boundaries. Use this as a blueprint when designing new cross-player interactions.

---

## The Three Layers and Their Boundaries

```
┌─────────────────────────────────────────────────────────────┐
│  LOGIC LAYER (Phases, turnHandler, ngTickPending)           │
│  - Owns game state, runs on HOST/LOCAL only                 │
│  - Decides WHAT input is needed and from WHO                │
│  - Validates before showing UI (preValidateSkill)           │
│  - Pushes events into EventQueue                            │
│  - Never touches sprites, overlays, or network directly     │
├─────────────────────────────────────────────────────────────┤
│  PRESENTATION LAYER (PresentationLayer, AnimationManager)   │
│  - Reads events from EventQueue                             │
│  - Filters: skips REQUEST_ACTION_INPUT for bots/remote      │
│    players (checks PlayerInfo.isBot / isRemote)             │
│  - Calls UIManager to show overlays                         │
│  - Blocks on cutscene events (PHASE_ANNOUNCED, CARD_DRAWN)  │
├─────────────────────────────────────────────────────────────┤
│  UI LAYER (UIManager)                                       │
│  - Dumb renderer + input collector                          │
│  - Shows whatever it's told, never pushes events            │
│  - Fires callbacks (onActionChosen, onTargetChosen)         │
│  - Identical behavior for HOST, CLIENT, and LOCAL           │
│  - Never knows about networking or game rules               │
└─────────────────────────────────────────────────────────────┘
```

**Rule: information flows downward (logic → presentation → UI). User input flows upward via callbacks (UI → Game.cpp wiring → game state). Events are the ONLY bridge between logic and presentation.**

---

## Callback Wiring (Game.cpp)

Callbacks are wired ONCE in `Game::RunGame()`, with two branches:

### HOST / LOCAL
```
onActionChosen:
  1. Find active player via uiManager.getActivePlayerId()
  2. If SKILL_REQUEST → preValidateSkill() FIRST
     - Fail: push SKILL_ERROR event, menu stays visible
     - Pass: setPendingAction, call requestTargetInput()
  3. Otherwise: setPendingAction (menu already hidden by button)

onTargetChosen:
  1. If targeting has data → gameState.pendingTarget = targeting (no player lookup needed)
  2. If empty (cancel) → find active player, re-show action menu, reset to IDLE
```

### CLIENT
```
onActionChosen:
  1. sendAction() over network
  2. If SKILL_REQUEST → show targeting overlay locally (server validates later)

onTargetChosen:
  1. If targeting has data → sendTarget() over network
  2. If empty (cancel) → re-show action menu locally
```

**Critical lesson:** The data path (`gameState.pendingTarget = targeting`) must NOT depend on `activePlayerId`. The `clearInput()` from cutscene events resets `activePlayerId` to -1, and multi-actor flows like ngTickPending call `requestPickCard` long after the original action menu was dismissed. Only the cancel path needs `activePlayerId`.

---

## How NeuralGambit Works End-to-End

### Step 1: Player clicks Skill → targets two players

```
CLIENT                          SERVER
Button click
  → onActionChosen(SKILL)
  → sendAction(SKILL_REQUEST)   → turnHandler receives action
  → show targeting overlay
Player picks 2 targets
  → onTargetChosen(2 playerIds)
  → sendTarget(targeting)       → turnHandler receives targeting
                                → skillHandler: sees NG + playerIds, no cards
                                → creates ngPending{skillUserId, target1, target2}
                                → sets player to IDLE, clears pendingTarget
                                → returns (no skill executed yet)
```

### Step 2: ngTickPending orchestrates card picks

`turnHandler` detects `ngPending.has_value()` and calls `ngTickPending(skillUser)` every frame. The orchestrator runs one step per frame:

```
Step                    Local player        Bot              Remote player
─────────────────────── ─────────────────── ──────────────── ────────────────────
First target picks      requestPickCard()   autoPick()       sendTargetRequest()
                        → user clicks card  → instant        → client shows overlay
                        → confirmTargeting  → ng.firstCardId → client sends target
                        → pendingTarget set                  → server consumes

Second target picks     (same as above)     (same)           (same)

Skill user picks boost  requestBoostPickInput()              sendTargetRequest(isBoostPick=true)
                        → user clicks card                   → client shows boost overlay
                        → confirmTargeting                   → client sends target
                        → pendingTarget set                  → server consumes
```

### Step 3: Execution

Once all three picks are collected, ngTickPending builds a `SkillContext` with the three `Card*` pointers and calls `skillManager.processSkill()` → `execute()` → boost is applied → events pushed → `ngPending` cleared.

---

## Key Bugs Found and Architectural Fixes

### 1. `activePlayerId` goes stale during multi-actor flows

**Problem:** `clearInput()` (from cutscene events) resets `activePlayerId = -1`. When `requestPickCard` fires later and the user picks a card, the `onTargetChosen` callback couldn't find the player → silently returned → `gameState.pendingTarget` never set → deadlock.

**Fix:** The data path in `onTargetChosen` sets `gameState.pendingTarget` FIRST, without any player lookup. Only the cancel path needs `activePlayerId`.

**Rule: Callbacks that write to gameState.pendingTarget must not depend on transient UI state.**

### 2. Action menu persisted across turn boundaries

**Problem:** After sending an action (Stand), the client's menu stayed visible during the network round-trip. Player could spam buttons, sending stale actions.

**Fix (UI):** Buttons set `showActionMenu = false` immediately on click, before firing the callback.

**Fix (Server):** `RoundManager::changePhase()` calls `networkManager->clearAllRemoteInputs()` to flush stale actions/targets on every phase transition.

**Rule: UI must go silent immediately after capturing input. Server must flush on turn boundaries.**

### 3. Phase reuse skipped UI prompts for next player

**Problem:** After P1 stood, `PlayerHitPhase::onUpdate` incremented to P2 but returned `std::nullopt` — reusing the same phase without running `onEnter`. P2 never got `REQUEST_ACTION_INPUT`.

**Fix:** Return `PhaseName::PLAYER_HIT_PHASE` instead of `std::nullopt` when more players remain. This triggers `changePhase` → `onExit` (clear UI) → new phase → `onEnter` (push events for next player).

**Rule: When the active player changes, always re-enter the phase so onEnter pushes fresh UI prompts.**

### 4. HOST could see remote player's action menu

**Problem:** `REQUEST_ACTION_INPUT` events are pushed for all players (needed for broadcast). PresentationLayer on HOST was calling `uiManager.requestActionInput()` for remote players too.

**Fix:** PresentationLayer checks `PlayerInfo.isBot` and `PlayerInfo.isRemote` before calling UIManager. UIManager stays dumb — no access control.

**Rule: PresentationLayer is the gatekeeper for what reaches UIManager. UIManager never filters.**

---

## Checklist for New Multi-Actor Skills

1. **Skill.h subclass:** `canUse()` returns string (empty = OK), `execute()` modifies state + pushes result events. Pure logic, no UI, no network.

2. **SkillManager:** Register in `createSkills()`. Add `preValidateSkill()` check if the skill has pre-targeting requirements.

3. **skillHandler (Phase.cpp):** Detect the skill's initial targeting data. If multi-actor, create a `PendingState` struct, clear `pendingTarget`, set player to IDLE, and return early.

4. **tickPending (Phase.cpp):** Write a `skillNameTickPending()` function. Follow the pattern:
   - One step per frame (return after each request)
   - For each actor: check isBot (auto-pick), isRemote (sendTargetRequest), or local (requestPickCard)
   - Poll results: `gameState.pendingTarget` for local, `net->hasRemoteTarget()` for remote
   - After all inputs collected: build SkillContext, call processSkill, push events, clear pending

5. **turnHandler (Phase.cpp):** At the top, check `if (pendingState.has_value()) { tickPending(player); return true; }`

6. **skillProcessAftermath (Phase.cpp):** Add a case for the new skill's presentation events.

7. **UIManager:** Add any new overlays needed. Overlays just collect raw input and fire `confirmTargeting()`. No game logic.

8. **Events (GameEvent.h):** Add result event types for presentation. Payloads carry IDs only — never pointers.

---

## gameState.pendingTarget: The Shared Mailbox

`gameState.pendingTarget` is the universal channel between UI callbacks and game logic:

- **Writer:** `onTargetChosen` callback (for local players), `net->consumeRemoteTarget()` (for remote players), `autoPick` lambdas (for bots)
- **Reader:** `turnHandler` (for simple skills), `ngTickPending` (for multi-actor skills)
- **Cleared by:** the reader, immediately after consuming

It is NOT typed to a specific skill — any skill can use it. The orchestrator (`tickPending`) knows what it asked for and interprets the data accordingly.
