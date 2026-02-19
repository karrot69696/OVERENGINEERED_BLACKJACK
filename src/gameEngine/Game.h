#ifndef GAME_H
#define GAME_H

#include <vector>
#include "../lowLevelEntities/Deck.h"
#include "../skillEngine/SkillManager.h"
#include "../lowLevelEntities/SkillDeck.h"
#include "../lowLevelEntities/Player.h"
#include "../lowLevelEntities/GameState.h"
#include <algorithm>
#include <memory>
#define maxNumPlayer 7
#define initialCardCount 2
class Game {
private:
    Deck deck;
    SkillDeck skillDeck;
    std::vector<Player> players;

    
public:
    void dealInitialCards(int numCards);  
    void displayPoints();
    void SetupGame();
    void RunGame();
};

class RoundManager {
private:
    std::vector<Player>& players;
    Deck& deck;
    SkillManager skillManager;
    GameState gameState;
public:
    RoundManager(std::vector<Player>& players, Deck& deck, SkillManager& skillManager, GameState gameState) 
        : players(players), deck(deck), skillManager(skillManager), gameState(gameState) {}
    void playRound(int roundNumber);
    void roundEndHandler();
    void dealCardsToPlayers(int numCards);  
    bool blackJackHandler();
    void battleHandler(Player& host, Player& opponent);
    void turnHandler(Player& player, Player& opponent);
    void startNewRound(std::vector<Player>& players);   
    void skillHandler(Player& player);
    void updateGameState(Phase phase, int playerId);
    Player &getHostPlayer();
    void createSkills();
    std::string phaseToString();
};
#endif