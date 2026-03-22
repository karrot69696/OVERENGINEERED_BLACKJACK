# Auto-Pick & Input Timer System

How the input timer and auto-pick system works, and why the existing architecture made it nearly free to implement.

---

## What It Does

Every input prompt (action menu, targeting overlays, card pick overlays) has a countdown timer bar. When the timer expires, the system auto-picks a default action:

| Overlay | Auto-timeout action |
|---------|-------------------|
| Action menu (Hit/Stand/Skill) | Stand |
| Deliverance targeting | Random card from hand |
| NeuralGambit PICK_PLAYER | 2 random players |
| NeuralGambit PICK_BOOST | Random of the 2 revealed cards |
| NeuralGambit WAITING_FOR_PICKS | No timeout (server-driven) |
| Pick card overlay | Random allowed card |

Timer durations are configured in `GameConfig.h`:
- `ACTION_PROMPT_DURATION = 30` (action menu)
- `TARGET_PROMPT_DURATION = 20` (all targeting)
- `REACTIVE_PROMPT_DURATION_DEFAULT = 12` (reactive prompts like Fatal Deal)

---

## Why It Works Without Validation

The auto-pick fires the **exact same callbacks** as a real player click:
- Action menu timeout: `onActionChosen(PlayerAction::STAND)`
- Targeting timeout: builds a `PlayerTargeting` struct and calls `confirmTargeting()` -> `onTargetChosen(targeting)`

From the game logic's perspective, there is **zero difference** between a player clicking "Stand" and the timer auto-standing. The data flows through the same callback wiring in `Game.cpp`, through the same `gameState.pendingTarget` mailbox, into the same `turnHandler` / `skillHandler` / `ngTickPending` logic.

No special "auto-pick" code path exists in the game logic layer. No `if (wasAutomatic)` checks anywhere.

### Why random picks are always valid

By the time any overlay is shown, the **allowed targets are already constrained**:

- **Deliverance**: overlay only highlights cards where `cv.ownerId == activePlayerId` — picking any of those is valid
- **Pick card**: overlay only allows `pickCardAllowedIds` — picking any of those is valid
- **NG boost pick**: only the 2 revealed card IDs are clickable — picking either is valid
- **NG player pick**: all players are valid targets (the skill itself handles edge cases)

The constraint is enforced at the UI layer, so random selection within those constraints is guaranteed correct.

---

## Architecture That Made This Free

This system was trivial to implement because of three design rules established earlier:

### 1. UIManager is a dumb renderer + input collector

UIManager never pushes events, never validates game rules, never knows about networking. It shows whatever it's told and fires callbacks. This means auto-pick just needs to fire the same callback — no game logic to duplicate.

### 2. Information flows down, input flows up via callbacks

```
Logic Layer (Phases, turnHandler)
    |  events (what input is needed)
    v
Presentation Layer (filters for local humans)
    |  calls UIManager methods
    v
UI Layer (shows overlay, fires callback on click OR timeout)
    |  callback
    v
Game.cpp wiring (sets gameState.pendingTarget or setPendingAction)
    |
    v
Logic Layer (consumes input, same path for all sources)
```

Auto-pick inserts at the UI layer — the same point where player clicks insert. Everything above and below is unchanged.

### 3. Allowed targets are pre-constrained

The logic layer decides WHAT to show (which cards, which players). The UI layer just renders and collects. So "pick random from what's shown" is always a valid choice — the filtering already happened upstream.

---

## Implementation Details

### Single shared timer

Only one input overlay is active at a time (they clear each other), so a single `sf::Clock inputTimerClock` + `float inputTimerDuration` pair works for all overlays. The clock restarts whenever any input request method is called:

- `requestActionInput(playerId, duration)`
- `requestTargetInput(playerId, duration)`
- `requestPickCard(allowedCardIds)` (uses `TARGET_PROMPT_DURATION`)
- `requestBoostPickInput(card1Id, card2Id)` (uses `TARGET_PROMPT_DURATION`)

### Timer bar rendering

`renderInputTimerBar()` is a shared helper called at the end of each overlay's render method. Draws a 300px bar just above the bottom strip. Green when >30% remaining, red when <=30%.

### Duration propagation

Duration travels through the event system:
1. Phase pushes `RequestActionInputEvent{playerId}` (duration has a default from `GameConfig`)
2. PresentationLayer passes `e.duration` to `uiManager.requestActionInput(id, duration)`
3. UIManager starts the clock with that duration

For sub-step overlays (`requestPickCard`, `requestBoostPickInput`), Phase calls UIManager directly — no event needed, timer restarts internally.

### Networking

`RequestActionInputEvent` and `RequestTargetInputEvent` serialize the `duration` field, so clients receive the same timer values as the host.

---

## Future: Smart AI Takeover

The auto-pick currently uses random selection (stand for actions, random for targets). This is the foundation for a "smart AI takeover" feature — when a player goes AFK, instead of random picks, an actual bot AI could take over and play optimally. The callback interface stays identical; only the selection logic inside the timeout handler changes.
