# Card Visibility System

## Architecture

Two layers decide what players see:

- **`Card.faceUp`** (game logic) — publicly revealed to ALL players. Set by `flipAllCardsFaceUp()` or `card->flip()`.
- **`CardVisual.faceUp`** (visual) — what's actually rendered. Controlled by `enforceVisibility()`.

`enforceVisibility()` runs every frame after animations, before render. It is the final authority.

### Iron Rules (enforceVisibility)

| Condition | Result |
|-----------|--------|
| `Card.faceUp == true` in game logic | Face-up for everyone |
| `cv.ownerId == localPlayerId` | Face-up (player sees own cards) |
| `cheatOn` | Face-up |
| `cv.isPinned()` | Skipped (temporary override) |
| Otherwise | Face-down |

It also verifies the texture rect matches the expected rank/suit every frame, correcting stale textures from bad `cardIndex` lookups.

## Pin System

`CardVisual.pinCount` (not a boolean) — multiple systems can pin independently.

- `pin()` increments, `unpin()` decrements
- Card is pinned when `pinCount > 0`
- Pinned cards are skipped by both `reconcile()` and `enforceVisibility()`
- Use `pinCount = 0` to force-clear all pins (e.g., Fatal Deal card swap)

### Who pins what

| System | Pins | Unpins |
|--------|------|--------|
| `CARDS_REVEALED` handler | `pin()` before flip animation | `unpin()` in `playCardFlip` onFinish |
| `requestBoostPickInput` | `pin()` to keep cards visible during pick | `unpin()` on confirm or `clearInput()` |
| `NEURALGAMBIT_REVEAL` callback | — | `unpin()` when NG effect finishes |
| `FATALDEAL_SWAP` handler | — | `pinCount = 0` (force-clear, card changing hands) |

## Critical Rule: updateGameState After Logic Flips

When `Card.faceUp` changes on HOST, `roundManager.updateGameState()` **must** be called before the next `return` or event push. Without it, `serverBroadcast` sends stale `faceUp` to clients and `enforceVisibility` on the client forces cards back down.

Pattern:
```cpp
card->flip();                          // mutate
roundManager.updateGameState(...);     // sync to gameState
eventQueue.push({...});                // push events
```

Places that do this:
- `ngTickPending` — flip up on reveal, flip down on complete
- `BattlePhase::onUpdate` — `flipAllCardsFaceUp` both players
- `HostHitPhase::onUpdate` — `flipAllCardsFaceUp` opponent
- `BlackJackCheckPhase::onUpdate` — `flipAllCardsFaceUp` on blackjack

## Temporary vs Permanent Reveals

**Permanent** (battle, host-hit, blackjack-check): `flipAllCardsFaceUp()` sets `Card.faceUp = true`. After CARDS_REVEALED unpin, `enforceVisibility` sees `logicFaceUp = true` → stays up.

**Temporary** (Neural Gambit, quintet mid-round): `Card.faceUp` is flipped true for the reveal, then flipped back false when done. Pin keeps cards visible during the interim. After unpin + flip-back, `enforceVisibility` forces them down.

## What NOT To Do

- **Don't flip cards from UIManager.** UIManager should never call `flipCardVisualFaceUp/Down`. Use `pin()` + one-time flip in a request method instead.
- **Don't rely on reconcile for HOST.** Reconcile only runs on CLIENT. `enforceVisibility()` covers all modes.
- **Don't set `cv.faceUp` directly from PresentationLayer.** Let `enforceVisibility()` handle face state. PresentationLayer should only set metadata (ownerId, cardIndex) and start animations.
