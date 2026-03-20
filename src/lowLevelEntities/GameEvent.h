#ifndef GAMEEVENT_H
#define GAMEEVENT_H
#include "SkillDeck.h"
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
    SKILL_ERROR,

    SHOCK_EFFECT,
    EXPLOSION_EFFECT,
    DELIVERANCE_EFFECT,

    REQUEST_ACTION_INPUT,
    REQUEST_TARGET_INPUT,
    CLEAR_INPUT,

    NEURALGAMBIT_REVEAL,         // skill visual: flash both revealed cards

    REACTIVE_SKILL_PROMPT,       // tell presentation to show yes/no prompt
    FATALDEAL_SWAP,              // skill visual: card swap animation
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

struct SkillErrorEvent {
    int playerId;
    std::string reason;
};
struct ClearInputEvent {};

struct NeuralGambitRevealEvent {
    int cardId1;  // user's card
    int cardId2;  // target's card
    int boostCardId;
    int boostAmount;
};

struct ReactiveSkillPromptEvent {
    int skillOwnerId;
    SkillName skillName;
    float timerDuration;
};

struct FatalDealSwapEvent {
    int drawerId;         // player who drew the card
    int fatalDealUserId;  // Fatal Deal owner
    int drawnCardId;      // card that was drawn (now going to FD user)
    int swappedCardId;    // card from FD user's hand (now going to drawer)
};

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
    ClearInputEvent,
    SkillErrorEvent,
    NeuralGambitRevealEvent,
    ReactiveSkillPromptEvent,
    FatalDealSwapEvent
>;

struct GameEvent {
    GameEventType type;
    GameEventData data;
};

#endif
