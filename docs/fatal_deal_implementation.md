# Fatal Deal — Implementation & Debug Log

## What It Does

Fatal Deal is a **reactive** skill. When another player draws a card, the Fatal Deal owner gets a Yes/No prompt. If they accept, they pick one of their own cards to swap with the drawn card. The two cards exchange hands.

- Triggers: `ON_CARD_DRAWN` (any player except self)
- Uses: 3 per game
- Cannot be activated manually (pressing Skill button shows error)
- The trigger prompt also includes extraInfo - containing the rank of the drawn card
---

## Key Implementation Steps

### 1. Reactive Skill System (base infrastructure)

**Files**: `Skill.h`, `Phase.h`, `Phase.cpp`

Added `ReactiveTrigger` enum, `ReactiveContext` struct, and `canReact()` virtual method to the `Skill` base class. Any skill can now declare itself as reactive by overriding `getReactiveTrigger()`.

`ReactiveCheckState` in Phase.h mirrors the existing `NgPendingState` pattern — a per-frame state machine with steps: `PROMPTING → WAITING_RESPONSE → PICKING_CARD → EXECUTING`.

`startReactiveCheck()` is called after every HIT in `turnHandler`. It queries `skillManager.getReactiveSkills()` for qualified skills, builds a queue sorted by turn order, and kicks off `reactiveTickPending()`.

### 2. Fatal Deal Skill Class

**File**: `Skill_FatalDeal.h`

Self-contained header. Key design decisions:
- `canReact()`: rejects own draws (`ctx.drawerId != userId`), checks uses > 0 and cards in hand
- `canActivate()`: always returns error string — prevents manual activation via Skill button
- `execute()`: calls `removeCardFromHand` + `addCardToHand` on both players to swap cards directly (no deck involvement)

### 3. Card Swap Without Deck

**File**: `Player.h`

Added `removeCardFromHand(int cardId)` — removes a card from the hand vector without returning it to the deck. Re-indexes remaining cards after removal to keep `handIndex` sequential.

### 4. Events

**File**: `GameEvent.h`

Two new events:
- `REACTIVE_SKILL_PROMPT` — tells PresentationLayer to show Yes/No prompt
- `FATALDEAL_SWAP` — carries drawer/FD-user IDs and both card IDs for the visual swap

### 5. UI Prompt

**File**: `UIManager.h/cpp`

Reactive prompt overlay: skill name, extra info, countdown timer bar, Yes/No buttons. Same pattern as action menu — buttons hide overlay before firing callback. Auto-declines on timeout.

### 6. Visual Animation

**File**: `AnimationManager.h`, `PresentationLayer.cpp`

`addTeleportSwapAnimation()`: shake in place → shrink to zero → pop in at target position. Uses `origScale` captured at start to preserve the card's actual display scale.

`playFatalDealEffect()`: spritesheet effect (same params as holy effect) played at both card positions before the swap.

### 7. Networking

**Files**: `NetMessage.h`, `NetworkManager.h/cpp`, `NetSerializer.h`

- `SERVER_REACTIVE_PROMPT` / `CLIENT_REACTIVE_RESPONSE` message types
- Server sends prompt to specific client, client sends yes/no back
- Event serialization for `ReactiveSkillPromptEvent` and `FatalDealSwapEvent`

### 8. Server-Authoritative Skill Validation

**Files**: `Phase.cpp` (turnHandler), `Game.cpp`

Server pre-validates remote `SKILL_REQUEST` actions before processing. If invalid (e.g., reactive-only skill), broadcasts `SKILL_ERROR` + `REQUEST_ACTION_INPUT` back to client. If valid, broadcasts `REQUEST_TARGET_INPUT` so client shows targeting overlay. Client just sends the action and waits — no local pre-validation for remote players.

---

## Bugs Encountered & Fixes

### Bug 1: Graphics didn't update after swap
**Symptom**: Logic swapped cards correctly but visuals showed old state.
**Cause**: PresentationLayer's `FATALDEAL_SWAP` handler wasn't implemented yet.
**Fix**: Added handler that updates `CardVisual` ownership/index and triggers animation.

### Bug 2: Cards became giant after swap
**Symptom**: All cards in affected hands rendered at ~2x size after animation.
**Cause**: Cards are created with a scale derived from `UILayout::CARD_SIZE / textureCell` (not 1.0). The animation's `phase2Finish` reset scale to `{1.f, 1.f}`, making them full texture size.
**Fix**: Captured `origScale` at animation start, restored it instead of hardcoded `{1.f, 1.f}`.

### Bug 3: UI disappeared after clicking Skill with 0 uses
**Symptom**: After SKILL_ERROR, action menu vanished — host couldn't interact.
**Cause**: Button `onClick` sets `showActionMenu = false` *before* `onActionChosen` fires. The error path returned early without re-showing the menu.
**Fix**: On HOST, directly call `uiManager.requestActionInput()` after pushing SKILL_ERROR (bypasses event queue delay). On CLIENT, server broadcasts the error events.

### Bug 4: UI disappeared for reactive-only skill on Skill button press
**Symptom**: CLIENT with Fatal Deal clicks Skill → blank screen.
**Cause**: Client called `requestTargetInput()` locally, but Fatal Deal has no targeting overlay registered (it's not Deliverance or NeuralGambit). No overlay shown, action menu already hidden.
**Fix**: Server-authoritative validation. Client just sends action and re-shows action menu while waiting. Server pre-validates in `turnHandler`, rejects with error events or approves with `REQUEST_TARGET_INPUT`.

### Bug 5: Hand card ordering desync between host and client
**Symptom**: After swap, host showed cards in different order than clients (e.g., "3 9" vs "9 3").
**Cause**: `removeCardFromHand` didn't re-index remaining cards' `handIndex` after erasing. `reconcile` (client-side) overwrote `CardVisual.cardIndex` before the `FATALDEAL_SWAP` event was processed, causing index computation to use post-swap values on client but pre-swap values on host.
**Fix**: (a) Added re-indexing in `removeCardFromHand`. (b) Scrapped all index-based position computation — cards now simply swap their screen positions directly via `std::swap` on ownerId/cardIndex, then each card teleports to the other's old position. No `repositionHand` needed.

### Bug 6: Compile error — lambda capture
**Symptom**: `roundManager` referenced in client lambda without capture.
**Cause**: Added `preValidateSkill` call to client `onActionChosen` which only captured `[this]`.
**Fix**: Removed local pre-validation from client entirely (server-authoritative approach). No `roundManager` needed in client lambda.
