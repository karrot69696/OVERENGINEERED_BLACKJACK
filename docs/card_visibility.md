# Card Visibility System

## Architecture

Three systems touch card face state, each with a strict lane:

| System | Owns | Does NOT touch |
|--------|------|----------------|
| `enforceVisibility()` | Local player's cards (always face-up), cheat mode, deck cards (face-down) | Other players' face state — event-driven only |
| `reconcile()` | ownerId, cardIndex, location, rankBonus | faceUp — never touches face state |
| Events (`CARDS_REVEALED`) | Face state for non-local cards via flip animations | — |

- **`Card.faceUp`** (game logic) — publicly revealed to ALL players. Set by `flipAllCardsFaceUp()` or `card->flip()`.
- **`CardVisual.faceUp`** (visual) — what's actually rendered. For local player: driven by `enforceVisibility()`. For other players: driven by events only.

`enforceVisibility()` runs every frame after animations, before render.

### Iron Rules (enforceVisibility)

| Condition | Result |
|-----------|--------|
| `cv.isPinned()` | Skipped (animation in flight) |
| `cv.ownerId == localPlayerId` | Face-up (player sees own cards) |
| `cheatOn` | Face-up |
| Card in deck (not in any hand) | Face-down |
| Otherwise (other players' cards) | **Not touched** — face state is event-driven |

It also verifies the texture rect matches the expected rank/suit for local/cheat cards, correcting stale textures.

### Why enforceVisibility doesn't touch other players' cards

The server sends state (final result) and events (animation triggers) together. If `enforceVisibility` followed `logicFaceUp` for all cards, it would race with flip animations — it sees the server's final face-up state and applies it before `CARDS_REVEALED` gets to play the animation. By limiting `enforceVisibility` to local player cards only, events are the sole driver of non-local face state. No races.

## Pin System

`CardVisual.pinCount` (not a boolean) — multiple systems can pin independently.

- `pin()` increments, `unpin()` decrements (floor at 0)
- Card is pinned when `pinCount > 0`
- Pinned cards are skipped by both `reconcile()` and `enforceVisibility()`
- Use `pinCount = 0` to force-clear all pins (e.g., Fatal Deal card swap)

### Who pins what

| System | Pins | Unpins |
|--------|------|--------|
| `clientReceive()` pre-pin | `pin()` on arrival for `CARDS_REVEALED` cards | — (PresentationLayer unpins) |
| `CARDS_REVEALED` handler | `pin()` only if not already pinned (`isPinned()` check) | `unpin()` in `playCardFlip` onFinish |
| `NEURALGAMBIT_REVEAL` callback | — | `unpin()` when NG effect finishes |
| `FATALDEAL_SWAP` handler | — | `pinCount = 0` (force-clear, card changing hands) |

### CLIENT pre-pin (the critical fix)

On HOST, `roundManager.update()` is gated on `!animationManager.playing()`, so events trickle in one cutscene at a time — PresentationLayer always processes `CARDS_REVEALED` before `enforceVisibility` runs.

On CLIENT, events arrive in batches from the network. A `CARDS_REVEALED` event can be stuck behind a prior cutscene in the queue, unprocessed. Meanwhile `enforceVisibility` runs every frame. Without a pin, it would see `logicFaceUp = true` and flip the card face-up, killing the animation.

Fix: `clientReceive()` scans incoming events and pre-pins `CARDS_REVEALED` cards immediately on arrival, before they enter the queue. PresentationLayer's handler skips the pin if already pinned, so pinCount stays balanced:

```
CLIENT: pre-pin in clientReceive (0->1) -> PL skips pin -> animation plays -> onFinish unpin (1->0)
HOST:   no pre-pin -> PL pins (0->1) -> animation plays -> onFinish unpin (1->0)
```

## reconcile and faceUp

`reconcile()` does NOT touch `faceUp` or face textures. It only syncs: ownerId, cardIndex, location, rankBonus.

This prevents reconcile from flipping cards face-up before `CARDS_REVEALED` animations play on the client.

## Critical Rule: updateGameState After Logic Flips

When `Card.faceUp` changes on HOST, `roundManager.updateGameState()` **must** be called before the next `return` or event push. Without it, `serverBroadcast` sends stale `faceUp` to clients.

Pattern:
```cpp
card->flip();                          // mutate
roundManager.updateGameState(...);     // sync to gameState
eventQueue.push({...});                // push events
```

Places that do this:
- `ngTickPending` — flip up on reveal, conditionally flip down on complete
- `BattlePhase::onUpdate` — `flipAllCardsFaceUp` both players
- `HostHitPhase::onUpdate` — `flipAllCardsFaceUp` opponent
- `BlackJackCheckPhase::onUpdate` — `flipAllCardsFaceUp` on blackjack

## Temporary vs Permanent Reveals

**Permanent** (battle, host-hit, blackjack-check): `flipAllCardsFaceUp()` sets `Card.faceUp = true`. After `CARDS_REVEALED` unpin, `enforceVisibility` doesn't touch them (non-local), and logic keeps them face-up, so they stay visible.

**Temporary** (Neural Gambit reveal): cards are flipped face-up for the reveal, then flipped back face-down when done. The cleanup only flips back cards that were **originally face-down** (`ng.firstWasFaceDown` / `ng.secondWasFaceDown` in `NgPendingState`). During battle phase, cards are already face-up, so the cleanup is skipped — no spurious face-down followed by a flip-up glitch.

## Borrowed PlayerVisuals

`PlayerVisual` lives in `VisualState.h`. UIManager owns the vector but AnimationManager can borrow a visual for effects (e.g., shake during explosion). While borrowed:

- UIManager skips `setPosition()` / `setScale()` / `draw()` for that visual
- AnimationManager has full control and renders it via `renderBorrowedPlayerVisual()`
- `borrowedPlayerVisualIds` tracks which player IDs are borrowed
- On animation finish, the borrowed ID is cleared and UIManager resumes control

## What NOT To Do

- **Don't use `logicFaceUp` in `enforceVisibility` for non-local cards.** Events own that. Using logic causes races with flip animations.
- **Don't touch faceUp in `reconcile()`.** `enforceVisibility` and events own face state.
- **Don't flip cards from UIManager.** UIManager should never call `flipCardVisualFaceUp/Down`.
- **Don't set `cv.faceUp` directly from PresentationLayer.** Let `enforceVisibility()` or `playCardFlip` handle it.
- **Don't unconditionally flip NG cards back face-down.** Check `firstWasFaceDown`/`secondWasFaceDown` first — during battle phase they should stay up.
