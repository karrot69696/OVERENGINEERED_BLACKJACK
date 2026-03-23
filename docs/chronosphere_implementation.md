# Chronosphere Implementation

## Overview

Chronosphere is the first **fusion skill** — both actively triggered (click Skill button) and reactively triggered (on hit phase start, on hand value change). It lets players snapshot their hand ranks (free) and later rewind to that snapshot (costs 1 use, 3 uses per game).

## Skill Behavior

- **Snapshot**: Copy current hand card ranks to `gameState.ranksSnapShot`. FREE (no use cost). Can overwrite a previous snapshot.
- **Rewind**: Return all current hand cards to deck, then pull cards matching snapshot ranks from deck (regardless of suit/order). Costs 1 use.

## Reactive Triggers

1. **ON_HIT_PHASE_START** — fires at the start of every non-host player's hit phase via the existing reactive system (`startReactiveCheck`). The reactive queue processes Fatal Deal first (ON_CARD_DRAWN), then Chronosphere sees the final hand state. Zero conflicts.
2. **Hand value change detection** — `tryChronoReactiveCheck()` is a universal guard called before every `REQUEST_ACTION_INPUT` push. It compares the Chronosphere owner's current hand value against `gameState.chronoTrackedHandValue`. Catches ALL sources: HIT, Fatal Deal swap, NeuralGambit boost, any future skill.

## Active Trigger

Click Skill button during Chronosphere owner's own turn. `onActionChosen` skips the targeting overlay — `turnHandler` SKILL_REQUEST case detects Chronosphere with empty pendingTarget and starts `chronoPending`.

## Key Architecture

### ChronoPendingState + chronoTickPending()

Follows the `NgPendingState` pattern but simpler (single player, no card picking). One step per frame:

1. **Send prompt**: bot auto-picks, remote via `sendChronoPrompt`, local via `requestChronoPrompt` UI
2. **Poll response**: check `gameState.pendingChronoChoice` (local/bot) or `consumeChronoResponse` (remote)
3. **Execute**: route to `executeSnapshot()` or `executeRewind()` via `processSkill`, then `skillProcessAftermath`

Two IDs tracked:
- `skillUserId` — the Chronosphere owner (who gets prompted)
- `actingPlayerId` — who gets `REQUEST_ACTION_INPUT` after resolution (differs in HOST_HIT_PHASE where host is decision-maker but chrono owner may be an opponent)

### tryChronoReactiveCheck(int playerId)

- Scans ALL players for a Chronosphere owner with changed hand value (not just the passed-in player)
- `playerId` param = the acting player who gets re-prompted if chrono doesn't fire
- `chronoTickPending` pushes `REQUEST_ACTION_INPUT` directly (no re-check) to avoid infinite loops
- `chronoTrackedHandValue` is initialized to the Chronosphere OWNER's hand value at phase start, not the acting player's

### Reactive Priority Chain

When a card is drawn:
```
HIT → startReactiveCheck(ON_CARD_DRAWN) → Fatal Deal procs first → reactiveTickPending completes
    → tryChronoReactiveCheck detects net hand value change → chronoPending
```

Fatal Deal always gets priority because it uses the reactive trigger system. Chronosphere sees the final hand state after all reactive skills resolve. This means no wasted rewinds on cards about to be swapped.

### UI: Chrono Choice Prompt

`requestChronoPrompt(bool hasSnapshot, float timer)` shows a modal with [Snapshot] and [Rewind] buttons. Rewind is grayed out if no snapshot exists. Blue color theme. Timer bar counts down.

### Event Flow (Rewind)

`skillProcessAftermath` pushes:
1. `CHRONOSPHERE_REWIND` — plays effect animation + floating text
2. `CARD_RETURNED` per old card — animates cards back to deck
3. `CARD_DRAWN` per new card — animates new matching cards into hand

### Round Reset

`RoundEndPhase::onEnter()` clears: `snapShotTaken`, `ranksSnapShot`, `pendingChronoChoice`, `chronoTrackedHandValue`.

## Files Modified

| File | Changes |
|------|---------|
| `GameState.h` | `ChronoChoice` enum, `pendingChronoChoice`, `chronoTrackedHandValue` |
| `Skill.h` | `FUSION` skill type, `returnedCardIds`/`drawnCardIds` in SkillContext |
| `Skill_ChronoSphere.h` | Full skill implementation |
| `SkillManager.h/.cpp` | Registration |
| `Phase.h` | `ChronoPendingState`, `chronoTickPending`, `tryChronoReactiveCheck` |
| `Phase.cpp` | turnHandler integration, chronoTickPending, tryChronoReactiveCheck, skillProcessAftermath |
| `PlayerHitPhase.cpp` | Chrono baseline init, ON_HIT_PHASE_START reactive check |
| `HostHitPhase.cpp` | Same as PlayerHitPhase |
| `UIManager.h/.cpp` | Chrono prompt UI |
| `Game.cpp` | Callback wiring, skip targeting for Chronosphere |
| `GameEvent.h` | CHRONOSPHERE_SNAPSHOT/REWIND events |
| `PresentationLayer.cpp` | Event handlers |
| `AnimationManager.h` | playChronosphereEffect (reuses holy spritesheet, tinted) |
| `RoundEndPhase.cpp` | Reset chrono state |
| `NetMessage.h` | SERVER_CHRONO_PROMPT, CLIENT_CHRONO_RESPONSE |
| `NetSerializer.h` | Chrono prompt/response + event serialization, snapShotTaken in GameState |
| `NetworkManager.h/.cpp` | sendChronoPrompt, chrono response handling |
| `CMakeLists.txt` | Added Skill_ChronoSphere.h |

## Validation

The implementation proved the existing reactive system was fully functional — Chronosphere (fusion) layered on top of Fatal Deal (reactive) with zero conflicts. The priority chain (Fatal Deal → Chronosphere) emerged naturally from the architecture.
