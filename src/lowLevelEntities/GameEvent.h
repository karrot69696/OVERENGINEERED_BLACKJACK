#ifndef GAMEEVENT_H
#define GAMEEVENT_H

#include <variant>
#include <vector>
#include <string>

// Every event the game logic can emit — pure data, no rendering types
enum class GameEventType {
    CARD_DRAWN,
    CARD_RETURNED,
    CARD_DELIVERANCE,            // skill visual: spin → return → reposition
    HAND_REPOSITIONED,

    BATTLE_WIN,
    BATTLE_TIE,
    POINT_CHANGED,

    PHASE_ANNOUNCED,

    SHOCK_EFFECT,
    EXPLOSION_EFFECT,
    DELIVERANCE_EFFECT,

    REQUEST_ACTION_INPUT,
    REQUEST_TARGET_INPUT,
    CLEAR_INPUT,
};

// Payloads carry IDs only, never screen positions
struct CardDrawnEvent {
    int playerId;
    int handIndex;
    int cardId;
};

struct CardReturnedEvent {
    int cardId;
};

struct CardSpinEvent {
    int cardId;
    int playerId; // owner before skill fired
};

struct HandRepositionedEvent {
    int playerId;
};

struct BattleWinEvent {
    int winnerId;
    int loserId;
};

struct BattleTieEvent {
    int player1Id;
    int player2Id;
};

struct PointChangedEvent {
    int playerId;
    std::string text; // "+1 POINT", "+0"
};

struct PhaseAnnouncedEvent {
    std::string text;
    float duration;
};

struct ShockEffectEvent {
    int winnerId;
    int loserId;
    float duration;
};

struct ExplosionEffectEvent {
    int targetPlayerId;
    float scale;
    float duration;
};

struct DeliveranceEffectEvent {
    int cardId;
};

struct RequestActionInputEvent {
    int playerId;
};

struct RequestTargetInputEvent {
    int playerId;
};

struct ClearInputEvent {};

using GameEventData = std::variant<
    CardDrawnEvent,
    CardReturnedEvent,
    CardSpinEvent,
    HandRepositionedEvent,
    BattleWinEvent,
    BattleTieEvent,
    PointChangedEvent,
    PhaseAnnouncedEvent,
    ShockEffectEvent,
    ExplosionEffectEvent,
    DeliveranceEffectEvent,
    RequestActionInputEvent,
    RequestTargetInputEvent,
    ClearInputEvent
>;

struct GameEvent {
    GameEventType type;
    GameEventData data;
};

#endif
