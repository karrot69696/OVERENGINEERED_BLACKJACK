#ifndef PLAYER_H
#define PLAYER_H

#include <vector>
#include <memory>
#include "Card.h"
#include "Deck.h"
#include "SkillDeck.h"
#include <iostream>
#include "GameState.h"
#include <random>
#include <algorithm>
// ============================================================================
// Constants and Configuration
// ============================================================================

namespace GameConfig {
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
enum class PlayerAction{
    HIT,
    SKILL_REQUEST,
    STAND,
    IDLE
};
struct PlayerTargeting{
    std::vector<int> targetPLayerIds;
    std::vector<Card*> targetCards;
};
class Player{
    protected:
        //basic properties
        int id;
        std::vector<Card> cardsInHand;
        SkillName skillName;
        
        //game related properties
        bool isHost = 0;
        int point = 0;
        bool isBlackJacked = 0;
        int loss = 0;
        int battleCount = 0;

    public:
        bool isBot = 1;
        Player( int id, SkillName skillName, int isBot, int isHost) : skillName(skillName), isBot(isBot), isHost(isHost), id(id) {};
        virtual ~Player() = default;


        //getters
        Card getCardInHand(int index){
            return cardsInHand[index];
        }
        void blackJackSet(bool status = 1){
            isBlackJacked = status;
        }
        bool getBlackJacked(){
            return isBlackJacked;
        }
        SkillName getSkillName(){
            return skillName;
        }
        int getLoss() const{
            return loss;
        }
        int getBattleCount() const{
            return battleCount;
        }
        int getId() const{
            return id;
        }
        int _isBot() const{
            return isBot;
        }
        int getHandSize() const{
            return cardsInHand.size();
        }
        int getHost() const{
            return isHost;
        }
        int getPoint() const{
            return point;
        }

        // setters
        void giveHost(){
            isHost = 1;
        }
        void removeHost(){
            isHost = 0;
        }
        void gainPoint(int pointsGained = 1){
            point += pointsGained;
        }
        void gainLoss(int lossGained = 1){
            loss += lossGained;
        }
        void gainBattleCount(int games=1){
            battleCount += games;
        }

        //hand manipulation
        void Stand();
        void returnCards(Deck& deck);
        void returnCards(Deck& deck, const std::vector<Card*>& cards);
        void addCardToHand(const Card& card){
            cardsInHand.push_back(card);
        }
        void flipAllCardsFaceUp();


        
        //heavy logic

        PlayerAction playerTurnLogic(GameState& state);
        PlayerAction hostTurnLogic(GameState& state, Player& opponent, int hostLoseStreak);
        
        PlayerAction manualMode(GameState& state);
        PlayerAction botMode(GameState& state);

        PlayerTargeting targetAndConfirm(GameState& state);
        PlayerTargeting skillTarget_Deliverance(GameState& gameState);
        PlayerTargeting skillTarget_NeuralGambit(GameState& gameState);


        //logic helpers
        int calculateHandValue() const;
        double burstRiskCalculation();
        bool shouldTakeRisk(double risk) const;
        bool shouldHostHit(Player& opponent, double losingPressure);
        int calculateWiningStatusOfAllPlayers(){
            //a metric to determine how well a player is doing in the game
            //consider hand value, number of cards, skill used, skill available, etc.
            return 0;
        }

        // print functions
        char getValidChoice() const;
        void displayManualMenu() const;
        void displayGameState(GameState& state) const;
        void printCardsInHand();
        std::string skillNameToString();

};

#endif