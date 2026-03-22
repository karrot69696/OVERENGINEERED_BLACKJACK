#ifndef NETSERIALIZER_H
#define NETSERIALIZER_H

#include <vector>
#include <cstdint>
#include <cstring>
#include <string>
#include "lowLevelEntities/GameState.h"
#include "lowLevelEntities/GameEvent.h"

// Simple byte buffer for serialization
class ByteBuffer {
private:
    std::vector<uint8_t> data;
    size_t readPos = 0;

public:
    ByteBuffer() = default;
    ByteBuffer(const uint8_t* src, size_t len) : data(src, src + len) {}

    const uint8_t* getData() const { return data.data(); }
    size_t getSize() const { return data.size(); }
    size_t getReadPos() const { return readPos; }
    bool canRead(size_t bytes) const { return readPos + bytes <= data.size(); }

    // Write primitives
    void writeU8(uint8_t v) { data.push_back(v); }
    void writeU16(uint16_t v) {
        data.push_back(static_cast<uint8_t>(v & 0xFF));
        data.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
    }
    void writeI32(int32_t v) {
        uint32_t u;
        std::memcpy(&u, &v, sizeof(u));
        data.push_back(static_cast<uint8_t>(u & 0xFF));
        data.push_back(static_cast<uint8_t>((u >> 8) & 0xFF));
        data.push_back(static_cast<uint8_t>((u >> 16) & 0xFF));
        data.push_back(static_cast<uint8_t>((u >> 24) & 0xFF));
    }
    void writeFloat(float v) {
        uint32_t u;
        std::memcpy(&u, &v, sizeof(u));
        writeI32(static_cast<int32_t>(u));
    }
    void writeString(const std::string& s) {
        writeU16(static_cast<uint16_t>(s.size()));
        for (char c : s) data.push_back(static_cast<uint8_t>(c));
    }
    void writeBool(bool v) { writeU8(v ? 1 : 0); }

    // Read primitives
    uint8_t readU8() {
        uint8_t v = data[readPos++];
        return v;
    }
    uint16_t readU16() {
        uint16_t v = data[readPos] | (static_cast<uint16_t>(data[readPos + 1]) << 8);
        readPos += 2;
        return v;
    }
    int32_t readI32() {
        uint32_t u = data[readPos]
            | (static_cast<uint32_t>(data[readPos + 1]) << 8)
            | (static_cast<uint32_t>(data[readPos + 2]) << 16)
            | (static_cast<uint32_t>(data[readPos + 3]) << 24);
        readPos += 4;
        int32_t v;
        std::memcpy(&v, &u, sizeof(v));
        return v;
    }
    float readFloat() {
        int32_t i = readI32();
        uint32_t u = static_cast<uint32_t>(i);
        float v;
        std::memcpy(&v, &u, sizeof(v));
        return v;
    }
    std::string readString() {
        uint16_t len = readU16();
        std::string s(data.begin() + readPos, data.begin() + readPos + len);
        readPos += len;
        return s;
    }
    bool readBool() { return readU8() != 0; }
};

// ============================================================================
// Target request (server → specific client, for multi-actor skills)
// ============================================================================
struct TargetRequestData {
    int requesterId = -1;             // who triggered the skill
    std::vector<int> allowedCardIds;  // card IDs the recipient may pick from
    bool isBoostPick = false;         // false = pick your card, true = pick which card to boost
};

struct ReactivePromptData {
    SkillName skill;
    std::string extraInfo;
    float timerDuration;
};

// ============================================================================
// Card serialization (Suit, Rank, faceUp, id, ownerId, handIndex)
// ============================================================================
namespace NetSerializer {

inline void writeCard(ByteBuffer& buf, const Card& card) {
    buf.writeI32(card.getId());
    buf.writeU8(static_cast<uint8_t>(card.getSuit()));
    buf.writeU8(static_cast<uint8_t>(card.getRank()));
    buf.writeBool(card.isFaceUp());
    buf.writeI32(card.getRankBonus());
}

inline Card readCard(ByteBuffer& buf) {
    int id = buf.readI32();
    Suit suit = static_cast<Suit>(buf.readU8());
    Rank rank = static_cast<Rank>(buf.readU8());
    bool faceUp = buf.readBool();
    int rankBonus = buf.readI32();
    Card c(suit, rank, faceUp);
    c.setId(id);
    c.setRankBonus(rankBonus);
    return c;
}

// ============================================================================
// PlayerInfo serialization
// ============================================================================
inline void writePlayerInfo(ByteBuffer& buf, const PlayerInfo& info) {
    buf.writeI32(info.playerId);
    buf.writeU8(static_cast<uint8_t>(info.skill));
    buf.writeI32(info.skillUses);
    buf.writeI32(info.points);
    buf.writeI32(info.handValue);
    buf.writeBool(info.isBot);
    buf.writeBool(info.isHost);
    buf.writeU16(static_cast<uint16_t>(info.cardsInHand.size()));
    for (const Card& card : info.cardsInHand) {
        writeCard(buf, card);
    }
}

inline PlayerInfo readPlayerInfo(ByteBuffer& buf) {
    PlayerInfo info;
    info.playerId = buf.readI32();
    info.skill = static_cast<SkillName>(buf.readU8());
    info.skillUses = buf.readI32();
    info.points = buf.readI32();
    info.handValue = buf.readI32();
    info.isBot = buf.readBool();
    info.isHost = buf.readBool();
    uint16_t cardCount = buf.readU16();
    for (uint16_t i = 0; i < cardCount; i++) {
        info.cardsInHand.push_back(readCard(buf));
    }
    return info;
}

// ============================================================================
// GameState serialization
// ============================================================================
inline void writeGameState(ByteBuffer& buf, GameState& state) {
    buf.writeU8(static_cast<uint8_t>(state.getPhaseName()));
    buf.writeI32(state.getCurrentPlayerId());
    // Deck cards
    const auto& deckCards = state.getDeckCards();
    buf.writeU16(static_cast<uint16_t>(deckCards.size()));
    for (const Card& card : deckCards) {
        writeCard(buf, card);
    }
    // Players
    auto allInfo = state.getAllPlayerInfo();
    buf.writeU16(static_cast<uint16_t>(allInfo.size()));
    for (const PlayerInfo& info : allInfo) {
        writePlayerInfo(buf, info);
    }
}

inline void readGameState(ByteBuffer& buf, GameState& state) {
    PhaseName phase = static_cast<PhaseName>(buf.readU8());
    int currentPlayerId = buf.readI32();
    // Deck cards
    uint16_t deckCardCount = buf.readU16();
    std::vector<Card> deckCards;
    for (uint16_t i = 0; i < deckCardCount; i++) {
        deckCards.push_back(readCard(buf));
    }
    // Players
    uint16_t playerCount = buf.readU16();
    std::vector<PlayerInfo> allInfo;
    for (uint16_t i = 0; i < playerCount; i++) {
        allInfo.push_back(readPlayerInfo(buf));
    }
    state.setAllPlayerInfo(allInfo);
    state.setPhaseName(phase, currentPlayerId);
    state.setDeckCards(deckCards);
}

// ============================================================================
// PlayerAction serialization
// ============================================================================
inline void writePlayerAction(ByteBuffer& buf, int playerId, PlayerAction action) {
    buf.writeI32(playerId);
    buf.writeU8(static_cast<uint8_t>(action));
}

inline void readPlayerAction(ByteBuffer& buf, int& playerId, PlayerAction& action) {
    playerId = buf.readI32();
    action = static_cast<PlayerAction>(buf.readU8());
}

// ============================================================================
// PlayerTargeting serialization
// ============================================================================
inline void writePlayerTargeting(ByteBuffer& buf, int playerId, const PlayerTargeting& targeting) {
    buf.writeI32(playerId);
    buf.writeU16(static_cast<uint16_t>(targeting.targetPlayerIds.size()));
    for (int id : targeting.targetPlayerIds) {
        buf.writeI32(id);
    }
    buf.writeU16(static_cast<uint16_t>(targeting.targetCards.size()));
    for (const Card& card : targeting.targetCards) {
        writeCard(buf, card);
    }
}

inline void readPlayerTargeting(ByteBuffer& buf, int& playerId, PlayerTargeting& targeting) {
    playerId = buf.readI32();
    uint16_t targetCount = buf.readU16();
    for (uint16_t i = 0; i < targetCount; i++) {
        targeting.targetPlayerIds.push_back(buf.readI32());
    }
    uint16_t cardCount = buf.readU16();
    for (uint16_t i = 0; i < cardCount; i++) {
        targeting.targetCards.push_back(readCard(buf));
    }
}

// ============================================================================
// GameEvent serialization
// ============================================================================
inline void writeGameEvent(ByteBuffer& buf, const GameEvent& event) {
    buf.writeU8(static_cast<uint8_t>(event.type));

    std::visit([&](const auto& payload) {
        using T = std::decay_t<decltype(payload)>;

        if constexpr (std::is_same_v<T, CardDrawnEvent>) {
            buf.writeI32(payload.playerId);
            buf.writeI32(payload.handIndex);
            buf.writeI32(payload.cardId);
        }
        else if constexpr (std::is_same_v<T, CardReturnedEvent>) {
            buf.writeI32(payload.cardId);
        }
        else if constexpr (std::is_same_v<T, CardSpinEvent>) {
            buf.writeI32(payload.cardId);
            buf.writeI32(payload.playerId);
        }
        else if constexpr (std::is_same_v<T, HandRepositionedEvent>) {
            buf.writeI32(payload.playerId);
        }
        else if constexpr (std::is_same_v<T, BattleWinEvent>) {
            buf.writeI32(payload.winnerId);
            buf.writeI32(payload.loserId);
        }
        else if constexpr (std::is_same_v<T, BattleTieEvent>) {
            buf.writeI32(payload.player1Id);
            buf.writeI32(payload.player2Id);
        }
        else if constexpr (std::is_same_v<T, PointChangedEvent>) {
            buf.writeI32(payload.playerId);
            buf.writeString(payload.text);
        }
        else if constexpr (std::is_same_v<T, PhaseAnnouncedEvent>) {
            buf.writeString(payload.text);
            buf.writeFloat(payload.duration);
        }
        else if constexpr (std::is_same_v<T, ShockEffectEvent>) {
            buf.writeI32(payload.winnerId);
            buf.writeI32(payload.loserId);
            buf.writeFloat(payload.duration);
        }
        else if constexpr (std::is_same_v<T, ExplosionEffectEvent>) {
            buf.writeI32(payload.targetPlayerId);
            buf.writeFloat(payload.scale);
            buf.writeFloat(payload.duration);
        }
        else if constexpr (std::is_same_v<T, DeliveranceEffectEvent>) {
            buf.writeI32(payload.playerId);
        }
        else if constexpr (std::is_same_v<T, RequestActionInputEvent>) {
            buf.writeI32(payload.playerId);
        }
        else if constexpr (std::is_same_v<T, RequestTargetInputEvent>) {
            buf.writeI32(payload.playerId);
        }
        else if constexpr (std::is_same_v<T, ClearInputEvent>) {
            // no payload
        }
        else if constexpr (std::is_same_v<T, SkillErrorEvent>) {
            buf.writeI32(payload.playerId);
            buf.writeString(payload.reason);
        }
        else if constexpr (std::is_same_v<T, NeuralGambitRevealEvent>) {
            buf.writeI32(payload.cardId1);
            buf.writeI32(payload.cardId2);
            buf.writeI32(payload.boostCardId);
            buf.writeI32(payload.boostAmount);
        }
        else if constexpr (std::is_same_v<T, ReactiveSkillPromptEvent>) {
            buf.writeI32(payload.skillOwnerId);
            buf.writeU8(static_cast<uint8_t>(payload.skillName));
            buf.writeFloat(payload.timerDuration);
        }
        else if constexpr (std::is_same_v<T, FatalDealSwapEvent>) {
            buf.writeI32(payload.drawerId);
            buf.writeI32(payload.fatalDealUserId);
            buf.writeI32(payload.drawnCardId);
            buf.writeI32(payload.swappedCardId);
        }
        else if constexpr (std::is_same_v<T, CardsRevealedEvent>) {
            buf.writeU16(static_cast<uint16_t>(payload.cardIds.size()));
            for (int id : payload.cardIds) {
                buf.writeI32(id);
            }
        }
    }, event.data);
}

inline GameEvent readGameEvent(ByteBuffer& buf) {
    GameEventType type = static_cast<GameEventType>(buf.readU8());
    GameEventData data;

    switch (type) {
        case GameEventType::CARD_DRAWN: {
            CardDrawnEvent e;
            e.playerId = buf.readI32();
            e.handIndex = buf.readI32();
            e.cardId = buf.readI32();
            data = e;
        } break;
        case GameEventType::CARD_RETURNED: {
            CardReturnedEvent e;
            e.cardId = buf.readI32();
            data = e;
        } break;
        case GameEventType::CARD_DELIVERANCE: {
            CardSpinEvent e;
            e.cardId = buf.readI32();
            e.playerId = buf.readI32();
            data = e;
        } break;
        case GameEventType::HAND_REPOSITIONED: {
            HandRepositionedEvent e;
            e.playerId = buf.readI32();
            data = e;
        } break;
        case GameEventType::BATTLE_WIN: {
            BattleWinEvent e;
            e.winnerId = buf.readI32();
            e.loserId = buf.readI32();
            data = e;
        } break;
        case GameEventType::BATTLE_TIE: {
            BattleTieEvent e;
            e.player1Id = buf.readI32();
            e.player2Id = buf.readI32();
            data = e;
        } break;
        case GameEventType::POINT_CHANGED: {
            PointChangedEvent e;
            e.playerId = buf.readI32();
            e.text = buf.readString();
            data = e;
        } break;
        case GameEventType::PHASE_ANNOUNCED: {
            PhaseAnnouncedEvent e;
            e.text = buf.readString();
            e.duration = buf.readFloat();
            data = e;
        } break;
        case GameEventType::SHOCK_EFFECT: {
            ShockEffectEvent e;
            e.winnerId = buf.readI32();
            e.loserId = buf.readI32();
            e.duration = buf.readFloat();
            data = e;
        } break;
        case GameEventType::EXPLOSION_EFFECT: {
            ExplosionEffectEvent e;
            e.targetPlayerId = buf.readI32();
            e.scale = buf.readFloat();
            e.duration = buf.readFloat();
            data = e;
        } break;
        case GameEventType::DELIVERANCE_EFFECT: {
            DeliveranceEffectEvent e;
            e.playerId = buf.readI32();
            data = e;
        } break;
        case GameEventType::REQUEST_ACTION_INPUT: {
            RequestActionInputEvent e;
            e.playerId = buf.readI32();
            data = e;
        } break;
        case GameEventType::REQUEST_TARGET_INPUT: {
            RequestTargetInputEvent e;
            e.playerId = buf.readI32();
            data = e;
        } break;
        case GameEventType::CLEAR_INPUT: {
            data = ClearInputEvent{};
        } break;
        case GameEventType::SKILL_ERROR: {
            SkillErrorEvent e;
            e.playerId = buf.readI32();
            e.reason = buf.readString();
            data = e;
        } break;
        case GameEventType::NEURALGAMBIT_REVEAL: {
            NeuralGambitRevealEvent e;
            e.cardId1 = buf.readI32();
            e.cardId2 = buf.readI32();
            e.boostCardId = buf.readI32();
            e.boostAmount = buf.readI32();
            data = e;
        } break;
        case GameEventType::REACTIVE_SKILL_PROMPT: {
            ReactiveSkillPromptEvent e;
            e.skillOwnerId = buf.readI32();
            e.skillName = static_cast<SkillName>(buf.readU8());
            e.timerDuration = buf.readFloat();
            data = e;
        } break;
        case GameEventType::FATALDEAL_SWAP: {
            FatalDealSwapEvent e;
            e.drawerId = buf.readI32();
            e.fatalDealUserId = buf.readI32();
            e.drawnCardId = buf.readI32();
            e.swappedCardId = buf.readI32();
            data = e;
        } break;
        case GameEventType::CARDS_REVEALED: {
            CardsRevealedEvent e;
            uint16_t count = buf.readU16();
            for (uint16_t i = 0; i < count; i++) {
                e.cardIds.push_back(buf.readI32());
            }
            data = e;
        } break;
    }

    return GameEvent{type, data};
}

// Batch write/read for event lists
inline void writeEventBatch(ByteBuffer& buf, const std::vector<GameEvent>& events) {
    buf.writeU16(static_cast<uint16_t>(events.size()));
    for (const GameEvent& e : events) {
        writeGameEvent(buf, e);
    }
}

inline std::vector<GameEvent> readEventBatch(ByteBuffer& buf) {
    uint16_t count = buf.readU16();
    std::vector<GameEvent> events;
    events.reserve(count);
    for (uint16_t i = 0; i < count; i++) {
        events.push_back(readGameEvent(buf));
    }
    return events;
}

// ============================================================================
// TargetRequest serialization
// ============================================================================
inline void writeTargetRequest(ByteBuffer& buf, const TargetRequestData& req) {
    buf.writeI32(req.requesterId);
    buf.writeBool(req.isBoostPick);
    buf.writeU16(static_cast<uint16_t>(req.allowedCardIds.size()));
    for (int id : req.allowedCardIds) buf.writeI32(id);
}

inline TargetRequestData readTargetRequest(ByteBuffer& buf) {
    TargetRequestData req;
    req.requesterId = buf.readI32();
    req.isBoostPick = buf.readBool();
    uint16_t n = buf.readU16();
    for (uint16_t i = 0; i < n; i++) req.allowedCardIds.push_back(buf.readI32());
    return req;
}

// ============================================================================
// ReactivePrompt serialization
// ============================================================================
inline void writeReactivePrompt(ByteBuffer& buf, SkillName skill, std::string extraInfo, float timerDuration) {
    buf.writeU8(static_cast<uint8_t>(skill));
    buf.writeString(extraInfo);
    buf.writeFloat(timerDuration);
}

inline ReactivePromptData readReactivePrompt(ByteBuffer& buf) {
    ReactivePromptData data;
    data.skill = static_cast<SkillName>(buf.readU8());
    data.extraInfo = buf.readString();
    data.timerDuration = buf.readFloat();
    return data;
}

inline void writeReactiveResponse(ByteBuffer& buf, int playerId, bool accepted) {
    buf.writeI32(playerId);
    buf.writeBool(accepted);
}

inline void readReactiveResponse(ByteBuffer& buf, int& playerId, bool& accepted) {
    playerId = buf.readI32();
    accepted = buf.readBool();
}

} // namespace NetSerializer

#endif
