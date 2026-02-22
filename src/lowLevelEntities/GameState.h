#ifndef GAMESTATE_H
#define GAMESTATE_H
#include "Card.h"
#include "Deck.h"
#include "SkillDeck.h"
#include <vector>
enum class Phase{
    BLACKJACK_CHECK_PHASE,
    PLAYER_HIT_PHASE,
    HOST_HIT_PHASE,
    BATTLE_PHASE,
    ROUND_END,
};
struct RevealedCardInfo {
    int ownerId;
    Suit suit;
    Rank rank;
};

struct PlayerInfo {
    int playerId;
    std::vector<Card> cardsInHand;
    SkillName skill;
    int skillUses;
    int points;
};

class GameState {
private:
    std::vector<PlayerInfo> playersInfo;
    Phase phase;
    int currentPlayerId=0;
    int deckCount;
public:
    void addRevealedCard(int ownerId, Card& card);
    void updateAllPlayerInfo(std::vector<PlayerInfo> playersInfo);
    std::string revealCardRankToString(const RevealedCardInfo& info);
    std::string revealCardSuitToString(const RevealedCardInfo& info);
    void updatePhase(Phase newPhase, int newCurrentPlayerId);
    void updateDeckCount(int count){
        deckCount=count;
    }
    void updatePhase(Phase newPhase);
    int getCurrentPlayerId() {return currentPlayerId;}
    Phase getPhase();
    PlayerInfo getPlayerInfo(int id){
        for (auto& player : playersInfo){
            if (player.playerId==id)
            return player;
        }
        std::cout << "Can't find player "<< id << std::endl;
        return {};
    }
    std::vector<PlayerInfo> getAllPlayerInfo() { return playersInfo; }
    std::string phaseToString(Phase phase) {
        switch (phase) {
            case Phase::BLACKJACK_CHECK_PHASE: return "BLACKJACK_CHECK_PHASE";
            case Phase::PLAYER_HIT_PHASE: return "PLAYER_HIT_PHASE";
            case Phase::HOST_HIT_PHASE: return "HOST_HIT_PHASE";
            case Phase::BATTLE_PHASE: return "BATTLE_PHASE";
            case Phase::ROUND_END: return "ROUND_END";
            default: return "UNKNOWN_PHASE";
        }
    }
    
    std::string skillNameToString(SkillName skill) {
        switch (skill) {
            case SkillName::DELIVERANCE: return "DELIVERANCE";
            default: return "UNKNOWN_SKILL";
        }
    }

    void printAllInfo();

};

#endif