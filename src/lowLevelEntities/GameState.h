#ifndef GAMESTATE_H
#define GAMESTATE_H
#include "Card.h"
#include "Deck.h"
#include "SkillDeck.h"
#include <vector>
namespace GameConfig {
    constexpr int WINDOW_WIDTH = 1366;
    constexpr int WINDOW_HEIGHT = 768;
    constexpr int BLACKJACK_VALUE = 21;
    constexpr int DEALER_STAND_VALUE = 17;
    constexpr int ACE_HIGH_VALUE = 11;
    constexpr int ACE_LOW_VALUE = 1;
    constexpr int FACE_CARD_VALUE = 10;
    constexpr int CARD_RANKS = 13;
    
    // Bot behavior constants
    constexpr double BURST_RISK_WEIGHT = 0.5;
    constexpr double OPPONENT_PRESSURE_WEIGHT = 0.70;
    constexpr double LOSING_PRESSURE_WEIGHT = 0.15;
}
enum class PhaseName{
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
};
enum class PlayerAction{
    HIT,
    SKILL_REQUEST,
    STAND,
    IDLE
};
struct PlayerTargeting{
    std::vector<int> targetPlayerIds;
    std::vector<Card> targetCards;
};
class GameState {
private:
    std::vector<PlayerInfo> playersInfo;
    PhaseName phase;
    int currentPlayerId=0;
    int deckCount;
public:
    PlayerAction pendingPlayerAction = PlayerAction::IDLE;
    PlayerTargeting pendingTarget;
    void setAllPlayerInfo(std::vector<PlayerInfo> playersInfo);
    void setPhaseName(PhaseName newPhaseName, int newCurrentPlayerId);
    void setDeckCount(int count){
        deckCount=count;
    }
    //getters
    int getCurrentPlayerId() {return currentPlayerId;}
    void incrementCurrentPlayerId(int numPlayers) {
        currentPlayerId++;
        if (currentPlayerId >= numPlayers) {
            //std::cout << "[GameState][incrementCurrentPlayerId] Current player ID exceeded number of players" << std::endl;
           currentPlayerId = -1;
        }
    }
    PhaseName getPhaseName();
    PlayerInfo getPlayerInfo(int id){
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
            case PhaseName::BLACKJACK_CHECK_PHASE: return "BLACKJACK CHECK PHASE";
            case PhaseName::PLAYER_HIT_PHASE: return "PLAYER HIT PHASE";
            case PhaseName::HOST_HIT_PHASE: return "HOST HIT PHASE";
            case PhaseName::BATTLE_PHASE: return "BATTLE PHASE";
            case PhaseName::ROUND_END: return "ROUND END";
            default: return "UNKNOWN PHASE";
        }
    }
    
    std::string skillNameToString(SkillName skill) {
        switch (skill) {
            case SkillName::DELIVERANCE: return "DELIVERANCE";
            default: return "UNKNOWN";
        }
    }

    void printAllInfo();

};

#endif