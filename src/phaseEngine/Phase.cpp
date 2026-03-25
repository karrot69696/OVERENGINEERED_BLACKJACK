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
bool Phase::allPlayersProcessed(){
    return gameState.allPlayersProcessed((int)players.size());
}

bool Phase::turnHandler(Player& player, Player& opponent){

    // Multi-actor skill in progress — drive it and keep turn alive
    if (ngPending.has_value()) {
        ngTickPending(player);
        return true;
    }

    // Reactive skill check in progress — drive it and keep turn alive
    if (reactiveCheck.has_value()) {
        reactiveTickPending();
        return true;
    }

    // Chronosphere pending — drive choice prompt and keep turn alive
    if (chronoPending.has_value()) {
        chronoTickPending(player);
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

            // Server-side preValidation for remote skill requests
            if (remoteAction == PlayerAction::SKILL_REQUEST) {
                std::string error = skillManager.preValidateSkill(player.getId(), gameState);
                if (!error.empty()) {
                    std::cout << "[turnHandler] Remote P" << player.getId()
                              << " skill rejected: " << error << std::endl;
                    eventQueue.push({GameEventType::SKILL_ERROR,
                        SkillErrorEvent{player.getId(), error}});
                    eventQueue.push({GameEventType::REQUEST_ACTION_INPUT,
                        RequestActionInputEvent{player.getId()}});
                    return true; // stay alive, don't set pendingAction
                }
                // Valid: tell client to show targeting overlay
                std::cout << "[turnHandler] Remote P" << player.getId()
                          << " skill validated, sending REQUEST_TARGET_INPUT" << std::endl;
                eventQueue.push({GameEventType::REQUEST_TARGET_INPUT,
                    RequestTargetInputEvent{player.getId()}});
            }

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

            // Check reactive skills before re-prompting
            if (startReactiveCheck(ReactiveTrigger::ON_CARD_DRAWN,
                    drawnCard->getId(), player.getId(), player.getId())) {
                break;  // reactiveTickPending will push REQUEST_ACTION_INPUT when done
            }
            blackJackAndQuintetCheck(players);

            // Check Chronosphere hand-value-change trigger before re-prompting
            if (tryChronoReactiveCheck(player.getId())) {
                break; // chronoTickPending will push REQUEST_ACTION_INPUT when done
            }

            // Re-prompt for next action after HIT
            eventQueue.push({GameEventType::REQUEST_ACTION_INPUT, RequestActionInputEvent{player.getId()}});
        } break;

        case PlayerAction::SKILL_REQUEST: {
            // Chronosphere: doesn't use pendingTarget, uses chronoPending instead
            if (player.getSkillName() == SkillName::CHRONOSPHERE
                    && gameState.pendingTarget.targetPlayerIds.empty()
                    && gameState.pendingTarget.targetCards.empty()) {
                chronoPending = ChronoPendingState{};
                chronoPending->skillUserId = player.getId();
                chronoPending->actingPlayerId = player.getId();
                player.setPendingAction(PlayerAction::IDLE);
                std::cout << "[turnHandler] Chronosphere active use initiated for P" << player.getId() << std::endl;
                break;
            }

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
    player.setPendingAction(PlayerAction::IDLE);

    // Check Chronosphere hand-value-change trigger before re-prompting
    if (!tryChronoReactiveCheck(player.getId())) {
        eventQueue.push({GameEventType::REQUEST_ACTION_INPUT, RequestActionInputEvent{player.getId()}});
    }
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
        //FATAL DEAL
        case SkillName::FATALDEAL:
            if (context.targetCards.size() >= 2) {
                context.eventQueue.push({GameEventType::FATALDEAL_SWAP,
                    FatalDealSwapEvent{
                        context.targetPlayers[0]->getId(),  // drawer
                        context.user.getId(),               // FD user
                        context.targetCards[0]->getId(),     // drawn card
                        context.targetCards[1]->getId()      // swapped card
                    }
                });
            }
        break;
        //DESTINY DEFLECT
        case SkillName::DESTINYDEFLECT:
            if (!context.targetPlayers.empty() && context.targetPlayers.size() >= 2 && !context.targetCards.empty()) {
                context.eventQueue.push({GameEventType::CARD_REDIRECTED, CardRedirectedEvent{
                    context.targetCards[0]->getId(),
                    context.targetPlayers[0]->getId(),  // drawer (from)
                    context.targetPlayers[1]->getId()    // redirect target (to)
                }});
            }
        break;
        //CHRONOSPHERE
        case SkillName::CHRONOSPHERE:
            if (context.state.pendingChronoChoice == ChronoChoice::SNAPSHOT) {
                context.eventQueue.push({GameEventType::CHRONOSPHERE_SNAPSHOT,
                    ChronosphereSnapshotEvent{context.user.getId()}});
            } else if (context.state.pendingChronoChoice == ChronoChoice::REWIND) {
                context.eventQueue.push({GameEventType::CHRONOSPHERE_REWIND,
                    ChronosphereRewindEvent{context.user.getId()}});
                // Animate old cards returning to deck
                for (int cardId : context.returnedCardIds) {
                    context.eventQueue.push({GameEventType::CARD_RETURNED,
                        CardReturnedEvent{cardId}});
                }
                // Animate all redrawn cards at once (no reactive trigger)
                {
                    CardRedrawnEvent redrawn;
                    redrawn.playerId = context.user.getId();
                    for (int i = 0; i < (int)context.drawnCardIds.size(); i++) {
                        redrawn.cardIds.push_back(context.drawnCardIds[i]);
                        redrawn.handIndices.push_back(i);
                    }
                    context.eventQueue.push({GameEventType::CARD_REDRAWN, redrawn});
                }
            }
        break;
    }
    blackJackAndQuintetCheck(players);
    int actingId;
    if (gameState.getPhaseName()==PhaseName::HOST_HIT_PHASE) actingId = gameState.getHostPlayerId();
    else actingId = gameState.getCurrentPlayerId();
    eventQueue.push({GameEventType::REQUEST_ACTION_INPUT,
                RequestActionInputEvent{actingId}});
}

// ============================================================================
// Passive skill processing
// ============================================================================
void Phase::processPassiveSkills(int priorityPlayerId) {
    ReactiveContext context = {ReactiveTrigger::NONE, 0, 0};
    std::vector<std::pair<int, SkillName>> executedPassives = skillManager.skillPassiveHandler(
        ReactiveTrigger::NONE, context, gameState, priorityPlayerId, (int)players.size()
    );
    for (auto& [playerId, skillName] : executedPassives) {
        switch (skillName) {
            case SkillName::DELIVERANCE:
                std::cout << "[Phase] Deliverance passive activated for P" << playerId << std::endl;
                eventQueue.push({GameEventType::DELIVERANCE_EFFECT, DeliveranceEffectEvent{playerId}});
                eventQueue.push({GameEventType::POINT_CHANGED, PointChangedEvent{playerId,"+1 SKILL USES"}});
                break;
            default:
                break;
        }
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

    // --- Both picks received: reveal cards first ---
    if (!ng.revealSent) {
        // Remember original face state so we only flip back cards that were face-down
        Card* c1 = findCard(ng.firstCardId);
        Card* c2 = findCard(ng.secondCardId);
        ng.firstWasFaceDown  = c1 && !c1->isFaceUp();
        ng.secondWasFaceDown = c2 && !c2->isFaceUp();
        // Flip face-up in game logic so ALL players see the reveal (not just skill user)
        if (c1 && !c1->isFaceUp()) c1->flip();
        if (c2 && !c2->isFaceUp()) c2->flip();
        // Sync gameState so broadcast includes the faceUp change
        roundManager.updateGameState(gameState.getPhaseName(), gameState.getCurrentPlayerId());
        eventQueue.push({GameEventType::CARDS_REVEALED, CardsRevealedEvent{{ng.firstCardId, ng.secondCardId}}});
        ng.revealSent = true;
        return; // let the flip animation play before showing boost pick UI
    }

    // --- Request boost pick (after reveal animation) ---
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

    // Flip cards back face-down only if they were face-down before NG revealed them.
    // During battle phase cards are already face-up — don't flip them back down.
    if (ng.firstWasFaceDown && firstCard->isFaceUp()) firstCard->flip();
    if (ng.secondWasFaceDown && secondCard->isFaceUp()) secondCard->flip();
    roundManager.updateGameState(gameState.getPhaseName(), gameState.getCurrentPlayerId());

    skillUser.setPendingAction(PlayerAction::IDLE);
    int ngUserId = ng.skillUserId;
    ngPending = std::nullopt;
    std::cout << "[ngTickPending] NeuralGambit complete for P" << ngUserId << std::endl;

    // Check Chronosphere hand-value-change trigger before re-prompting
    if (!tryChronoReactiveCheck(ngUserId)) {
        eventQueue.push({GameEventType::REQUEST_ACTION_INPUT,
            RequestActionInputEvent{ngUserId}});
    }
}

// ============================================================================
// Reactive skill check orchestration
// ============================================================================
bool Phase::startReactiveCheck(ReactiveTrigger trigger, int drawnCardId, int drawerId, int actingPlayerId) {
    ReactiveContext ctx{trigger, drawerId, drawnCardId};
    auto qualified = skillManager.getReactiveSkills(trigger, ctx, gameState, actingPlayerId, (int)players.size());

    if (qualified.empty()) return false;

    ReactiveCheckState state;
    state.trigger = trigger;
    state.drawnCardId = drawnCardId;
    state.drawerId = drawerId;
    state.actingPlayerId = actingPlayerId;
    std::string extraInfo="";
    for (auto& [ownerId, skillName] : qualified) {
        switch (skillName) {
            case SkillName::FATALDEAL: {
                int rankofCardDrawn = static_cast<int>(gameState.getCardRankById(drawnCardId));
                std::string peekInfo = "Lethal revelation: " + std::to_string(rankofCardDrawn) + " , ready to strike the deal?";
                std::cout<<"[startReactiveCheck] FATALDEAL PEEK: "<< peekInfo <<std::endl;
                extraInfo = peekInfo;
                break;
            }
            case SkillName::DESTINYDEFLECT: {
                std::string peekInfo = "Make this their problem instead?";
                std::cout<<"[startReactiveCheck] DESTINYDEFLECT info: "<< peekInfo <<std::endl;
                extraInfo = peekInfo;
                break;
            }
            case SkillName::CHRONOSPHERE:{
                std::string peekInfo = "Wanna snapshot this moment?";
                std::cout<<"[startReactiveCheck] CHRONOSPHERE info: "<< peekInfo <<std::endl;
                extraInfo = peekInfo;
                break;
            }
            default:
                break;
        }
        state.queue.push_back({ownerId, skillName, extraInfo});
    }
    state.currentIndex = 0;
    state.step = ReactiveCheckState::PROMPTING;
    state.requestSent = false;

    reactiveCheck = state;
    std::cout << "[startReactiveCheck] Reactive check started with " << qualified.size()
              << " qualified skill(s)" << std::endl;
    return true;
}

void Phase::reactiveTickPending() {
    auto& rc = *reactiveCheck;

    // All skills processed — check blackjack/quintet, then re-prompt the acting player
    if (rc.currentIndex >= (int)rc.queue.size()) {
        blackJackAndQuintetCheck(players);
        int actingId = rc.actingPlayerId;
        reactiveCheck = std::nullopt;
        std::cout << "[reactiveTickPending] All reactive checks done, re-prompting P"
                  << actingId << std::endl;
        // Check Chronosphere hand-value-change trigger before re-prompting
        if (!tryChronoReactiveCheck(actingId)) {
            eventQueue.push({GameEventType::REQUEST_ACTION_INPUT,
                RequestActionInputEvent{actingId}});
        }
        return;
    }

    auto& entry = rc.queue[rc.currentIndex];

    // Dispatch to per-skill isolated workflow
    switch (entry.skillName) {
        case SkillName::FATALDEAL:       tickReactive_FatalDeal(rc, entry); break;
        case SkillName::DESTINYDEFLECT:  tickReactive_DestinyDeflect(rc, entry); break;
        case SkillName::CHRONOSPHERE:    tickReactive_Chronosphere(rc, entry); break;
        default:
            std::cerr << "[reactiveTickPending] Unknown reactive skill, skipping" << std::endl;
            rc.currentIndex++;
            rc.step = ReactiveCheckState::PROMPTING;
            rc.requestSent = false;
            break;
    }
}

// ============================================================================
// Fatal Deal reactive workflow
// ============================================================================
void Phase::tickReactive_FatalDeal(ReactiveCheckState& rc, ReactiveCheckState::QueueEntry& entry) {
    NetworkManager* net = roundManager.getNetworkManager();
    int localPlayerId = net ? net->getLocalPlayerId() : 0;
    PlayerInfo ownerInfo = gameState.getPlayerInfo(entry.skillOwnerId);

    auto isRemotePlayer = [&](const PlayerInfo& info) -> bool {
        return !info.isBot && net && net->isServer() && info.playerId != localPlayerId;
    };

    auto advance = [&]() {
        rc.currentIndex++;
        rc.step = ReactiveCheckState::PROMPTING;
        rc.requestSent = false;
    };

    switch (rc.step) {
    case ReactiveCheckState::PROMPTING: {
        if (rc.requestSent) break;

        eventQueue.push({GameEventType::FATALDEAL_EFFECT, FatalDealEffectEvent{entry.skillOwnerId}});

        if (ownerInfo.isBot) {
            gameState.pendingReactiveResponse = ReactiveResponse::YES;
            std::cout << "[tickReactive_FatalDeal] Bot P" << entry.skillOwnerId << " auto-accepts" << std::endl;
        } else if (isRemotePlayer(ownerInfo)) {
            if (net) net->sendReactivePrompt(entry.skillOwnerId, entry.skillName, entry.extraInfo, ReactiveCheckState::PROMPT_TIMEOUT);
            std::cout << "[tickReactive_FatalDeal] Sent prompt to remote P" << entry.skillOwnerId << std::endl;
        } else {
            uiManager.requestReactivePrompt(
                gameState.skillNameToString(entry.skillName), entry.extraInfo, ReactiveCheckState::PROMPT_TIMEOUT);
            std::cout << "[tickReactive_FatalDeal] Prompting local P" << entry.skillOwnerId << std::endl;
        }
        rc.requestSent = true;
        rc.promptTimer = 0.f;
        rc.step = ReactiveCheckState::WAITING_RESPONSE;
    } break;

    case ReactiveCheckState::WAITING_RESPONSE: {
        rc.promptTimer += 1.f / 60.f;
        ReactiveResponse response = ReactiveResponse::NONE;

        if (ownerInfo.isBot) {
            response = gameState.pendingReactiveResponse;
        } else if (isRemotePlayer(ownerInfo)) {
            if (net && net->hasReactiveResponse(entry.skillOwnerId)) {
                bool accepted = net->consumeReactiveResponse(entry.skillOwnerId);
                response = accepted ? ReactiveResponse::YES : ReactiveResponse::NO;
            }
        } else {
            response = gameState.pendingReactiveResponse;
        }

        if (response == ReactiveResponse::NONE && rc.promptTimer >= ReactiveCheckState::PROMPT_TIMEOUT) {
            response = ReactiveResponse::NO;
            std::cout << "[tickReactive_FatalDeal] Prompt timed out for P" << entry.skillOwnerId << std::endl;
        }

        if (response == ReactiveResponse::YES) {
            gameState.pendingReactiveResponse = ReactiveResponse::NONE;
            rc.step = ReactiveCheckState::PICKING_CARD;
            rc.requestSent = false;
            std::cout << "[tickReactive_FatalDeal] P" << entry.skillOwnerId << " accepted" << std::endl;
        } else if (response == ReactiveResponse::NO) {
            gameState.pendingReactiveResponse = ReactiveResponse::NONE;
            advance();
            std::cout << "[tickReactive_FatalDeal] P" << entry.skillOwnerId << " declined" << std::endl;
        }
    } break;

    case ReactiveCheckState::PICKING_CARD: {
        if (!rc.requestSent) {
            std::vector<int> handCardIds;
            for (auto& p : players) {
                if (p.getId() == entry.skillOwnerId) {
                    for (int i = 0; i < p.getHandSize(); i++)
                        handCardIds.push_back(p.getCardInHand(i)->getId());
                    break;
                }
            }

            if (ownerInfo.isBot) {
                if (!handCardIds.empty()) {
                    int pickedId = handCardIds[rand() % handCardIds.size()];
                    Card tempCard(Suit::Hearts, Rank::Ace, false);
                    tempCard.setId(pickedId);
                    gameState.pendingTarget = PlayerTargeting{{}, {tempCard}};
                    std::cout << "[tickReactive_FatalDeal] Bot P" << entry.skillOwnerId
                              << " auto-picked card " << pickedId << std::endl;
                }
            } else if (isRemotePlayer(ownerInfo)) {
                if (net) net->sendTargetRequest(entry.skillOwnerId,
                    TargetRequestData{-1, handCardIds, false});
                std::cout << "[tickReactive_FatalDeal] Sent card pick to remote P" << entry.skillOwnerId << std::endl;
            } else {
                uiManager.requestPickCard(handCardIds);
                rc.waitingForLocalPick = true;
                std::cout << "[tickReactive_FatalDeal] Requesting card pick from local P" << entry.skillOwnerId << std::endl;
            }
            rc.requestSent = true;
        } else {
            int pickedCardId = -1;

            if (isRemotePlayer(ownerInfo) && net && net->hasRemoteTarget(entry.skillOwnerId)) {
                auto t = net->consumeRemoteTarget(entry.skillOwnerId);
                if (!t.targetCards.empty()) pickedCardId = t.targetCards[0].getId();
            } else if (!gameState.pendingTarget.targetCards.empty()) {
                pickedCardId = gameState.pendingTarget.targetCards[0].getId();
                gameState.pendingTarget = {};
                rc.waitingForLocalPick = false;
            }

            if (pickedCardId != -1) {
                std::cout << "[tickReactive_FatalDeal] P" << entry.skillOwnerId
                          << " picked card " << pickedCardId << std::endl;

                Card* drawnCard = nullptr;
                Card* pickedCard = nullptr;
                Player* currentHolder = nullptr;  // whoever currently holds the drawn card (may differ from original drawer)
                Player* skillOwner = nullptr;
                for (auto& p : players) {
                    if (p.getId() == entry.skillOwnerId) skillOwner = &p;
                    for (int i = 0; i < p.getHandSize(); i++) {
                        Card* c = p.getCardInHand(i);
                        if (c->getId() == rc.drawnCardId) { drawnCard = c; currentHolder = &p; }
                        if (c->getId() == pickedCardId) pickedCard = c;
                    }
                }

                if (drawnCard && pickedCard && currentHolder && skillOwner) {
                    SkillContext context{
                        *skillOwner, {currentHolder}, {drawnCard, pickedCard},
                        deck, gameState, eventQueue
                    };
                    SkillExecutionResult result = skillManager.processSkill(context);
                    if (result.name == SkillName::UNDEFINED) {
                        eventQueue.push({GameEventType::SKILL_ERROR,
                            SkillErrorEvent{entry.skillOwnerId, result.errorMsg}});
                    } else {
                        skillProcessAftermath(context, result);
                    }
                    roundManager.updateGameState(gameState.getPhaseName(), rc.actingPlayerId);
                } else {
                    std::cerr << "[tickReactive_FatalDeal] ERROR: could not resolve cards/players" << std::endl;
                }
                advance();
            }
        }
    } break;

    default: break;
    }
}

// ============================================================================
// Destiny Deflect reactive workflow
// ============================================================================
void Phase::tickReactive_DestinyDeflect(ReactiveCheckState& rc, ReactiveCheckState::QueueEntry& entry) {
    NetworkManager* net = roundManager.getNetworkManager();
    int localPlayerId = net ? net->getLocalPlayerId() : 0;
    PlayerInfo ownerInfo = gameState.getPlayerInfo(entry.skillOwnerId);

    auto isRemotePlayer = [&](const PlayerInfo& info) -> bool {
        return !info.isBot && net && net->isServer() && info.playerId != localPlayerId;
    };

    auto advance = [&]() {
        rc.currentIndex++;
        rc.step = ReactiveCheckState::PROMPTING;
        rc.requestSent = false;
    };

    switch (rc.step) {
    case ReactiveCheckState::PROMPTING: {
        if (rc.requestSent) break;

        eventQueue.push({GameEventType::DESTINYDEFLECT_EFFECT, DestinyDeflectEffectEvent{entry.skillOwnerId}});

        if (ownerInfo.isBot) {
            gameState.pendingReactiveResponse = ReactiveResponse::YES;
            std::cout << "[tickReactive_DestinyDeflect] Bot P" << entry.skillOwnerId << " auto-accepts" << std::endl;
        } else if (isRemotePlayer(ownerInfo)) {
            if (net) net->sendReactivePrompt(entry.skillOwnerId, entry.skillName, entry.extraInfo, ReactiveCheckState::PROMPT_TIMEOUT);
            std::cout << "[tickReactive_DestinyDeflect] Sent prompt to remote P" << entry.skillOwnerId << std::endl;
        } else {
            uiManager.requestReactivePrompt(
                gameState.skillNameToString(entry.skillName), entry.extraInfo, ReactiveCheckState::PROMPT_TIMEOUT);
            std::cout << "[tickReactive_DestinyDeflect] Prompting local P" << entry.skillOwnerId << std::endl;
        }
        rc.requestSent = true;
        rc.promptTimer = 0.f;
        rc.step = ReactiveCheckState::WAITING_RESPONSE;
    } break;

    case ReactiveCheckState::WAITING_RESPONSE: {
        rc.promptTimer += 1.f / 60.f;
        ReactiveResponse response = ReactiveResponse::NONE;

        if (ownerInfo.isBot) {
            response = gameState.pendingReactiveResponse;
        } else if (isRemotePlayer(ownerInfo)) {
            if (net && net->hasReactiveResponse(entry.skillOwnerId)) {
                bool accepted = net->consumeReactiveResponse(entry.skillOwnerId);
                response = accepted ? ReactiveResponse::YES : ReactiveResponse::NO;
            }
        } else {
            response = gameState.pendingReactiveResponse;
        }

        if (response == ReactiveResponse::NONE && rc.promptTimer >= ReactiveCheckState::PROMPT_TIMEOUT) {
            response = ReactiveResponse::NO;
            std::cout << "[tickReactive_DestinyDeflect] Prompt timed out for P" << entry.skillOwnerId << std::endl;
        }

        if (response == ReactiveResponse::YES) {
            gameState.pendingReactiveResponse = ReactiveResponse::NONE;
            rc.step = ReactiveCheckState::PICKING_PLAYER;
            rc.requestSent = false;
            std::cout << "[tickReactive_DestinyDeflect] P" << entry.skillOwnerId << " accepted, picking player" << std::endl;
        } else if (response == ReactiveResponse::NO) {
            gameState.pendingReactiveResponse = ReactiveResponse::NONE;
            advance();
            std::cout << "[tickReactive_DestinyDeflect] P" << entry.skillOwnerId << " declined" << std::endl;
        }
    } break;

    case ReactiveCheckState::PICKING_PLAYER: {
        if (!rc.requestSent) {
            std::vector<int> allowedPlayerIds;
            for (auto& p : players) {
                if (p.getId() != entry.skillOwnerId)
                    allowedPlayerIds.push_back(p.getId());
            }

            if (ownerInfo.isBot) {
                // Deterministic: pick player with highest hand value (push toward bust)
                int bestId = -1, bestValue = -1;
                for (auto& p : players) {
                    if (p.getId() == entry.skillOwnerId) continue;
                    int val = p.calculateHandValue();
                    if (val > bestValue || (val == bestValue && (bestId == -1 || p.getId() < bestId))) {
                        bestValue = val;
                        bestId = p.getId();
                    }
                }
                if (bestId != -1) {
                    gameState.pendingTarget = PlayerTargeting{{bestId}, {}};
                    std::cout << "[tickReactive_DestinyDeflect] Bot P" << entry.skillOwnerId
                              << " picked target P" << bestId << " (hand value " << bestValue << ")" << std::endl;
                }
            } else if (isRemotePlayer(ownerInfo)) {
                if (net) net->sendTargetRequest(entry.skillOwnerId,
                    TargetRequestData{-1, allowedPlayerIds, false, true});
                std::cout << "[tickReactive_DestinyDeflect] Sent player pick to remote P" << entry.skillOwnerId << std::endl;
            } else {
                uiManager.requestPlayerPick(allowedPlayerIds);
                rc.waitingForLocalPick = true;
                std::cout << "[tickReactive_DestinyDeflect] Requesting player pick from local P" << entry.skillOwnerId << std::endl;
            }
            rc.requestSent = true;
        } else {
            int pickedPlayerId = -1;

            if (isRemotePlayer(ownerInfo) && net && net->hasRemoteTarget(entry.skillOwnerId)) {
                auto t = net->consumeRemoteTarget(entry.skillOwnerId);
                if (!t.targetPlayerIds.empty()) pickedPlayerId = t.targetPlayerIds[0];
            } else if (!gameState.pendingTarget.targetPlayerIds.empty()) {
                pickedPlayerId = gameState.pendingTarget.targetPlayerIds[0];
                gameState.pendingTarget = {};
                rc.waitingForLocalPick = false;
            }

            if (pickedPlayerId != -1) {
                std::cout << "[tickReactive_DestinyDeflect] P" << entry.skillOwnerId
                          << " picked redirect target P" << pickedPlayerId << std::endl;

                Card* drawnCard = nullptr;
                Player* currentHolder = nullptr;  // whoever currently holds the drawn card (may differ from original drawer after FD swap)
                Player* redirectTarget = nullptr;
                Player* skillOwner = nullptr;
                for (auto& p : players) {
                    if (p.getId() == pickedPlayerId) redirectTarget = &p;
                    if (p.getId() == entry.skillOwnerId) skillOwner = &p;
                    for (int i = 0; i < p.getHandSize(); i++) {
                        if (p.getCardInHand(i)->getId() == rc.drawnCardId) { drawnCard = p.getCardInHand(i); currentHolder = &p; }
                    }
                }

                if (drawnCard && currentHolder && redirectTarget && skillOwner) {
                    SkillContext context{
                        *skillOwner, {currentHolder, redirectTarget}, {drawnCard},
                        deck, gameState, eventQueue
                    };
                    SkillExecutionResult result = skillManager.processSkill(context);
                    if (result.name == SkillName::UNDEFINED) {
                        eventQueue.push({GameEventType::SKILL_ERROR,
                            SkillErrorEvent{entry.skillOwnerId, result.errorMsg}});
                    } else {
                        skillProcessAftermath(context, result);
                    }
                    roundManager.updateGameState(gameState.getPhaseName(), rc.actingPlayerId);
                } else {
                    std::cerr << "[tickReactive_DestinyDeflect] ERROR: could not resolve card/players" << std::endl;
                }
                advance();
            }
        }
    } break;

    default: break;
    }
}

// ============================================================================
// Chronosphere reactive workflow
// ============================================================================
void Phase::tickReactive_Chronosphere(ReactiveCheckState& rc, ReactiveCheckState::QueueEntry& entry) {
    NetworkManager* net = roundManager.getNetworkManager();
    int localPlayerId = net ? net->getLocalPlayerId() : 0;
    PlayerInfo ownerInfo = gameState.getPlayerInfo(entry.skillOwnerId);

    auto isRemotePlayer = [&](const PlayerInfo& info) -> bool {
        return !info.isBot && net && net->isServer() && info.playerId != localPlayerId;
    };

    auto advance = [&]() {
        rc.currentIndex++;
        rc.step = ReactiveCheckState::PROMPTING;
        rc.requestSent = false;
    };

    switch (rc.step) {
    case ReactiveCheckState::PROMPTING: {
        if (rc.requestSent) break;

        if (ownerInfo.isBot) {
            gameState.pendingReactiveResponse = ReactiveResponse::YES;
            std::cout << "[tickReactive_Chronosphere] Bot P" << entry.skillOwnerId << " auto-accepts" << std::endl;
        } else if (isRemotePlayer(ownerInfo)) {
            if (net) net->sendReactivePrompt(entry.skillOwnerId, entry.skillName, entry.extraInfo, ReactiveCheckState::PROMPT_TIMEOUT);
            std::cout << "[tickReactive_Chronosphere] Sent prompt to remote P" << entry.skillOwnerId << std::endl;
        } else {
            uiManager.requestReactivePrompt(
                gameState.skillNameToString(entry.skillName), entry.extraInfo, ReactiveCheckState::PROMPT_TIMEOUT);
            std::cout << "[tickReactive_Chronosphere] Prompting local P" << entry.skillOwnerId << std::endl;
        }
        rc.requestSent = true;
        rc.promptTimer = 0.f;
        rc.step = ReactiveCheckState::WAITING_RESPONSE;
    } break;

    case ReactiveCheckState::WAITING_RESPONSE: {
        rc.promptTimer += 1.f / 60.f;
        ReactiveResponse response = ReactiveResponse::NONE;

        if (ownerInfo.isBot) {
            response = gameState.pendingReactiveResponse;
        } else if (isRemotePlayer(ownerInfo)) {
            if (net && net->hasReactiveResponse(entry.skillOwnerId)) {
                bool accepted = net->consumeReactiveResponse(entry.skillOwnerId);
                response = accepted ? ReactiveResponse::YES : ReactiveResponse::NO;
            }
        } else {
            response = gameState.pendingReactiveResponse;
        }

        if (response == ReactiveResponse::NONE && rc.promptTimer >= ReactiveCheckState::PROMPT_TIMEOUT) {
            response = ReactiveResponse::NO;
            std::cout << "[tickReactive_Chronosphere] Prompt timed out for P" << entry.skillOwnerId << std::endl;
        }

        if (response == ReactiveResponse::YES) {
            gameState.pendingReactiveResponse = ReactiveResponse::NONE;
            // Reactive ON_HIT_PHASE_START: snapshot only — execute directly
            Player* skillOwner = nullptr;
            for (auto& p : players) if (p.getId() == entry.skillOwnerId) { skillOwner = &p; break; }
            if (skillOwner) {
                gameState.pendingChronoChoice = ChronoChoice::SNAPSHOT;
                SkillContext context{ *skillOwner, {}, {}, deck, gameState, eventQueue };
                SkillExecutionResult result = skillManager.processSkill(context);
                if (result.name == SkillName::UNDEFINED) {
                    eventQueue.push({GameEventType::SKILL_ERROR,
                        SkillErrorEvent{entry.skillOwnerId, result.errorMsg}});
                } else {
                    skillProcessAftermath(context, result);
                }
                gameState.pendingChronoChoice = ChronoChoice::NONE;
                roundManager.updateGameState(gameState.getPhaseName(), rc.actingPlayerId);
            }
            std::cout << "[tickReactive_Chronosphere] Executed for P" << entry.skillOwnerId << std::endl;
            advance();
        } else if (response == ReactiveResponse::NO) {
            gameState.pendingReactiveResponse = ReactiveResponse::NONE;
            advance();
            std::cout << "[tickReactive_Chronosphere] P" << entry.skillOwnerId << " declined" << std::endl;
        }
    } break;

    default: break;
    }
}

void Phase::blackJackAndQuintetCheck(std::vector<Player>& players){
    static const std::vector<std::string> blackjackPhrases = {
        "BLACKJACK! THEY KNEW WHAT THEY WERE DOING",
        "BLACKJACK! CALCULATED? NAH, JUST LUCKY",
        "BLACKJACK! THE CARDS WERE SCARED TO SAY NO",
        "BLACKJACK! SOMEONE CALL THE POLICE",
        "BLACKJACK! ACTUAL WITCHCRAFT",
        "BLACKJACK! THAT'S NOT FAIR AND WE ALL KNOW IT",
        "BLACKJACK! ZERO HESITATION ZERO REGRETS",
        "BLACKJACK! THE PROPHECY IS FULFILLED",
        "BLACKJACK! DISGUSTING. ABSOLUTELY DISGUSTING.",
        "BLACKJACK! THEY CAN'T KEEP GETTING AWAY WITH THIS",
    };
    static const std::vector<std::string> quintetPhrases = {
        "QUINTET! FIVE CARDS AND STILL BREATHING???",
        "QUINTET! THIS SHOULDN'T BE MATHEMATICALLY POSSIBLE",
        "QUINTET! THEY BROUGHT THE WHOLE SQUAD",
        "QUINTET! HOARDING CARDS LIKE POKEMON",
        "QUINTET! THE GREEDIEST HAND IN HISTORY",
        "QUINTET! LIVING LIFE ON THE EDGE AND WINNING",
        "QUINTET! ABSOLUTE MANIAC WITH FIVE CARDS",
        "QUINTET! THE DECK RAN OUT OF IDEAS",
        "QUINTET! COLLECT THEM ALL APPARENTLY",
        "QUINTET! FIVE DEEP NO FEAR ALL GAS",
    };

    for (auto& player : players) {
        int handValue = player.calculateHandValue();
        auto currentIds = [&]() {
            std::vector<int> ids;
            for (int i = 0; i < player.getHandSize(); i++)
                ids.push_back(player.getCardInHand(i)->getId());
            std::sort(ids.begin(), ids.end());
            return ids;
        }();
        if (player.getHandSize() == 5 && handValue <= GameConfig::BLACKJACK_VALUE && currentIds != player.lastQuintetHand) {
            player.lastQuintetHand = currentIds;
            std::vector<int> revealIds;
            for (int i = 0; i < player.getHandSize(); i++)
                revealIds.push_back(player.getCardInHand(i)->getId());
            eventQueue.push({GameEventType::CARDS_REVEALED, CardsRevealedEvent{revealIds}});
            player.gainPoint(GameConfig::POINTS_GAIN_QUINTET);
            const std::string& phrase = quintetPhrases[std::rand() % quintetPhrases.size()];
            eventQueue.push({GameEventType::POINT_CHANGED, PointChangedEvent{
                player.getId(), phrase + " +" + std::to_string(GameConfig::POINTS_GAIN_QUINTET)}});
        }
        else if (player.getHandSize() == 2 && handValue == GameConfig::BLACKJACK_VALUE && currentIds != player.lastBlackjackHand) {
            player.lastBlackjackHand = currentIds;
            std::vector<int> revealIds;
            for (int i = 0; i < player.getHandSize(); i++)
                revealIds.push_back(player.getCardInHand(i)->getId());
            eventQueue.push({GameEventType::CARDS_REVEALED, CardsRevealedEvent{revealIds}});
            player.gainPoint(GameConfig::POINTS_GAIN_BLACKJACK);
            const std::string& phrase = blackjackPhrases[std::rand() % blackjackPhrases.size()];
            eventQueue.push({GameEventType::POINT_CHANGED, PointChangedEvent{
                player.getId(), phrase + " +" + std::to_string(GameConfig::POINTS_GAIN_BLACKJACK)}});
        }
    }
    roundManager.updateGameState(gameState.getPhaseName(), gameState.getCurrentPlayerId());
}

// ============================================================================
// Chronosphere: universal hand-value-change detector
// ============================================================================
bool Phase::tryChronoReactiveCheck(int playerId) {
    // Scan ALL players for a Chronosphere owner whose hand value changed.
    // playerId is the acting player (who will be re-prompted if chrono doesn't fire).
    // The Chronosphere owner may be a different player (e.g. NeuralGambit boosted their card).
    for (auto& p : players) {
        if (p.getSkillName() != SkillName::CHRONOSPHERE) continue;
        if (p.getHandSize() == 0) continue;
        int currentValue = p.calculateHandValue();
        if (currentValue != gameState.chronoTrackedHandValue && gameState.chronoTrackedHandValue != -1) {
            int oldValue = gameState.chronoTrackedHandValue;
            gameState.chronoTrackedHandValue = currentValue;
            chronoPending = ChronoPendingState{};
            chronoPending->skillUserId = p.getId();
            chronoPending->actingPlayerId = playerId;
            std::cout << "[tryChronoReactiveCheck] Hand value changed for P" << p.getId()
                      << " (was " << oldValue
                      << ", now " << currentValue << ") — starting chrono prompt" << std::endl;
            return true;
        }
    }
    return false;
}

// ============================================================================
// Chronosphere pending state orchestration
// ============================================================================
void Phase::chronoTickPending(Player& skillUser) {
    auto& cp = *chronoPending;
    NetworkManager* net = roundManager.getNetworkManager();
    int localPlayerId = net ? net->getLocalPlayerId() : 0;
    PlayerInfo ownerInfo = gameState.getPlayerInfo(cp.skillUserId);

    bool isBot = ownerInfo.isBot;
    bool isRemote = !ownerInfo.isBot && net && net->isServer() && ownerInfo.playerId != localPlayerId;

    // Step 1: Send prompt
    if (!cp.promptSent) {
        if (isBot) {
            // Bot auto-picks: snapshot if none, rewind if exists
            gameState.pendingChronoChoice = gameState.snapShotTaken
                ? ChronoChoice::REWIND : ChronoChoice::SNAPSHOT;
            std::cout << "[chronoTickPending] Bot P" << cp.skillUserId << " auto-chose "
                      << (gameState.snapShotTaken ? "REWIND" : "SNAPSHOT") << std::endl;
        } else if (isRemote) {
            if (net) net->sendChronoPrompt(cp.skillUserId, gameState.snapShotTaken);
            std::cout << "[chronoTickPending] Sent chrono prompt to remote P" << cp.skillUserId << std::endl;
        } else {
            // Local player: show chrono choice UI
            uiManager.requestChronoPrompt(gameState.snapShotTaken, ChronoPendingState::PROMPT_TIMEOUT);
            std::cout << "[chronoTickPending] Showing chrono prompt to local P" << cp.skillUserId << std::endl;
        }
        cp.promptSent = true;
        cp.promptTimer = 0.f;
        return;
    }

    // Step 2: Poll for response
    cp.promptTimer += 1.f / 60.f;

    ChronoChoice response = ChronoChoice::NONE;

    if (isBot) {
        response = gameState.pendingChronoChoice;
    } else if (isRemote) {
        if (net && net->hasChronoResponse(cp.skillUserId)) {
            response = net->consumeChronoResponse(cp.skillUserId);
        }
    } else {
        response = gameState.pendingChronoChoice;
    }

    // Timeout: decline
    if (response == ChronoChoice::NONE && cp.promptTimer >= ChronoPendingState::PROMPT_TIMEOUT) {
        std::cout << "[chronoTickPending] Prompt timed out for P" << cp.skillUserId << std::endl;
        gameState.pendingChronoChoice = ChronoChoice::NONE;
        int actingId = cp.actingPlayerId;
        chronoPending = std::nullopt;
        skillUser.setPendingAction(PlayerAction::IDLE);
        eventQueue.push({GameEventType::REQUEST_ACTION_INPUT, RequestActionInputEvent{actingId}});
        return;
    }

    if (response == ChronoChoice::NONE) return; // still waiting

    // Step 3: Execute
    gameState.pendingChronoChoice = response;

    Player* chronoUser = nullptr;
    for (auto& p : players) if (p.getId() == cp.skillUserId) { chronoUser = &p; break; }
    if (!chronoUser) { chronoPending = std::nullopt; return; }

    SkillContext context{ *chronoUser, {}, {}, deck, gameState, eventQueue };

    SkillExecutionResult result = skillManager.processSkill(context);
    if (result.name == SkillName::UNDEFINED) {
        eventQueue.push({GameEventType::SKILL_ERROR,
            SkillErrorEvent{cp.skillUserId, result.errorMsg}});
    } else {
        skillProcessAftermath(context, result);
    }

    // Update tracked hand value after chrono action
    gameState.chronoTrackedHandValue = chronoUser->calculateHandValue();
    gameState.pendingChronoChoice = ChronoChoice::NONE;
    roundManager.updateGameState(gameState.getPhaseName(), gameState.getCurrentPlayerId());

    int actingId = cp.actingPlayerId;
    chronoPending = std::nullopt;
    skillUser.setPendingAction(PlayerAction::IDLE);
    // Push directly — don't re-check chrono to avoid infinite loop
    // Re-prompt the acting player (may differ from chrono owner in HOST_HIT_PHASE)
    eventQueue.push({GameEventType::REQUEST_ACTION_INPUT, RequestActionInputEvent{actingId}});
    std::cout << "[chronoTickPending] Chronosphere complete for P" << cp.skillUserId
              << ", re-prompting P" << actingId << std::endl;
}