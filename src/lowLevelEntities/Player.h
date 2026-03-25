#ifndef PLAYER_H
#define PLAYER_H

#include <iostream>
#include <random>
#include <algorithm>
#include <vector>
#include <memory>

#include "GameState.h"

// ============================================================================
// Constants and Configuration
// ============================================================================


class Player{
    protected:
        //basic properties
        int id;
        std::vector<Card*> cardsInHand;
        SkillName skillName;
        
        //game related properties
        bool isHost = 0;
        int point = 0;
        bool isBlackJacked = 0;
        int loss = 0;
        int battleCount = 0;
        PlayerAction pendingAction = PlayerAction::IDLE;
    public:
        std::vector<int> lastQuintetHand;
        std::vector<int> lastBlackjackHand;
        bool isBot = 1;
        bool isRemote = false;  // true for players whose input comes over network
        Player( int id, SkillName skillName, int isBot, int isHost) : skillName(skillName), isBot(isBot), isHost(isHost), id(id) {};
        virtual ~Player() = default;


        //getters
        Card* getCardInHand(int index) const {
            if(index >= (int)cardsInHand.size()){
                std::cout << "Invalid card index: " << index << std::endl;
                throw std::out_of_range("Card index out of range");
            }
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
        void setSkillName(SkillName name) { skillName = name; }
        int getLoss() const{
            return loss;
        }
        PlayerAction consumePendingAction() {
            PlayerAction temp = pendingAction;
            pendingAction = PlayerAction::IDLE; 
            return temp;
        }
        PlayerAction getPendingAction() const {
            return pendingAction;
        }
        void setPendingAction(PlayerAction action){
            pendingAction = action;
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
            return (int)cardsInHand.size();
        }
        int getHost() const{
            return isHost;
        }
        int getPoint() const{
            return point;
        }
        // setters
        void setHost(){
            isHost = 1;
        }
        void resetHost(){
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
        void returnCards(Deck& deck, const std::vector<Card*>& cards, bool toBotDeck = false);
        void addCardToHand(Card* card, bool isSync = false);
        void flipAllCardsFaceUp();
        void clearHand() { cardsInHand.clear(); }  // for network sync (doesn't touch card ownership)

        // Remove a card from hand by ID without returning to deck (for direct swaps)
        Card* removeCardFromHand(int cardId) {
            for (auto it = cardsInHand.begin(); it != cardsInHand.end(); ++it) {
                if ((*it)->getId() == cardId) {
                    Card* card = *it;
                    card->setOwnerId(-1);
                    card->setHandIndex(-1);
                    cardsInHand.erase(it);
                    // Re-index remaining cards so handIndex stays sequential
                    for (int i = 0; i < (int)cardsInHand.size(); i++) {
                        cardsInHand[i]->setHandIndex(i);
                    }
                    return card;
                }
            }
            return nullptr;
        }


        
        //bot logic - run when it's bot's turn
        PlayerAction hostTurnLogic(GameState& state, Player& opponent, int hostLoseStreak);
        PlayerAction botMode(GameState& state);


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