# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

```bash
# Configure + build (MinGW, debug)
cmake --preset debug
cmake --build --preset debug -j

# Configure + build (MinGW, release)
cmake --preset release
cmake --build --preset release -j

# MSVC variants
cmake --preset windows-msvc-debug
cmake --build --preset windows-msvc-debug
```

Output: `build/<preset>/bin/CrazyJack.exe`. Assets are copied to `build/<preset>/assets/` automatically.

**Don't build to verify after small changes.** Only build when explicitly asked or after a large batch of changes is complete.

**SFML 3.0.2** is the only external dependency, path hardcoded in CMakeLists.txt. No vcpkg/conan. No test framework.

## Architecture

**Overengineered Blackjack** — a multiplayer blackjack game (1 human + up to 6 bots) with a phase-based state machine, skill system, and sprite animation framework. C++17, SFML 3.

### Module hierarchy (each is a static library):

```
Game (entry point, game loop)
├── RoundManager         — owns phases, manages transitions and turn order
│   ├── Phase subclasses — state machine (onEnter/onUpdate/onExit lifecycle)
│   └── SkillManager     — creates and executes skills via SkillContext
├── UIManager            — renders UI, captures player input (menus, buttons)
├── AnimationManager     — functional animation system (lambda-based tweening)
└── VisualState / GameState — visual layout vs pure game logic (separated)
```

### Phase flow (state machine)

```
GAME_START → BLACKJACK_CHECK → PLAYER_HIT (per player) → HOST_HIT → BATTLE → ROUND_END → repeat
```

Each phase is a subclass of `Phase`. RoundManager calls `createPhase()` to transition. Phases access players, deck, animation, UI, and skills through base class references.

### Animation system (`AnimationManager.h`)

Animations are lambda functions driven by normalized progress (0.0–1.0):

```cpp
struct Animation {
    std::function<void(float)> update;  // called each frame with progress
    std::function<void()> onFinish;     // called once when complete
    float time, duration;
};
```

`playSpriteAnimation()` is the reusable helper for spritesheet frame animations. Sprite-specific methods (explosion, shock, holy) set up the sprite then delegate to it.

### Skill system (`skillEngine/`)

Skills inherit from `Skill` base class. Each skill is fully self-contained in its own `Skill_Name.h`.

- `canUse(const SkillContext&)` returns `std::string` — empty = allowed, non-empty = error message shown to player
- `execute(SkillContext&)` — pure logic, modifies game state only, never pushes events
- Skills stored as `std::vector<std::unique_ptr<Skill>>` in `SkillManager`

**Execution flow** (`Phase::skillHandler` → `Phase.cpp`):
1. `skillManager.processSkill(context)` → returns `SkillExecutionResult { SkillName name, string errorMsg }`
2. On failure: push `SKILL_ERROR` event with the error message
3. On success: call `skillProcessAftermath(context, result)` — a free function in `Phase.cpp` that emits skill-specific presentation events (one `case` per skill name)

`skillProcessAftermath` is defined after `skillHandler` in Phase.cpp — it needs a forward declaration at the top of the file if the order changes.

### Event system (`GameEvent.h`, `EventQueue.h`)

Events are the only communication channel between game logic and presentation. **Payloads carry IDs only — never pointers, screen positions, or `Card*`/`Player*`.** PresentationLayer resolves IDs to local objects at render time. This is what makes client-side replaying of server events safe.

`SKILL_ERROR` is pushed by `skillHandler` (not by `processSkill` or `canUse`). `REQUEST_ACTION_INPUT` is re-pushed after every HIT or skill use to re-show the action menu.

### UIManager NeuralGambit targeting (`NgStep`)

NeuralGambit uses a 4-step UI state machine stored in `UIManager`:

```
PICK_PLAYER → FIRST_PLAYER_PICK → SECOND_PLAYER_PICK → PICK_BOOST_CARD → confirmTargeting()
```

- `ngTargetPlayerIds`: sorted vector of 2 chosen player IDs
- `ngTargetCardIds`: vector of 2 chosen card IDs (index 0 = first player's, index 1 = second player's)
- Bots auto-pick randomly at their step; remote human targets are a known gap (currently no net message to request a remote pick)
- `confirmTargeting()` packages everything into `PlayerTargeting` and fires `onTargetChosen`

### Key constants

- `GameConfig::BLACKJACK_VALUE = 21`, `DEALER_STAND_VALUE = 17` (in `GameState.h`)
- `UILayout::CARD_SIZE`, `CARD_SPACING` (in `GameState.h`)
- `AnimConfig::PHASE_TEXT_DURATION`, `CARD_DRAW_DURATION` (in `AnimationManager.h`)

### Networking (`networking/`, ENet-based)

Three modes: `LOCAL` (no networking), `HOST` (server + local player), `CLIENT` (remote player).

**Server-authoritative model**: HOST runs all game logic (RoundManager/phases). CLIENT receives state + events, never runs phases.

#### Message types (`NetMessage.h`)
- Client → Server: `CLIENT_JOIN`, `CLIENT_ACTION`, `CLIENT_TARGET`, `CLIENT_DISCONNECT`
- Server → Client: `SERVER_WELCOME`, `SERVER_GAMESTATE`, `SERVER_EVENT_BATCH`, `SERVER_GAME_START`, etc.

#### Game loop order (per frame)
```
1. window.pollEvent()        — SFML events
2. networkManager.poll()     — ENet receive
3. clientReceive()           — CLIENT only: apply GameState, sync Card*/Player, reconcile visuals, push received events into local EventQueue
4. roundManager.update()     — HOST/LOCAL only, gated on !animationManager.playing()
5. serverBroadcast()         — HOST only, called right after roundManager.update() with snapshot index
6. presentationLayer.processEvents() — pops events from EventQueue, triggers animations (blocks on cutscene events)
7. animationManager.update() — tick animations
8. render
```

#### Event broadcast (the critical path)
```
Before roundManager.update(): snapshot = eventQueue.size()
After  roundManager.update(): broadcast only events[snapshot..size) to clients
```
Events stay in the queue for the local PresentationLayer — **no drain/re-push**. This prevents duplicate events when PresentationLayer is blocked on a cutscene animation.

#### Client state sync (`Game::clientReceive`)
1. Apply `GameState` from server (phase, players, deck, cards)
2. `syncLocalFromGameState()` — match local `Card*` and `Player` objects to server state by card ID
3. First frame: `visualState.rebuildFromState()`. Every subsequent frame: `visualState.reconcile()` (metadata-only update, safe during animations)
4. Push received events into local EventQueue for PresentationLayer

#### Input flow (remote players)
- CLIENT: `UIManager` callbacks → `networkManager.sendAction()`/`sendTarget()` → server
- HOST: `Phase::turnHandler()` checks `networkManager.hasRemoteAction(playerId)` for non-local players

#### Lobby flow
1. Host starts server, shows lobby screen with live player count
2. Client connects, receives `SERVER_WELCOME` with assigned player ID
3. Host clicks "Start Game" → `broadcastGameStart()` sends `SERVER_GAME_START`
4. Client receives signal → exits waiting screen → enters game loop

#### Key debugging patterns
- **Event duplication**: check if events are being broadcast more than once per logic tick
- **Desync**: compare `[Server]` and `[Client]` console logs for phase/turn/card count
- **Animation stalls**: PresentationLayer blocks on cutscene events — if an event is pushed but animation never starts, the queue stalls
- **reconcile vs rebuildFromState**: reconcile only updates metadata (ownerId, cardIndex, faceUp texture), never sprite positions. rebuildFromState is destructive — only used once on initial connect

## Adding new features

| Feature | Where to add |
|---------|-------------|
| New phase | `phaseEngine/` + register in `RoundManager::createPhase()` |
| New skill | Create `skillEngine/Skill_Name.h` (canUse returns string, execute is pure logic) + `case` in `SkillManager::createSkills()` + `case` in `skillProcessAftermath()` in Phase.cpp |
| New animation effect | Load texture in `AnimationManager` constructor, use `playSpriteAnimation()` |
| UI changes | `UIManager.h/cpp` + `VisualState.h` for layout |
| Game rules | `Player.h/cpp` or relevant phase in `phaseEngine/` |
