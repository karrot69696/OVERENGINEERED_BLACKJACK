#include "Phase.h"
#include "../gameEngine/Game.h"
#include "../gameEngine/UIManager.h"
#include "../networking/NetworkManager.h"
#include "../skillEngine/SkillManager.h"
Phase::Phase(UIManager& uiManager,
             EventQueue& eventQueue,
             RoundManager& roundManager,
             SkillManager& skillManager,
             GameState& gameState, VisualState& visualState)
    : uiManager(uiManager),
      eventQueue(eventQueue),
      roundManager(roundManager),
      skillManager(skillManager),
      gameState(gameState), visualState(visualState),
      deck(roundManager.getDeck()),
      players(roundManager.getPlayers())
{}

Player& Phase::getCurrentPlayer(){
    int currentPlayerIndex = gameState.getCurrentPlayerId();
    Player& currentPlayer = players[currentPlayerIndex];
    return currentPlayer;
}
int Phase::getCurrentPlayerId(){
    return gameState.getCurrentPlayerId();
}
void Phase::incrementCurrentPlayerId(){
    std::cout <<"[Phase] To next player..." << std::endl;
    gameState.incrementCurrentPlayerId((int)players.size());
}

bool Phase::turnHandler(Player& player, Player& opponent){

    // Multi-actor skill in progress — drive it and keep turn alive
    if (ngPending.has_value()) {
        ngTickPending(player);
        return true;
    }

    if(player._isBot()){
       player.setPendingAction(player.botMode(gameState));
    }
    // Remote player: pull action from NetworkManager
    else if (player.isRemote) {
        NetworkManager* net = roundManager.getNetworkManager();
        if (net && net->hasRemoteAction(player.getId())) {
            PlayerAction remoteAction = net->consumeRemoteAction(player.getId());
            player.setPendingAction(remoteAction);

            // If skill request, also check for targeting
            if (remoteAction == PlayerAction::SKILL_REQUEST && net->hasRemoteTarget(player.getId())) {
                gameState.pendingTarget = net->consumeRemoteTarget(player.getId());
            }
        }
    }

    switch (player.getPendingAction()){
        case PlayerAction::HIT: {
            Card* drawnCard = deck.draw();
            player.addCardToHand(drawnCard);

            // Emit event — PresentationLayer handles visual state + animation
            eventQueue.push({GameEventType::CARD_DRAWN, CardDrawnEvent{
                player.getId(),
                player.getHandSize() - 1,
                drawnCard->getId()
            }});

            roundManager.updateGameState(
                player.getHost() ? PhaseName::HOST_HIT_PHASE : PhaseName::PLAYER_HIT_PHASE,
                player.getHost() ? opponent.getId() : player.getId()
            );
            player.setPendingAction(PlayerAction::IDLE);
            // Re-prompt for next action after HIT
            eventQueue.push({GameEventType::REQUEST_ACTION_INPUT, RequestActionInputEvent{player.getId()}});
        } break;

        case PlayerAction::SKILL_REQUEST: {
            // For remote players, target may arrive in a different frame from the action
            if (player.isRemote && gameState.pendingTarget.targetPlayerIds.empty()
                    && gameState.pendingTarget.targetCards.empty()) {
                NetworkManager* net = roundManager.getNetworkManager();
                if (net && net->hasRemoteTarget(player.getId())) {
                    gameState.pendingTarget = net->consumeRemoteTarget(player.getId());
                }
            }
            // Once pendingTarget is populated, run skillHandler
            if (!gameState.pendingTarget.targetPlayerIds.empty() ||
                    !gameState.pendingTarget.targetCards.empty()) {
                std::cout << "[Phase][turnHandler] Processing skill for player " << player.getId() << std::endl;
                skillHandler(player);

                roundManager.updateGameState(
                    player.getHost() ? PhaseName::HOST_HIT_PHASE : PhaseName::PLAYER_HIT_PHASE,
                    player.getHost() ? opponent.getId() : player.getId()
                );
            }
        } break;

        case PlayerAction::STAND:
            roundManager.updateGameState(
                player.getHost() ? PhaseName::HOST_HIT_PHASE : PhaseName::PLAYER_HIT_PHASE,
                player.getHost() ? opponent.getId() : player.getId()
            );
            player.setPendingAction(PlayerAction::IDLE);
            return false;
        break;

        case PlayerAction::IDLE:
            // No-op: REQUEST_ACTION_INPUT is pushed once by onEnter or after HIT/skill
        break;
    }

    return true;
}

void Phase::skillHandler(Player& player){

    // NeuralGambit first step: player IDs only, no card picks yet → start multi-actor flow
    if (player.getSkillName() == SkillName::NEURALGAMBIT
            && !gameState.pendingTarget.targetPlayerIds.empty()
            && gameState.pendingTarget.targetCards.empty()) {
        ngPending = NgPendingState{};
        ngPending->skillUserId    = player.getId();
        ngPending->firstTargetId  = gameState.pendingTarget.targetPlayerIds[0];
        ngPending->secondTargetId = gameState.pendingTarget.targetPlayerIds[1];
        gameState.pendingTarget = {};
        player.setPendingAction(PlayerAction::IDLE);
        std::cout << "[skillHandler] NeuralGambit initiated for P" << player.getId()
                  << " targeting P" << ngPending->firstTargetId
                  << " and P" << ngPending->secondTargetId << std::endl;
        return;
    }

    std::vector<Player*> actualTargets;
    std::vector<Card*> actualTargetCards;

    for (int id : gameState.pendingTarget.targetPlayerIds) {
        for (auto& p : players) {
            if (p.getId() == id) {
                actualTargets.push_back(&p);
            }
        }
    }

    for (const Card& card : gameState.pendingTarget.targetCards) {
        bool found = false;
        for (auto& player : players) {
            for (int i = 0; i < player.getHandSize(); i++) {
                Card* playerCard = player.getCardInHand(i);
                if (playerCard->getId() == card.getId()) {
                    std::cout << "[skillHandler] Found target card: id="
                    << playerCard->getId() << " "
                    << playerCard->getRankAsString()
                    << playerCard->getSuitAsString()
                    << " in player "
                    << player.getId() << "'s hand." << std::endl;
                    actualTargetCards.push_back(playerCard);
                    found = true;
                    break;
                }
            }
            if (found) break;
        }
    }

    SkillContext context{
        player,
        actualTargets,
        actualTargetCards,
        deck,
        gameState,
        eventQueue
    };

    //It's jacking time      
    SkillExecutionResult skillResult = skillManager.processSkill(context);
    
    //process after math
    if( skillResult.name==SkillName::UNDEFINED ){
        std::cout << "[skillHandler] Skill processing failed for player " << player.getId() << std::endl;
        context.eventQueue.push(
            { GameEventType::SKILL_ERROR, 
                SkillErrorEvent{context.user.getId(), skillResult.errorMsg} }
        );
    } else {
        skillProcessAftermath(context, skillResult);
    }

    gameState.pendingTarget = {};
    eventQueue.push({GameEventType::REQUEST_ACTION_INPUT, RequestActionInputEvent{player.getId()}});
    player.setPendingAction(PlayerAction::IDLE);
}

void Phase::skillProcessAftermath(SkillContext& context, SkillExecutionResult skillResult){
    std::cout << "[skillHandler] Skill processing succeeded for player " << context.user.getId() << std::endl;
    switch (skillResult.name){
        //DELIVERANCE
        case SkillName::DELIVERANCE:
            for (auto* card : context.targetCards) {
                if (card->getOwnerId() == -1) {
                    // Emit spin event — PresentationLayer handles the full chain:
                    // deliverance effect + spin → return to deck → reposition
                    context.eventQueue.push({GameEventType::CARD_DELIVERANCE, CardSpinEvent{
                        card->getId(), context.user.getId()
                    }});
                    break;
                }
            }
        break;
        //NEURAL GAMBIT
        // targetCards: [0]=first revealed, [1]=second revealed, [2]=boost card
        case SkillName::NEURALGAMBIT:
            if (context.targetCards.size() >= 3) {
                context.eventQueue.push({GameEventType::NEURALGAMBIT_REVEAL,
                    NeuralGambitRevealEvent{
                        context.targetCards[0]->getId(),
                        context.targetCards[1]->getId(),
                        context.targetCards[2]->getId(),
                        context.targetCards[2]->getRankBonus()
                    }
                });
            }
        break;
    }
}

// ============================================================================
// Multi-actor skill orchestration (NeuralGambit and future cross-player skills)
// ============================================================================
void Phase::ngTickPending(Player& skillUser) {
    auto& ng = *ngPending;
    NetworkManager* net = roundManager.getNetworkManager();
    int localPlayerId = net ? net->getLocalPlayerId() : 0;

    auto isRemotePlayer = [&](const PlayerInfo& info) -> bool {
        return !info.isBot && net && net->isServer() && info.playerId != localPlayerId;
    };

    auto getHandCardIds = [&](int playerId) -> std::vector<int> {
        std::vector<int> ids;
        for (auto& p : players)
            if (p.getId() == playerId)
                for (int i = 0; i < p.getHandSize(); i++)
                    ids.push_back(p.getCardInHand(i)->getId());
        return ids;
    };

    auto autoPick = [&](int playerId) -> int {
        auto ids = getHandCardIds(playerId);
        if (ids.empty()) return -1;
        return ids[rand() % ids.size()];
    };

    auto findCard = [&](int cardId) -> Card* {
        for (auto& p : players)
            for (int i = 0; i < p.getHandSize(); i++) {
                Card* c = p.getCardInHand(i);
                if (c->getId() == cardId) return c;
            }
        return nullptr;
    };

    // --- Collect first target's card pick ---
    if (ng.firstCardId == -1) {
        if (!ng.firstRequestSent) {
            PlayerInfo firstTarget = gameState.getPlayerInfo(ng.firstTargetId);
            if (firstTarget.isBot) {
                ng.firstCardId = autoPick(ng.firstTargetId);
                std::cout << "[ngTickPending] First target bot P" << ng.firstTargetId
                          << " auto-picked card " << ng.firstCardId << std::endl;
            } else if (isRemotePlayer(firstTarget)) {
                net->sendTargetRequest(ng.firstTargetId,
                    TargetRequestData{ng.skillUserId, getHandCardIds(ng.firstTargetId), false});
                std::cout << "[ngTickPending] Sent pick request to remote P" << ng.firstTargetId << std::endl;
            } else {
                uiManager.requestPickCard(getHandCardIds(ng.firstTargetId));
                ng.waitingForLocalPick = true;
                std::cout << "[ngTickPending] Requested pick from local P" << ng.firstTargetId << std::endl;
            }
            ng.firstRequestSent = true;
        } else {
            PlayerInfo firstTarget = gameState.getPlayerInfo(ng.firstTargetId);
             if (isRemotePlayer(firstTarget) && net->hasRemoteTarget(ng.firstTargetId)) {
                auto t = net->consumeRemoteTarget(ng.firstTargetId);
                if (!t.targetCards.empty()) {
                    ng.firstCardId = t.targetCards[0].getId();
                    std::cout << "[ngTickPending] Remote P" << ng.firstTargetId
                              << " picked card " << ng.firstCardId << std::endl;
                }
            } else if (ng.waitingForLocalPick && !gameState.pendingTarget.targetCards.empty()) {
                ng.firstCardId = gameState.pendingTarget.targetCards[0].getId();
                ng.waitingForLocalPick = false;
                gameState.pendingTarget = {};
                std::cout << "[ngTickPending] Local P" << ng.firstTargetId
                          << " picked card " << ng.firstCardId << std::endl;
            }
        }
        return; // one step per frame
    }

    // --- Collect second target's card pick ---
    if (ng.secondCardId == -1) {
        if (!ng.secondRequestSent) {
            PlayerInfo secondTarget = gameState.getPlayerInfo(ng.secondTargetId);
            if (secondTarget.isBot) {
                ng.secondCardId = autoPick(ng.secondTargetId);
                std::cout << "[ngTickPending] Second target bot P" << ng.secondTargetId
                          << " auto-picked card " << ng.secondCardId << std::endl;
            } else if (isRemotePlayer(secondTarget)) {
                net->sendTargetRequest(ng.secondTargetId,
                    TargetRequestData{ng.skillUserId, getHandCardIds(ng.secondTargetId), false});
                std::cout << "[ngTickPending] Sent pick request to remote P" << ng.secondTargetId << std::endl;
            } else {
                uiManager.requestPickCard(getHandCardIds(ng.secondTargetId));
                ng.waitingForLocalPick = true;
                std::cout << "[ngTickPending] Requested pick from local P" << ng.secondTargetId << std::endl;
            }
            ng.secondRequestSent = true;
        } else {
            PlayerInfo secondTarget = gameState.getPlayerInfo(ng.secondTargetId);
            if (isRemotePlayer(secondTarget) && net->hasRemoteTarget(ng.secondTargetId)) {
                auto t = net->consumeRemoteTarget(ng.secondTargetId);
                if (!t.targetCards.empty()) {
                    ng.secondCardId = t.targetCards[0].getId();
                    std::cout << "[ngTickPending] Remote P" << ng.secondTargetId
                              << " picked card " << ng.secondCardId << std::endl;
                }
            } else if (ng.waitingForLocalPick && !gameState.pendingTarget.targetCards.empty()) {
                ng.secondCardId = gameState.pendingTarget.targetCards[0].getId();
                ng.waitingForLocalPick = false;
                gameState.pendingTarget = {};
                std::cout << "[ngTickPending] Local P" << ng.secondTargetId
                          << " picked card " << ng.secondCardId << std::endl;
            }
        }
        return;
    }

    // --- Both picks received: request boost pick ---
    if (!ng.boostRequested) {
        bool skillUserIsLocal = !net || !net->isServer() || ng.skillUserId == localPlayerId;
        if (skillUserIsLocal) {
            uiManager.requestBoostPickInput(ng.firstCardId, ng.secondCardId);
            std::cout << "[ngTickPending] Requesting boost pick from local P" << ng.skillUserId << std::endl;
        } else {
            net->sendTargetRequest(ng.skillUserId,
                TargetRequestData{-1, {ng.firstCardId, ng.secondCardId}, true});
            std::cout << "[ngTickPending] Sent boost pick request to remote P" << ng.skillUserId << std::endl;
        }
        ng.boostRequested = true;
        return;
    }

    // --- Boost pick received: execute skill ---
    int boostCardId = -1;
    bool skillUserIsLocal = !net || !net->isServer() || ng.skillUserId == localPlayerId;
    if (skillUserIsLocal && !gameState.pendingTarget.targetCards.empty()) {
        boostCardId = gameState.pendingTarget.targetCards[0].getId();
        gameState.pendingTarget = {};
    } else if (!skillUserIsLocal && net && net->hasRemoteTarget(ng.skillUserId)) {
        auto t = net->consumeRemoteTarget(ng.skillUserId);
        if (!t.targetCards.empty()) boostCardId = t.targetCards[0].getId();
    }
    if (boostCardId == -1) return; // still waiting

    Card* firstCard  = findCard(ng.firstCardId);
    Card* secondCard = findCard(ng.secondCardId);
    Card* boostCard  = findCard(boostCardId);
    if (!firstCard || !secondCard || !boostCard) {
        std::cerr << "[ngTickPending] ERROR: could not resolve all cards" << std::endl;
        ngPending = std::nullopt;
        return;
    }

    Player* skillUserPlayer = nullptr;
    for (auto& p : players) if (p.getId() == ng.skillUserId) { skillUserPlayer = &p; break; }
    if (!skillUserPlayer) { ngPending = std::nullopt; return; }

    std::vector<Player*> targetPlayers;
    for (auto& p : players)
        if (p.getId() == ng.firstTargetId || p.getId() == ng.secondTargetId)
            targetPlayers.push_back(&p);

    SkillContext context{
        *skillUserPlayer,
        targetPlayers,
        {firstCard, secondCard, boostCard},
        deck,
        gameState,
        eventQueue
    };

    SkillExecutionResult result = skillManager.processSkill(context);
    if (result.name == SkillName::UNDEFINED) {
        eventQueue.push({GameEventType::SKILL_ERROR,
            SkillErrorEvent{ng.skillUserId, result.errorMsg}});
    } else {
        skillProcessAftermath(context, result);
    }

    eventQueue.push({GameEventType::REQUEST_ACTION_INPUT,
        RequestActionInputEvent{ng.skillUserId}});
    skillUser.setPendingAction(PlayerAction::IDLE);
    ngPending = std::nullopt;
    std::cout << "[ngTickPending] NeuralGambit complete for P" << ng.skillUserId << std::endl;
}
