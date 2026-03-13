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
#include "EventQueue.h"
#include "PresentationLayer.h"
#include "../networking/NetworkManager.h"

#define maxNumPlayer 7
#define initialCardCount 2

class Game {
private:

    sf::RenderWindow& window;
    std::vector<std::unique_ptr<Card>> allCards;
    Deck deck;
    SkillDeck skillDeck;
    std::vector<Player> players;

    GameState gameState;
    VisualState visualState;

    UIManager uiManager;
    AnimationManager animationManager;
    EventQueue eventQueue;
    PresentationLayer presentationLayer;
    NetworkManager networkManager;
    NetworkMode networkMode = NetworkMode::LOCAL;
public:
    Game(sf::RenderWindow& window, NetworkMode mode = NetworkMode::LOCAL);
    ~Game(){}
    void SetupGame();
    void SetupGame(int numHumans, int numBots);
    void RunGame();
    void eventHandler(const std::optional<sf::Event>& event);

    // Networking
    bool startServer(uint16_t port = NET_DEFAULT_PORT);
    bool connectToServer(const std::string& ip, uint16_t port = NET_DEFAULT_PORT);
    NetworkManager& getNetworkManager() { return networkManager; }

private:
    void serverBroadcast();
    void clientReceive();
    void syncLocalFromGameState();  // client: sync Card*/Player hands from received GameState
};





#endif