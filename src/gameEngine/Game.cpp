#include "Game.h"

// ============================================================================
// Game Implementation
// ============================================================================
Game::Game(sf::RenderWindow& window, NetworkMode mode)
    :
        window(window),
        gameState(), deck(), players(), skillDeck(),
        visualState(this->window, this->gameState),
        uiManager(this->window, this->gameState, this->visualState, this->visualState.getCardVisuals()),
        animationManager(this->window, this->gameState, this->visualState),
        eventQueue(),
        presentationLayer(this->eventQueue, this->animationManager, this->uiManager, this->visualState, this->gameState),
        networkManager(),
        networkMode(mode)
{
    //initialize card container
    for (int s = 0; s < 4; s++) {
        for ( int r = 1; r <= 13; r++) {
            this->allCards.push_back(std::make_unique<Card>(
                static_cast<Suit>(s),
                static_cast<Rank>(r), false
            ));
        }
    }
    //initialize deck
    for (size_t i=0;i<allCards.size();i++){
        Card* card = allCards[i].get();
        card->setId(static_cast<int>(i)); //every card has a unique ID
        deck.addCard(card);
    }

    visualState.buildCardVisuals(deck, players);

}

void Game::SetupGame(){
    SetupGame(1, 2); // default: 1 human, 2 bots
}

void Game::SetupGame(int numHumans, int numBots){

    int numPlayers = numHumans + numBots;
    if (numPlayers <= 1 || numBots < 0 ) {
        std::cout << "Invalid input. Using defaults (2 bots, 1 players)." << std::endl;
        numPlayers = 3;
        numBots = 2;
        numHumans = 1;
    }
    else{
        numPlayers = (numPlayers>maxNumPlayer)? maxNumPlayer : numPlayers;
        numBots = (numBots>numPlayers)? numPlayers: numBots;
    }

    //create human players
    for (int i = 0; i < numHumans; i++){
        SkillName newSkill = skillDeck.drawSkill();
        std::cout<< "Player " << i << " created." << std::endl;
        players.emplace_back( i ,newSkill, 0, 0);
    }
    //create bots
    for (int i = numHumans; i < numPlayers; i++){
        std::cout<< "Player (bot) " << i << " created." << std::endl;
        SkillName newSkill = skillDeck.drawSkill();
        players.emplace_back( i ,newSkill, 1, 0);
    }

    //choose host (dealer)
    players.front().setHost();
    std::cout << "Player " << players.front().getId() << " is the host." << std::endl;
}

void Game::RunGame(){
    std::cout << "\n=== GAME START ===\n" << std::endl;
    RoundManager roundManager(
        this->players,
        this->deck,
        this->gameState,
        this->visualState,
        this->uiManager,
        this->eventQueue,
        (networkMode != NetworkMode::LOCAL) ? &this->networkManager : nullptr
    );
    roundManager.createSkills();
    roundManager.updateGameState(PhaseName::GAME_START_PHASE, 0);
    roundManager.changePhase(PhaseName::GAME_START_PHASE);

    // Client mode: override UIManager callbacks to send input over network
    if (networkMode == NetworkMode::CLIENT) {
        uiManager.onActionChosen = [this](PlayerAction action) {
            std::cout << "[Client] Sending action: " << static_cast<int>(action) << std::endl;
            networkManager.sendAction(action);
        };
        uiManager.onTargetChosen = [this](PlayerTargeting targeting) {
            std::cout << "[Client] Sending targeting" << std::endl;
            networkManager.sendTarget(targeting);
        };
    }

    sf::Clock clock;

    while (window.isOpen()){

        float deltaTime = clock.restart().asSeconds();

        while (std::optional event = this->window.pollEvent()){
            eventHandler(event);
        }

        // Poll network
        networkManager.poll();

        if (networkMode == NetworkMode::CLIENT) {
            // Client: apply state + events from server
            clientReceive();
        }

        // Only advance game logic when no animations are playing
        // Server and LOCAL run game logic; client does NOT
        if (networkMode != NetworkMode::CLIENT) {
            if (!animationManager.playing()) {
                roundManager.update();
            }
        }

        if (networkMode == NetworkMode::HOST) {
            // Server: broadcast state + events after logic tick
            serverBroadcast();
        }

        // Drain event queue → trigger animations/UI
        presentationLayer.processEvents();
        animationManager.update(deltaTime);

        window.clear();

        uiManager.render();
        animationManager.render();
        window.display();

    }

    // Cleanup
    networkManager.shutdown();
    players.clear();
    deck.clearCards();
    skillDeck.clearCards();

}

void Game::eventHandler(const std::optional<sf::Event>& event){

    if ( event->is<sf::Event::Closed>()){
        window.close();
    }
    else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()){
        if (keyPressed->scancode == sf::Keyboard::Scancode::Escape){
            window.close();
        }
    }
    uiManager.handleEvent(event);
}

// ============================================================================
// Networking
// ============================================================================

bool Game::startServer(uint16_t port) {
    return networkManager.initServer(port);
}

bool Game::connectToServer(const std::string& ip, uint16_t port) {
    return networkManager.initClient(ip, port);
}

void Game::serverBroadcast() {
    // Broadcast current game state
    networkManager.broadcastGameState(gameState);

    // Drain the event queue and broadcast events, then re-push for local presentation
    // We peek at events that were already pushed to the queue
    // Actually, events are consumed by PresentationLayer, so we broadcast BEFORE draining
    // The events are still in the queue at this point — we need to serialize them
    // Better approach: capture events as they're pushed. For now, broadcast state only.
    // Events will be added in a follow-up when we hook into EventQueue.
}

void Game::clientReceive() {
    // Apply received game state
    if (networkManager.hasNewState()) {
        GameState& received = networkManager.consumeGameState();
        // Update local game state from server
        auto allInfo = received.getAllPlayerInfo();
        gameState.setAllPlayerInfo(allInfo);
        gameState.setPhaseName(received.getPhaseName(), received.getCurrentPlayerId());
    }

    // Push received events into local event queue for presentation
    auto events = networkManager.drainReceivedEvents();
    for (auto& e : events) {
        eventQueue.push(std::move(e));
    }
}
