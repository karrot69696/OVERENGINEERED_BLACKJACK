#ifndef GAMESTATE_H
#define GAMESTATE_H
#include "Card.h"
#include "Deck.h"
#include "SkillDeck.h"
#include <vector>
#include "GameConfig.h"
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
    int handValue = 0;
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
enum class ChronoChoice { NONE, SNAPSHOT, REWIND };
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
    bool snapShotTaken = false;
    std::vector<Rank> ranksSnapShot;
    ChronoChoice pendingChronoChoice = ChronoChoice::NONE;
    int chronoTrackedHandValue = -1;
    void setAllPlayerInfo(std::vector<PlayerInfo> playersInfo);
    void setPhaseName(PhaseName newPhaseName, int newCurrentPlayerId);
    void setDeckCards(const std::vector<Card>& cards) { deckCards = cards; }
    //getters
    int getDeckCount() const { return (int)deckCards.size(); }
    const std::vector<Card>& getDeckCards() const { return deckCards; }
    int getCurrentPlayerId() {return currentPlayerId;}
    int getHostPlayerId(){
        for (auto& player : playersInfo)
            if (player.isHost) return player.playerId;
        return -1;
    }
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

    Rank getCardRankById(int cardId){
        for (auto& player : playersInfo)
            for (auto& card : player.cardsInHand)
                if (card.getId() == cardId) return card.getRank();
        std::cout << "Can't find card id " << cardId << std::endl;
        return Rank::Ace;
    }
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
            case SkillName::CHRONOSPHERE:  return "CHRONOSPHERE";
            case SkillName::DESTINYDEFLECT: return "DESTINY DEFLECT";
            case SkillName::BOOGIEWOOGIE:  return "BOOGIE WOOGIE";
            case SkillName::LOOKMAXXING:   return "LOOKMAXXING";
            case SkillName::SIGMA:         return "SIGMA";
            case SkillName::UNDEFINED:     return "UNDEFINED";
            default:                       return "UNKNOWN";
        }
    }

    std::string skillDescriptionToString(SkillName skill) {
        switch (skill) {
            case SkillName::DELIVERANCE:    
            return "Active: During your hit phase, you may spend 1 use, return a card from your hand to the deck.\nPassive - Sanctum: At the start of your hit phase, you gain 1 use";
            case SkillName::NEURALGAMBIT:   
            return "Active: During your hit phase, you may spend 1 use, choose 2 players, each chosen player reveal 1 card from their hand, \nyou choose 1 of the cards revealed, that card rank is increased by X \n(X is the difference between the revealed cards).\nPassive - Repercussions: Card affected by Neural gambit can increase their rank infinitely";
            case SkillName::FATALDEAL:      
            return "Reactive: When a card is drawn, you may spend 1 use, choose 1 card from your hand, \nswap that card with the drawn card\nPassive - Lethal revelation: When a card is drawn from the deck, you may see its rank";
            case SkillName::CHRONOSPHERE:   
            return "Active: During your hit phase, you may snapshot your hand, if you have a hand snapshot, you may spend 1 use,\n initiate [hand rewind] - return all cards from your hand back to the deck, and draw cards matching the rank of the snapshot.\nPassive - Timeline Seal: At the start of a normal player's hit phase, you may snapshot your hand.\nPassive - Temporal Rollbackk: When your hand value changes, you may snapshot your hand, if you have a hand snapshot, you may initiate a [hand rewind]";
            case SkillName::DESTINYDEFLECT: 
            return "Reactive: When a card is drawn, you may spend 1 use, add that card to another player's hand\nPassive - Clairvoyance: At the start of the round, you will see the top 3 cards of the deck";
            case SkillName::BOOGIEWOOGIE:   
            return "TODO";
            case SkillName::LOOKMAXXING:    
            return "TODO";
            case SkillName::SIGMA:          
            return "TODO";
            default:                        
            return "";
        }
    }

    void printAllInfo();

};

#endif