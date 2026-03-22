#include "Game.h"
#include "Log.h"

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

void Game::SetupGame(int numLocal, int numRemote, int numBots){
    int numPlayers = numLocal + numRemote + numBots;
    if (numPlayers <= 1 || numBots < 0) {
        std::cout << "Invalid input. Using defaults." << std::endl;
        SetupGame(1, 2);
        return;
    }
    numPlayers = std::min(numPlayers, maxNumPlayer);

    // Create local human players
    for (int i = 0; i < numLocal; i++){
        SkillName newSkill = skillDeck.drawSkill();
        std::cout << "Player " << i << " created (local)." << std::endl;
        players.emplace_back(i, newSkill, 0, 0);
    }
    // Create remote human players
    for (int i = numLocal; i < numLocal + numRemote; i++){
        SkillName newSkill = skillDeck.drawSkill();
        std::cout << "Player " << i << " created (remote)." << std::endl;
        players.emplace_back(i, newSkill, 0, 0);
        players.back().isRemote = true;
    }
    // Create bots
    for (int i = numLocal + numRemote; i < numPlayers; i++){
        SkillName newSkill = skillDeck.drawSkill();
        std::cout << "Player (bot) " << i << " created." << std::endl;
        players.emplace_back(i, newSkill, 1, 0);
    }

    //choose host (dealer)
    players.front().setHost();
    std::cout << "Player " << players.front().getId() << " is the host." << std::endl;
}

void Game::RunGame(){
    TimestampBuf::resetClock();
    visualState.setLocalPlayerId(networkManager.getLocalPlayerId());
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

    // Wire input callbacks ONCE — logic validates, then tells UI what to show
    if (networkMode == NetworkMode::CLIENT) {
        uiManager.onActionChosen = [this](PlayerAction action) {
            std::cout << "[Client] Sending action: " << static_cast<int>(action) << std::endl;
            networkManager.sendAction(action);
            if (action == PlayerAction::SKILL_REQUEST) {
                // Keep action menu visible until server responds with either
                // REQUEST_TARGET_INPUT (valid) or SKILL_ERROR + REQUEST_ACTION_INPUT (invalid)
                int activeId = uiManager.getActivePlayerId();
                uiManager.requestActionInput(activeId);
            }
        };
        uiManager.onTargetChosen = [this](PlayerTargeting targeting) {
            if (!targeting.targetPlayerIds.empty() || !targeting.targetCards.empty()) {
                std::cout << "[Client] Sending targeting" << std::endl;
                networkManager.sendTarget(targeting);
                return;
            }
            // Cancel — re-show action menu locally
            int activeId = uiManager.getActivePlayerId();
            eventQueue.push({GameEventType::REQUEST_ACTION_INPUT,
                RequestActionInputEvent{activeId}});
        };
        uiManager.onReactiveResponse = [this](bool accepted) {
            networkManager.sendReactiveResponse(accepted);
        };
    } else {
        // HOST / LOCAL — logic layer validates before showing targeting UI
        uiManager.onActionChosen = [this, &roundManager](PlayerAction action) {
            int activeId = uiManager.getActivePlayerId();
            Player* active = nullptr;
            for (auto& p : players) {
                if (p.getId() == activeId) { active = &p; break; }
            }
            if (!active) return;

            if (action == PlayerAction::SKILL_REQUEST) {
                // Pre-validate: check uses, phase, hand — before showing targeting
                std::string error = roundManager.getSkillManager()
                    .preValidateSkill(activeId, gameState);
                if (!error.empty()) {
                    std::cout << "[Game] preValidateSkill failed for P" << activeId
                              << ": " << error << std::endl;
                    eventQueue.push({GameEventType::SKILL_ERROR,
                        SkillErrorEvent{activeId, error}});
                    // Directly re-show action menu (event queue may be blocked by cutscene)
                    uiManager.requestActionInput(activeId);
                    return;
                }
                active->setPendingAction(action);
                uiManager.requestTargetInput(activeId);
            } else {
                active->setPendingAction(action);
            }
        };
        uiManager.onTargetChosen = [this](PlayerTargeting targeting) {
            // Data path: targeting has content → store for logic layer (ngTickPending / skillHandler)
            // No player lookup needed — gameState.pendingTarget is read by whoever requested it
            if (!targeting.targetPlayerIds.empty() || !targeting.targetCards.empty()) {
                gameState.pendingTarget = targeting;
                return;
            }
            // Cancel path: empty targeting → re-show action menu for the active player
            int activeId = uiManager.getActivePlayerId();
            Player* active = nullptr;
            for (auto& p : players) {
                if (p.getId() == activeId) { active = &p; break; }
            }
            if (!active) return;
            eventQueue.push({GameEventType::REQUEST_ACTION_INPUT,
                RequestActionInputEvent{activeId}});
            active->setPendingAction(PlayerAction::IDLE);
        };
        uiManager.onReactiveResponse = [this](bool accepted) {
            gameState.pendingReactiveResponse = accepted ? ReactiveResponse::YES : ReactiveResponse::NO;
        };
    }

    // Fixed design resolution — content scales with letterboxing on resize
    sf::View gameView({0.f, 0.f}, {(float)window.getSize().x, (float)window.getSize().y});
    gameView.setCenter({window.getSize().x / 2.f, window.getSize().y / 2.f});
    window.setView(gameView);

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
                // Snapshot event queue size BEFORE logic runs
                size_t eventsBefore = eventQueue.size();
                roundManager.update();

                if (networkMode == NetworkMode::HOST) {
                    // Broadcast only NEW events produced by this update
                    serverBroadcast(eventsBefore);
                }
            }
        }

        // Drain event queue → trigger animations/UI
        presentationLayer.processEvents();
        animationManager.update(deltaTime);
        visualState.enforceVisibility();

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
    else if (const auto* resized = event->getIf<sf::Event::Resized>()) {
        // Letterbox: maintain aspect ratio, black bars fill the gap
        float designW = (float)window.getSize().x;
        float designH = (float)window.getSize().y;
        float windowW = (float)resized->size.x;
        float windowH = (float)resized->size.y;
        float designRatio = designW / designH;
        float windowRatio = windowW / windowH;

        sf::FloatRect viewport;
        if (windowRatio > designRatio) {
            // Window wider than design → pillarbox (bars on sides)
            float viewW = designRatio / windowRatio;
            viewport = {{(1.f - viewW) / 2.f, 0.f}, {viewW, 1.f}};
        } else {
            // Window taller than design → letterbox (bars top/bottom)
            float viewH = windowRatio / designRatio;
            viewport = {{0.f, (1.f - viewH) / 2.f}, {1.f, viewH}};
        }

        sf::View view({0.f, 0.f}, {designW, designH});
        view.setCenter({designW / 2.f, designH / 2.f});
        view.setViewport(viewport);
        window.setView(view);
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

void Game::serverBroadcast(size_t eventsBefore) {
    if (networkManager.getConnectedClientCount() == 0) return;

    // Broadcast current game state
    networkManager.broadcastGameState(gameState);

    // Broadcast only events that were added AFTER the snapshot
    // Events stay in the queue for local PresentationLayer to consume — no drain/re-push
    auto newEvents = eventQueue.getEventsFrom(eventsBefore);
    if (!newEvents.empty()) {
        std::cout << "[Server][broadcast] Sending " << newEvents.size()
                  << " new events to " << networkManager.getConnectedClientCount() << " client(s)" << std::endl;
        networkManager.broadcastEvents(newEvents);
    }
}

void Game::clientReceive() {
    // Apply received game state
    if (networkManager.hasNewState()) {
        GameState& received = networkManager.consumeGameState();
        auto allInfo = received.getAllPlayerInfo();

        PhaseName oldPhase = gameState.getPhaseName();
        int oldPlayerId = gameState.getCurrentPlayerId();
        gameState.setAllPlayerInfo(allInfo);
        gameState.setPhaseName(received.getPhaseName(), received.getCurrentPlayerId());
        gameState.setDeckCards(received.getDeckCards());

        // Log phase/turn changes (not every frame)
        if (received.getPhaseName() != oldPhase || received.getCurrentPlayerId() != oldPlayerId) {
            std::cout << "[Client] State changed: phase=" << gameState.phaseToString(received.getPhaseName())
                      << " currentPlayer=" << received.getCurrentPlayerId()
                      << " deck=" << received.getDeckCards().size() << std::endl;
            for (const auto& info : allInfo) {
                std::cout << "  P" << info.playerId << ": " << info.cardsInHand.size() << " cards [";
                for (const auto& c : info.cardsInHand) {
                    std::cout << " id=" << c.getId() << (c.isFaceUp() ? "↑" : "↓");
                }
                std::cout << " ] pts=" << info.points << std::endl;
            }
        }

        // Scale players to match server if count differs
        if ((int)players.size() != (int)allInfo.size()) {
            std::cout << "[Client] Player count mismatch: local=" << players.size()
                      << " server=" << allInfo.size() << " → rebuilding" << std::endl;
            players.clear();
            for (const auto& info : allInfo) {
                players.emplace_back(info.playerId, info.skill, 1, 0); // all remote bots by default
            }
            players.front().setHost();
        }

        // Sync local Card*/Player objects from server's GameState
        syncLocalFromGameState();

        if (!clientInitialBuild) {
            // First GameState — full visual build (bootstrapping)
            visualState.rebuildFromState(deck, players);
            clientInitialBuild = true;
            std::cout << "[Client] Initial visual build complete" << std::endl;
        } else {
            // Reconcile: updates ownership, cardIndex, location, rankBonus.
            // Does NOT touch faceUp — enforceVisibility() owns that.
            visualState.reconcile(gameState);
        }

        auto events = networkManager.drainReceivedEvents();
        if (!events.empty()) {
            std::cout << "[Client][clientReceive] Got " << events.size() << " events from server" << std::endl;
            visualState.setReconcileBlocked(true);
        }

        // Pre-pin CARDS_REVEALED cards so enforceVisibility doesn't flip them
        // face-up while they're stuck behind a cutscene in the queue.
        for (const auto& e : events) {
            if (e.type == GameEventType::CARDS_REVEALED) {
                const auto& data = std::get<CardsRevealedEvent>(e.data);
                for (int cardId : data.cardIds) {
                    visualState.getCardVisual(cardId).pin();
                }
            }
        }

        // Push events into local event queue for presentation
        int myId = networkManager.getLocalPlayerId();
        for (auto& e : events) {
            // Only show action/target input for this client's player
            if (e.type == GameEventType::REQUEST_ACTION_INPUT) {
                auto& data = std::get<RequestActionInputEvent>(e.data);
                if (data.playerId != myId) continue;
            }
            if (e.type == GameEventType::REQUEST_TARGET_INPUT) {
                auto& data = std::get<RequestTargetInputEvent>(e.data);
                if (data.playerId != myId) continue;
            }
            eventQueue.push(std::move(e));
        }
    }

    // Route target-pick requests from server (multi-actor skills like NeuralGambit)
    if (networkManager.hasPendingTargetRequest()) {
        TargetRequestData req = networkManager.consumePendingTargetRequest();
        if (req.isBoostPick) {
            if (req.allowedCardIds.size() >= 2)
                uiManager.requestBoostPickInput(req.allowedCardIds[0], req.allowedCardIds[1]);
        } else {
            uiManager.requestPickCard(req.allowedCardIds);
        }
    }

    // Route reactive skill prompts from server
    if (networkManager.hasPendingReactivePrompt()) {
        ReactivePromptData prompt = networkManager.consumePendingReactivePrompt();
        uiManager.requestReactivePrompt(
            gameState.skillNameToString(prompt.skill), prompt.extraInfo
            ,prompt.timerDuration);
    }
}

void Game::syncLocalFromGameState() {
    auto allInfo = gameState.getAllPlayerInfo();

    // No per-frame log — too noisy

    // Step 1: Reset all cards to unowned
    for (auto& cardPtr : allCards) {
        cardPtr->setOwnerId(-1);
        cardPtr->setHandIndex(-1);
    }

    // Step 2: Clear all player hands
    for (auto& player : players) {
        player.clearHand();
    }

    // Step 3: Assign cards to players based on server GameState (match by cardId)
    for (const PlayerInfo& info : allInfo) {
        for (const Card& infoCard : info.cardsInHand) {
            bool found = false;
            for (auto& cardPtr : allCards) {
                if (cardPtr->getId() == infoCard.getId()) {
                    // Sync face-up state
                    if (infoCard.isFaceUp() != cardPtr->isFaceUp()) {
                        cardPtr->flip();
                    }
                    // Find local player and add card to hand
                    for (auto& player : players) {
                        if (player.getId() == info.playerId) {
                            player.addCardToHand(cardPtr.get(), true);
                            found = true;
                            break;
                        }
                    }
                    break;
                }
            }
            if (!found) {
                std::cerr << "[Client][sync] WARNING: Could not find cardId="
                          << infoCard.getId() << " for player " << info.playerId << std::endl;
            }
        }
    }

    // Step 4: Rebuild deck from GameState's deck cards (by cardId)
    deck.clearCards();
    for (const Card& deckCard : gameState.getDeckCards()) {
        for (auto& cardPtr : allCards) {
            if (cardPtr->getId() == deckCard.getId()) {
                if (deckCard.isFaceUp() != cardPtr->isFaceUp()) {
                    cardPtr->flip();
                }
                deck.addCard(cardPtr.get());
                break;
            }
        }
    }

    // Log sync summary (only when something interesting happened)
    static int lastDeckSize = -1;
    static std::vector<int> lastHandSizes;
    bool changed = (int)deck.getCards().size() != lastDeckSize;
    std::vector<int> handSizes;
    for (auto& p : players) {
        handSizes.push_back(p.getHandSize());
    }
    if (handSizes != lastHandSizes) changed = true;
    if (changed) {
        std::cout << "[Sync] deck=" << deck.getCards().size();
        for (auto& p : players) {
            std::cout << " P" << p.getId() << "=" << p.getHandSize() << "cards";
        }
        std::cout << std::endl;
        lastDeckSize = (int)deck.getCards().size();
        lastHandSizes = handSizes;
    }
}
