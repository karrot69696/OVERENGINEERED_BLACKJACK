#ifndef GAMEEVENT_H
#define GAMEEVENT_H
#include "SkillDeck.h"
#include "GameConfig.h"
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
    FATALDEAL_EFFECT,
    REQUEST_ACTION_INPUT,
    REQUEST_TARGET_INPUT,
    CLEAR_INPUT,

    NEURALGAMBIT_REVEAL,         // skill visual: flash both revealed cards

    REACTIVE_SKILL_PROMPT,       // tell presentation to show yes/no prompt
    FATALDEAL_SWAP,              // skill visual: card swap animation
    CARDS_REVEALED,              // flip cards face-up with animation

    CHRONOSPHERE_SNAPSHOT,       // skill visual: snapshot taken
    CHRONOSPHERE_REWIND,         // skill visual: hand rewound to snapshot

    DESTINYDEFLECT_EFFECT,
    CARD_REDIRECTED,             // skill visual: card redirect animation
    DECK_PEEK,                   // passive: show top N deck cards (non-blocking)
    CARD_REDRAWN,                // chrono rewind: batch card draw (no reactive trigger, all animate at once)
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
    int playerId;
};

struct FatalDealEffectEvent {
    int playerId;
};
struct RequestActionInputEvent {
    int playerId;
    float duration = GameConfig::ACTION_PROMPT_DURATION;
};

struct RequestTargetInputEvent {
    int playerId;
    float duration = GameConfig::TARGET_PROMPT_DURATION;
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
    std::string extraInfo;
    float timerDuration;
};

struct FatalDealSwapEvent {
    int drawerId;         // player who drew the card
    int fatalDealUserId;  // Fatal Deal owner
    int drawnCardId;      // card that was drawn (now going to FD user)
    int swappedCardId;    // card from FD user's hand (now going to drawer)
};

struct CardsRevealedEvent {
    std::vector<int> cardIds;
};

struct ChronosphereSnapshotEvent {
    int playerId;
};

struct ChronosphereRewindEvent {
    int playerId;
};

struct CardRedirectedEvent {
    int cardId;
    int fromPlayerId;
    int toPlayerId;
};
struct DestinyDeflectEffectEvent{
    int playerId;
};

struct DeckPeekEvent {
    int playerId;
    std::vector<int> cardIds;
    std::vector<uint8_t> ranks;
    std::vector<uint8_t> suits;
};

struct CardRedrawnEvent {
    int playerId;
    std::vector<int> cardIds;       // all cards dealt in the rewind
    std::vector<int> handIndices;   // corresponding hand index for each card
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
    FatalDealEffectEvent,
    RequestActionInputEvent,
    RequestTargetInputEvent,
    ClearInputEvent,
    SkillErrorEvent,
    NeuralGambitRevealEvent,
    ReactiveSkillPromptEvent,
    FatalDealSwapEvent,
    CardsRevealedEvent,
    ChronosphereSnapshotEvent,
    ChronosphereRewindEvent,
    CardRedirectedEvent,
    DeckPeekEvent,
    DestinyDeflectEffectEvent,
    CardRedrawnEvent
>;

struct GameEvent {
    GameEventType type;
    GameEventData data;
};

#endif
