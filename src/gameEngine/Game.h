#ifndef GAME_H
#define GAME_H

#include <algorithm>
#include <memory>
#include <vector>
#include <optional>

#include "../skillEngine/SkillManager.h"

#include "../lowLevelEntities/Player.h"

#include "../phaseEngine/Phase.h"
#include "../phaseEngine/BlackJackCheckPhase.h"


#include "RoundManager.h"
#include "UIManager.h"
#include "AnimationManager.h"

#define maxNumPlayer 7
#define initialCardCount 2

class Game {
private:
    sf::RenderWindow& window;
    Deck deck;
    SkillDeck skillDeck;
    std::vector<Player> players; 
    GameState& gameState;
    UIManager uiManager;
    AnimationManager animationManager;
public:
    Game(sf::RenderWindow& window, GameState& _gameState) 
        : window(window), gameState(_gameState), uiManager(window, gameState), animationManager(window, gameState) {}
    ~Game(){}
    void dealInitialCards(int numCards);  
    void displayPoints();
    void SetupGame();
    void RunGame();
    void eventHandler(const std::optional<sf::Event>& event);
};





#endif