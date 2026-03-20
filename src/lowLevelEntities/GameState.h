#ifndef GAMESTATE_H
#define GAMESTATE_H
#include "Card.h"
#include "Deck.h"
#include "SkillDeck.h"
#include <vector>
namespace GameConfig {
    constexpr int WINDOW_WIDTH = 683;
    constexpr int WINDOW_HEIGHT = 384;
    constexpr int BLACKJACK_VALUE = 21;
    constexpr int DEALER_STAND_VALUE = 17;
    constexpr int ACE_HIGH_VALUE = 11;
    constexpr int ACE_LOW_VALUE = 1;
    constexpr int FACE_CARD_VALUE = 10;
    constexpr int CARD_RANKS = 13;
    constexpr int HAND_START_VALUE = 2;
    constexpr int WINNING_POINTS = 15;
    // Bot behavior constants
    constexpr double BURST_RISK_WEIGHT = 0.5;
    constexpr double OPPONENT_PRESSURE_WEIGHT = 0.70;
    constexpr double LOSING_PRESSURE_WEIGHT = 0.15;
}
enum class PhaseName{
    GAME_START_PHASE,
    BLACKJACK_CHECK_PHASE,
    PLAYER_HIT_PHASE,
    HOST_HIT_PHASE,
    BATTLE_PHASE,
    ROUND_END,
    GAME_OVER
};
enum class CardLocation{
    HAND,
    DECK,
    DISCARD_PILE
};
struct PlayerInfo {
    int playerId;
    std::vector<Card> cardsInHand;
    SkillName skill;
    int skillUses;
    int points;
    bool isBot = false;
    bool isHost = false;
    bool isRemote = false;
};
enum class PlayerAction{
    HIT,
    SKILL_REQUEST,
    STAND,
    IDLE
};
enum class ReactiveResponse { NONE, YES, NO };
struct PlayerTargeting{
    std::vector<int> targetPlayerIds;
    std::vector<Card> targetCards;
};
class GameState {
private:
    std::vector<PlayerInfo> playersInfo;
    PhaseName phase;
    int currentPlayerId=0;
    int playersProcessed=0;
    std::vector<Card> deckCards;
public:
    PlayerAction pendingPlayerAction = PlayerAction::IDLE;
    PlayerTargeting pendingTarget;
    ReactiveResponse pendingReactiveResponse = ReactiveResponse::NONE;
    void setAllPlayerInfo(std::vector<PlayerInfo> playersInfo);
    void setPhaseName(PhaseName newPhaseName, int newCurrentPlayerId);
    void setDeckCards(const std::vector<Card>& cards) { deckCards = cards; }
    //getters
    int getDeckCount() const { return (int)deckCards.size(); }
    const std::vector<Card>& getDeckCards() const { return deckCards; }
    int getCurrentPlayerId() {return currentPlayerId;}
    void incrementCurrentPlayerId(int numPlayers) {
        currentPlayerId = (currentPlayerId + 1) % numPlayers;
        playersProcessed++;
    }
    bool allPlayersProcessed(int numPlayers) const {
        return playersProcessed >= numPlayers;
    }
    PhaseName getPhaseName() const;
    PlayerInfo getPlayerInfo(int id) const {
        for (auto& player : playersInfo){
            if (player.playerId==id)
            return player;
        }
        std::cout << "Can't find player "<< id << std::endl;
        return {};
    }
    std::vector<PlayerInfo> getAllPlayerInfo() { return playersInfo; }

    Rank getCardRank(int playerId, int cardIndex){
        for (auto& player : playersInfo){
            if (player.playerId==playerId)
            return player.cardsInHand[cardIndex].getRank();
        }
        std::cout << "Can't find player "<< playerId << " or card index " << cardIndex << std::endl;
        return Rank::Ace; //default return to avoid compile error
    }
    Suit getCardSuit(int playerId, int cardIndex){
        for (auto& player : playersInfo){
            if (player.playerId==playerId)
            return player.cardsInHand[cardIndex].getSuit();
        }
        std::cout << "Can't find player "<< playerId << " or card index " << cardIndex << std::endl;
        return Suit::Hearts; //default return to avoid compile error
    }
    int getCardRankBonus(int playerId, int cardIndex){
        for (auto& player : playersInfo){
            if (player.playerId==playerId)
            return player.cardsInHand[cardIndex].getRankBonus();
        }
        std::cout << "Can't find player "<< playerId << " or card index " << cardIndex << std::endl;
        return 0; //default return to avoid compile error
    }
    bool isCardFaceUp(int playerId, int cardIndex){
        for (auto& player : playersInfo){
            if (player.playerId==playerId)
            return player.cardsInHand[cardIndex].isFaceUp();
        }
        std::cout << "Can't find player "<< playerId << " or card index " << cardIndex << std::endl;
        return false; //default return to avoid compile error
    }
    std::string phaseToString(PhaseName phase) {
        switch (phase) {
            case PhaseName::GAME_START_PHASE: return "GAME START PHASE";
            case PhaseName::BLACKJACK_CHECK_PHASE: return "BLACKJACK CHECK PHASE";
            case PhaseName::PLAYER_HIT_PHASE: return "PLAYER HIT PHASE";
            case PhaseName::HOST_HIT_PHASE: return "HOST HIT PHASE";
            case PhaseName::BATTLE_PHASE: return "BATTLE PHASE";
            case PhaseName::ROUND_END: return "ROUND END";
            case PhaseName::GAME_OVER: return "GAME OVER";
            default: return "UNKNOWN PHASE";
        }
    }
    
    std::string skillNameToString(SkillName skill) {
        switch (skill) {
            case SkillName::DELIVERANCE:   return "DELIVERANCE";
            case SkillName::NEURALGAMBIT:  return "NEURAL GAMBIT";
            case SkillName::FATALDEAL:     return "FATAL DEAL";
            case SkillName::CLONE:         return "CLONE";
            case SkillName::BOOGIEWOOGIE:  return "BOOGIE WOOGIE";
            case SkillName::LOOKMAXXING:   return "LOOKMAXXING";
            case SkillName::SIGMA:         return "SIGMA";
            case SkillName::UNDEFINED:     return "UNDEFINED";
            default:                       return "UNKNOWN";
        }
    }

    void printAllInfo();

};

#endif