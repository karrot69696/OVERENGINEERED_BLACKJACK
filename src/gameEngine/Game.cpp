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
    // Server/Local: run game logic init. Client: skip — server owns game state.
    if (networkMode != NetworkMode::CLIENT) {
        roundManager.createSkills();
        roundManager.updateGameState(PhaseName::GAME_START_PHASE, 0);
        roundManager.changePhase(PhaseName::GAME_START_PHASE);
    }

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
    if (networkManager.getConnectedClientCount() == 0) return;

    // Broadcast current game state
    networkManager.broadcastGameState(gameState);

    // Peek events (non-destructive) and broadcast to clients
    // PresentationLayer will drain them afterwards for local rendering
    auto events = eventQueue.peekAll();
    if (!events.empty()) {
        std::cout << "[Server][broadcast] Sending " << events.size()
                  << " events to " << networkManager.getConnectedClientCount() << " client(s)" << std::endl;
        networkManager.broadcastEvents(events);
    }
}

void Game::clientReceive() {
    // Apply received game state
    if (networkManager.hasNewState()) {
        GameState& received = networkManager.consumeGameState();
        auto allInfo = received.getAllPlayerInfo();

        std::cout << "[Client][clientReceive] Got new GameState: phase="
                  << static_cast<int>(received.getPhaseName())
                  << " currentPlayer=" << received.getCurrentPlayerId()
                  << " players=" << allInfo.size() << std::endl;

        gameState.setAllPlayerInfo(allInfo);
        gameState.setPhaseName(received.getPhaseName(), received.getCurrentPlayerId());
        gameState.setDeckCount(received.getDeckCount());

        // Sync local Card*/Player objects from server's GameState
        syncLocalFromGameState();

        // Rebuild visual state from synced local objects
        visualState.rebuildFromState(deck, players);
    }

    // Push received events into local event queue for presentation
    auto events = networkManager.drainReceivedEvents();
    if (!events.empty()) {
        std::cout << "[Client][clientReceive] Got " << events.size() << " events from server" << std::endl;
    }
    for (auto& e : events) {
        eventQueue.push(std::move(e));
    }
}

void Game::syncLocalFromGameState() {
    auto allInfo = gameState.getAllPlayerInfo();

    std::cout << "[Client][syncLocalFromGameState] Syncing " << allInfo.size()
              << " players from server state" << std::endl;

    // Step 1: Reset all cards to unowned
    for (auto& cardPtr : allCards) {
        cardPtr->setOwnerId(-1);
        cardPtr->setHandIndex(-1);
    }

    // Step 2: Clear all player hands
    for (auto& player : players) {
        player.clearHand();
    }

    // Step 3: Assign cards to players based on server GameState
    for (const PlayerInfo& info : allInfo) {
        std::cout << "[Client][sync] Player " << info.playerId
                  << " has " << info.cardsInHand.size() << " cards" << std::endl;

        for (const Card& infoCard : info.cardsInHand) {
            // Find matching Card* in allCards by suit+rank (first unassigned match)
            bool found = false;
            for (auto& cardPtr : allCards) {
                if (cardPtr->getSuit() == infoCard.getSuit() &&
                    cardPtr->getRank() == infoCard.getRank() &&
                    cardPtr->getOwnerId() == -1) {

                    // Sync face-up state
                    if (infoCard.isFaceUp() != cardPtr->isFaceUp()) {
                        cardPtr->flip();
                    }

                    // Find local player and add card to hand
                    for (auto& player : players) {
                        if (player.getId() == info.playerId) {
                            player.addCardToHand(cardPtr.get());
                            found = true;
                            break;
                        }
                    }
                    break;
                }
            }
            if (!found) {
                std::cerr << "[Client][sync] WARNING: Could not find card "
                          << infoCard.getRankAsString() << infoCard.getSuitAsString()
                          << " for player " << info.playerId << std::endl;
            }
        }
    }

    // Step 4: Rebuild deck with remaining unowned cards
    deck.clearCards();
    for (auto& cardPtr : allCards) {
        if (cardPtr->getOwnerId() == -1) {
            deck.addCard(cardPtr.get());
        }
    }

    std::cout << "[Client][sync] Done. Deck has " << deck.getSize()
              << " cards remaining" << std::endl;
}
