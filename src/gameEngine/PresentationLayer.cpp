#include "PresentationLayer.h"
#include "AnimationManager.h"
#include "UIManager.h"
#include "../lowLevelEntities/VisualState.h"

PresentationLayer::PresentationLayer(EventQueue& eq, AnimationManager& am,
                                     UIManager& um, VisualState& vs, GameState& gs)
    : eventQueue(eq), animationManager(am), uiManager(um), visualState(vs), gameState(gs)
{}

bool PresentationLayer::isCutscene(GameEventType type) {
    switch (type) {
        case GameEventType::PHASE_ANNOUNCED:
        case GameEventType::CARD_DRAWN:
        case GameEventType::CARD_RETURNED:
        case GameEventType::CARD_DELIVERANCE:
        case GameEventType::HAND_REPOSITIONED:
        case GameEventType::NEURALGAMBIT_REVEAL:
        case GameEventType::FATALDEAL_SWAP:
        case GameEventType::CARDS_REVEALED:
            uiManager.clearInput();
            return true;
        default:
            return false;
    }
}

void PresentationLayer::processEvents() {
    while (!eventQueue.empty()) {
        // A cutscene event was processed last time — wait for its animation to finish
        if (cutsceneBlocking && animationManager.playing()) break;
        cutsceneBlocking = false;

        GameEvent event = eventQueue.pop();

        switch (event.type) {

        case GameEventType::CARD_DRAWN: {
            auto& e = std::get<CardDrawnEvent>(event.data);
            std::cout << "[PresentationLayer] CARD_DRAWN: cardId=" << e.cardId
                      << " -> player " << e.playerId << " hand[" << e.handIndex << "]" << std::endl;
            CardVisual& card = visualState.getCardVisual(e.cardId);
            card.location = CardLocation::HAND;
            card.ownerId = e.playerId;
            card.cardIndex = e.handIndex;
            animationManager.addDrawAnimation(e.playerId, e.handIndex, e.cardId);
        } break;

        case GameEventType::CARD_RETURNED: {
            auto& e = std::get<CardReturnedEvent>(event.data);
            std::cout << "[PresentationLayer] CARD_RETURNED: cardId=" << e.cardId << std::endl;
            CardVisual& card = visualState.getCardVisual(e.cardId);
            card.ownerId = -1;
            card.cardIndex = -1;
            card.location = CardLocation::DECK;
            animationManager.addReturnToDeckAnimation(e.cardId);
        } break;

        case GameEventType::CARD_DELIVERANCE: {
            auto& e = std::get<CardSpinEvent>(event.data);
            std::cout << "[PresentationLayer] CARD_DELIVERANCE: cardId=" << e.cardId
                      << " player=" << e.playerId << std::endl;
            CardVisual& cv = visualState.getCardVisual(e.cardId);
            int cardId = e.cardId;
            int playerId = e.playerId;

            animationManager.playDeliveranceEffect(cv.cardSprite.getPosition());

            animationManager.addSpinAnimation(cardId, [this, cardId, playerId](){
                std::cout << "[PresentationLayer] DELIVERANCE spin done, cardId=" << cardId
                          << " -> return to deck" << std::endl;
                CardVisual& card = visualState.getCardVisual(cardId);
                card.ownerId = -1;
                card.cardIndex = -1;
                card.location = CardLocation::DECK;
                animationManager.addReturnToDeckAnimation(cardId, [this, playerId](){
                    std::cout << "[PresentationLayer] DELIVERANCE return done -> repositionHand player="
                              << playerId << std::endl;
                    animationManager.repositionHand(playerId);
                });
            });
        } break;

        case GameEventType::HAND_REPOSITIONED: {
            auto& e = std::get<HandRepositionedEvent>(event.data);
            std::cout << "[PresentationLayer] HAND_REPOSITIONED: player=" << e.playerId << std::endl;
            animationManager.repositionHand(e.playerId);
        } break;

        case GameEventType::BATTLE_WIN: {
            auto& e = std::get<BattleWinEvent>(event.data);
            std::cout << "[PresentationLayer] BATTLE_WIN: winner=" << e.winnerId
                      << " loser=" << e.loserId << std::endl;
            (void)e;
        } break;

        case GameEventType::BATTLE_TIE: {
            auto& e = std::get<BattleTieEvent>(event.data);
            std::cout << "[PresentationLayer] BATTLE_TIE: p1=" << e.player1Id
                      << " p2=" << e.player2Id << std::endl;
            (void)e;
        } break;

        case GameEventType::POINT_CHANGED: {
            auto& e = std::get<PointChangedEvent>(event.data);
            std::cout << "[PresentationLayer] POINT_CHANGED: player=" << e.playerId
                      << " text=\"" << e.text << "\"" << std::endl;
            sf::Vector2f seat = visualState.getPlayerSeatPos(
                e.playerId, (int)gameState.getAllPlayerInfo().size());
            sf::Vector2f textPos = {seat.x, seat.y - 40.f};
            sf::Color textColor(245, 224, 32);
            animationManager.spawnFloatingText(e.text, textPos, textColor, AnimConfig::POINT_CHANGE_DURATION);
        } break;

        case GameEventType::PHASE_ANNOUNCED: {
            auto& e = std::get<PhaseAnnouncedEvent>(event.data);
            std::cout << "[PresentationLayer] PHASE_ANNOUNCED: \"" << e.text << "\"" << std::endl;
            animationManager.spawnPhaseText(e.text, e.duration);
        } break;

        case GameEventType::SKILL_ERROR: {
            auto& e = std::get<SkillErrorEvent>(event.data);
                       std::cout << "[PresentationLayer] SKILL_ERROR: player=" << e.playerId
                      << " text=\"" << e.reason << "\"" << std::endl;
            sf::Vector2f seat = visualState.getPlayerSeatPos(
                e.playerId, (int)gameState.getAllPlayerInfo().size()
            );
            sf::Vector2f textPos = {seat.x, seat.y - 40.f};
            sf::Color textColor(255, 69, 69);
            animationManager.spawnFloatingText(e.reason, textPos, textColor, 0.7f);
        } break;

        case GameEventType::SHOCK_EFFECT: {
            auto& e = std::get<ShockEffectEvent>(event.data);
            std::cout << "[PresentationLayer] SHOCK_EFFECT: winner=" << e.winnerId
                      << " loser=" << e.loserId << std::endl;
            animationManager.playShockAnimation(e.winnerId, e.loserId, e.duration);
        } break;

        case GameEventType::EXPLOSION_EFFECT: {
            auto& e = std::get<ExplosionEffectEvent>(event.data);
            std::cout << "[PresentationLayer] EXPLOSION_EFFECT: target=" << e.targetPlayerId << std::endl;
            sf::Vector2f seat = visualState.getPlayerSeatPos(
                e.targetPlayerId, (int)gameState.getAllPlayerInfo().size());
            uiManager.getBorrowedPlayerVisualIds().push_back(e.targetPlayerId);
            PlayerVisual& targetPlayerPv = uiManager.getPlayerVisuals()[e.targetPlayerId];
            auto onDone = [this]() {
                    uiManager.getBorrowedPlayerVisualIds().clear();
                    
            };
            sf::Vector2f pos = {seat.x - 50.f, seat.y - 10.f};

            animationManager.shakeBorrowedPlayerVisual(targetPlayerPv, e.duration,onDone);
            animationManager.playExplosionAnimation(pos, e.scale, e.duration);
        } break;

        case GameEventType::DELIVERANCE_EFFECT: {
            auto& e = std::get<DeliveranceEffectEvent>(event.data);
            std::cout << "[PresentationLayer] DELIVERANCE_EFFECT: playerId=" << e.playerId << std::endl;
            sf::Vector2f playerPos = visualState.getPlayerSeatPos(e.playerId, (int)gameState.getAllPlayerInfo().size());
            animationManager.playDeliveranceEffect({playerPos.x-15.f,playerPos.y});
        } break;

        case GameEventType::FATALDEAL_EFFECT:{
            auto& e = std::get<FatalDealEffectEvent>(event.data);
            std::cout << "[PresentationLayer] FATALDEAL_EFFECT: playerId=" << e.playerId << std::endl;
            sf::Vector2f playerPos = visualState.getPlayerSeatPos(e.playerId, (int)gameState.getAllPlayerInfo().size());
            animationManager.playFatalDealInitialEffect({playerPos.x-15.f,playerPos.y});
        } break;

        case GameEventType::REQUEST_ACTION_INPUT: {
            auto& e = std::get<RequestActionInputEvent>(event.data);
            std::cout << "[PresentationLayer] REQUEST_ACTION_INPUT: player=" << e.playerId << std::endl;
            // Only show UI for locally-controlled human players
            PlayerInfo info = gameState.getPlayerInfo(e.playerId);
            if (!info.isBot && !info.isRemote) {
                uiManager.requestActionInput(e.playerId, e.duration);
            }
        } break;

        case GameEventType::REQUEST_TARGET_INPUT: {
            auto& e = std::get<RequestTargetInputEvent>(event.data);
            std::cout << "[PresentationLayer] REQUEST_TARGET_INPUT: player=" << e.playerId << std::endl;
            PlayerInfo info = gameState.getPlayerInfo(e.playerId);
            if (!info.isBot && !info.isRemote) {
                uiManager.requestTargetInput(e.playerId, e.duration);
            }
        } break;

        case GameEventType::NEURALGAMBIT_REVEAL: {
            auto& e = std::get<NeuralGambitRevealEvent>(event.data);
            std::cout << "[PresentationLayer] NEURALGAMBIT_REVEAL: cards "
                      << e.cardId1 << " & " << e.cardId2
                      << " -> boost card " << e.boostCardId
                      << " +" << e.boostAmount << std::endl;
            // Flip the two revealed cards with animation, then play the NG effect
            animationManager.playCardFlip(e.cardId1);
            animationManager.playCardFlip(e.cardId2);
            int cid1 = e.cardId1, cid2 = e.cardId2;
            animationManager.playNeuralGambitEffect(
                e.cardId1, e.cardId2, e.boostCardId, e.boostAmount,
                2.f, 1.0f,
                [this, cid1, cid2]() {
                    // Just unpin — enforceVisibility() handles face state
                    visualState.getCardVisual(cid1).unpin();
                    visualState.getCardVisual(cid2).unpin();
                }
            );
        } break;

        case GameEventType::CLEAR_INPUT: {
            std::cout << "[PresentationLayer] CLEAR_INPUT" << std::endl;
            uiManager.clearInput();
        } break;

        case GameEventType::REACTIVE_SKILL_PROMPT: {
            auto& e = std::get<ReactiveSkillPromptEvent>(event.data);
            std::cout << "[PresentationLayer] REACTIVE_SKILL_PROMPT: P" << e.skillOwnerId
                      << " skill=" << gameState.skillNameToString(e.skillName) << std::endl;
            PlayerInfo info = gameState.getPlayerInfo(e.skillOwnerId);
            if (!info.isBot && !info.isRemote) {
                uiManager.requestReactivePrompt(
                    gameState.skillNameToString(e.skillName),e.extraInfo, e.timerDuration);
            }
        } break;

        case GameEventType::FATALDEAL_SWAP: {
            auto& e = std::get<FatalDealSwapEvent>(event.data);
            std::cout << "[PresentationLayer] FATALDEAL_SWAP: P" << e.fatalDealUserId
                      << " swapped card " << e.swappedCardId
                      << " with P" << e.drawerId << "'s card " << e.drawnCardId << std::endl;

            CardVisual& swappedCv = visualState.getCardVisual(e.swappedCardId);  // was FD user's
            CardVisual& drawnCv   = visualState.getCardVisual(e.drawnCardId);    // was drawer's

            // Play effect at both card positions before the swap
            animationManager.playFatalDealEffect(
                swappedCv.cardSprite.getPosition(),
                drawnCv.cardSprite.getPosition());

            // Save current positions — cards will swap places
            sf::Vector2f swappedPos = swappedCv.cardSprite.getPosition();
            sf::Vector2f drawnPos   = drawnCv.cardSprite.getPosition();

            // Swap ownership metadata — enforceVisibility() decides face state from ownerId
            std::swap(swappedCv.ownerId, drawnCv.ownerId);
            std::swap(swappedCv.cardIndex, drawnCv.cardIndex);
            // Do NOT swap faceUp — it desynchronizes the flag from the texture rect,
            // causing enforceVisibility() to skip the texture correction.
            // Force-clear all pins — card is changing hands, enforceVisibility() decides face state
            swappedCv.pinCount = 0;
            drawnCv.pinCount = 0;

            std::cout << "[PresentationLayer]   after swap: card " << e.swappedCardId
                      << "(owner=" << swappedCv.ownerId << " idx=" << swappedCv.cardIndex
                      << ") card " << e.drawnCardId
                      << "(owner=" << drawnCv.ownerId << " idx=" << drawnCv.cardIndex << ")" << std::endl;

            // Teleport each card to the other's old position
            int swappedId = e.swappedCardId;
            int drawnId   = e.drawnCardId;

            animationManager.addTeleportSwapAnimation(swappedId, drawnPos, swappedCv.cardIndex, nullptr);
            animationManager.addTeleportSwapAnimation(drawnId,   swappedPos, drawnCv.cardIndex, nullptr);
        } break;

        case GameEventType::CARDS_REVEALED: {
            auto& e = std::get<CardsRevealedEvent>(event.data);
            std::cout << "[PresentationLayer] CARDS_REVEALED: " << e.cardIds.size() << " cards" << std::endl;
            for (int cardId : e.cardIds) {
                // HOST: pin here. CLIENT: already pre-pinned in clientReceive.
                if (!visualState.getCardVisual(cardId).isPinned())
                    visualState.getCardVisual(cardId).pin();
                animationManager.playCardFlip(cardId, [this, cardId]() {
                    visualState.getCardVisual(cardId).unpin();
                });
            }
        } break;

        } // switch

        // Cutscene event — stop processing, remaining events wait for animation
        if (isCutscene(event.type)) {
            cutsceneBlocking = true;
            break;
        }
    } // while

    // All events processed — unblock reconcile so it can sync latest server state
    if (eventQueue.empty()) {
        visualState.setReconcileBlocked(false);
    }
}
