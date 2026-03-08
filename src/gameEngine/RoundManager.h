#ifndef ROUNDMANAGER_H
#define ROUNDMANAGER_H

#include <algorithm>
#include <memory>
#include <vector>
#include <optional>

#include "../skillEngine/SkillManager.h"

#include "../lowLevelEntities/Player.h"

#include "../phaseEngine/Phase.h"
#include "../phaseEngine/BlackJackCheckPhase.h"
#include "../phaseEngine/PlayerHitPhase.h"
#include "../phaseEngine/HostHitPhase.h"
#include "../phaseEngine/BattlePhase.h"
#include "../phaseEngine/RoundEndPhase.h"

#include "UIManager.h"

class Game;

class RoundManager {
private:
    std::vector<Player>& players;
    Deck& deck;
    SkillManager skillManager;
    GameState& gameState;
    UIManager& uiManager;

    //NEW PHASE SYSTEM
    std::unique_ptr<Phase> currentPhase; 
    int round = 0;
public:


    RoundManager(std::vector<Player>& players, Deck& deck, SkillManager& skillManager, GameState& _gameState, UIManager& _uiManager) 
        : players(players), deck(deck), skillManager(skillManager), gameState(_gameState), uiManager(_uiManager) {}
    //getters
    Deck& getDeck(){return deck;}
    SkillManager& getSkillManager(){return skillManager;}
    int getRound(){return round;}

    //setters
    void incrementRound(){round++;}

    //EXTREMELY IMPORTANT STATE UPDATE
    void updateGameState(PhaseName phase, int playerId);

    //helper functions
    void dealCardsToPlayers(int numCards);  
    Player &getHostPlayer();
    std::vector<Player>& getPlayers(){return players;}
    Player& getPlayerById(int id);
    GameState& getGameState(){return gameState;}
    void createSkills();
    std::string phaseToString();

    //NEW PHASE SYSTEM
    void update();
    void changePhase(PhaseName newPhase);
    std::unique_ptr<Phase> createPhase(const PhaseName name);

};

#endif