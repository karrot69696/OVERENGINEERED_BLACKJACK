# Adding a New GameEvent — Step-by-Step

Every time game logic needs to tell the presentation layer "something happened," you add a new event. This is pure plumbing — follow these 5 steps in order.

## 1. GameEvent.h — Define the event

```
src/lowLevelEntities/GameEvent.h
```

1. Add enum value to `GameEventType` (e.g. `MY_NEW_EVENT,`)
2. Add a payload struct (IDs only, never pointers/positions):
   ```cpp
   struct MyNewEventPayload {
       int someId;
       int anotherId;
   };
   ```
3. Add the struct to the `GameEventData` variant (comma after the previous entry)

## 2. NetSerializer.h — Wire up networking

```
src/networking/NetSerializer.h
```

Two places to edit:

**writeGameEvent** — add an `else if constexpr` branch in the `std::visit` lambda:
```cpp
else if constexpr (std::is_same_v<T, MyNewEventPayload>) {
    buf.writeI32(payload.someId);
    buf.writeI32(payload.anotherId);
}
```

**readGameEvent** — add a `case` in the switch:
```cpp
case GameEventType::MY_NEW_EVENT: {
    MyNewEventPayload e;
    e.someId = buf.readI32();
    e.anotherId = buf.readI32();
    data = e;
} break;
```

For variable-length fields (vectors), write the count as `writeU16` first, then each element.

## 3. PresentationLayer.cpp — Handle the event

```
src/gameEngine/PresentationLayer.cpp
```

1. **If it's a cutscene** (should block the event queue until animation finishes): add it to `isCutscene()` switch
2. Add a `case` in the `processEvents()` switch:
   ```cpp
   case GameEventType::MY_NEW_EVENT: {
       auto& e = std::get<MyNewEventPayload>(event.data);
       std::cout << "[PresentationLayer] MY_NEW_EVENT: ..." << std::endl;
       // trigger animation, update UI, etc.
   } break;
   ```

## 4. Phase/Game Logic — Emit the event

Push the event from whichever phase or handler needs it:
```cpp
eventQueue.push({GameEventType::MY_NEW_EVENT, MyNewEventPayload{id1, id2}});
```

Then call `roundManager.updateGameState(...)` so the server broadcasts the new state + event to clients.

## 5. Verify

- [ ] Builds without errors
- [ ] Event fires in LOCAL mode (check console log from PresentationLayer)
- [ ] Event serializes correctly in HOST/CLIENT mode (no desync, no crash)
- [ ] If cutscene: event blocks queue, animation plays, next event waits

## Checklist (copy-paste for PRs)

```
Files touched:
- [ ] src/lowLevelEntities/GameEvent.h        (enum + struct + variant)
- [ ] src/networking/NetSerializer.h           (write + read)
- [ ] src/gameEngine/PresentationLayer.cpp     (isCutscene? + handler)
- [ ] src/phaseEngine/<YourPhase>.cpp          (emit the event)
```
