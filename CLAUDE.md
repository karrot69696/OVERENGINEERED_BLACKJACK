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

Skills inherit from `Skill` base class, implement `canUse()` and `execute()` with a `SkillContext` (user, targets, deck, state). New skills: create `Skill_Name.h`, register in `SkillManager`.

### Key constants

- `GameConfig::BLACKJACK_VALUE = 21`, `DEALER_STAND_VALUE = 17` (in `GameState.h`)
- `UILayout::CARD_SIZE`, `CARD_SPACING` (in `GameState.h`)
- `AnimConfig::PHASE_TEXT_DURATION`, `CARD_DRAW_DURATION` (in `AnimationManager.h`)

## Adding new features

| Feature | Where to add |
|---------|-------------|
| New phase | `phaseEngine/` + register in `RoundManager::createPhase()` |
| New skill | `skillEngine/Skill_Name.h` + register in `SkillManager` |
| New animation effect | Load texture in `AnimationManager` constructor, use `playSpriteAnimation()` |
| UI changes | `UIManager.h/cpp` + `VisualState.h` for layout |
| Game rules | `Player.h/cpp` or relevant phase in `phaseEngine/` |
