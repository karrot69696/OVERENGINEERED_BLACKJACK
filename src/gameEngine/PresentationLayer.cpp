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
            sf::Vector2f textPos = {seat.x - 90.f, seat.y - 40.f};
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
            animationManager.spawnFloatingText(e.reason, textPos, textColor, 0.5);
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
            sf::Vector2f pos = {seat.x - 50.f, seat.y - 10.f};
            animationManager.playExplosionAnimation(pos, e.scale, e.duration);
        } break;

        case GameEventType::DELIVERANCE_EFFECT: {
            auto& e = std::get<DeliveranceEffectEvent>(event.data);
            std::cout << "[PresentationLayer] DELIVERANCE_EFFECT: cardId=" << e.cardId << std::endl;
            CardVisual& cv = visualState.getCardVisual(e.cardId);
            animationManager.playDeliveranceEffect(cv.cardSprite.getPosition());
        } break;

        case GameEventType::REQUEST_ACTION_INPUT: {
            auto& e = std::get<RequestActionInputEvent>(event.data);
            std::cout << "[PresentationLayer] REQUEST_ACTION_INPUT: player=" << e.playerId << std::endl;
            // Only show UI for locally-controlled human players
            PlayerInfo info = gameState.getPlayerInfo(e.playerId);
            if (!info.isBot && !info.isRemote) {
                uiManager.requestActionInput(e.playerId);
            }
        } break;

        case GameEventType::REQUEST_TARGET_INPUT: {
            auto& e = std::get<RequestTargetInputEvent>(event.data);
            std::cout << "[PresentationLayer] REQUEST_TARGET_INPUT: player=" << e.playerId << std::endl;
            PlayerInfo info = gameState.getPlayerInfo(e.playerId);
            if (!info.isBot && !info.isRemote) {
                uiManager.requestTargetInput(e.playerId);
            }
        } break;

        case GameEventType::NEURALGAMBIT_REVEAL: {
            auto& e = std::get<NeuralGambitRevealEvent>(event.data);
            std::cout << "[PresentationLayer] NEURALGAMBIT_REVEAL: cards "
                      << e.cardId1 << " & " << e.cardId2
                      << " -> boost card " << e.boostCardId
                      << " +" << e.boostAmount << std::endl;
            CardVisual& boostCv = visualState.getCardVisual(e.boostCardId);
            sf::Vector2f pos = boostCv.cardSprite.getPosition();
            animationManager.spawnFloatingText(
                "+" + std::to_string(e.boostAmount),
                { pos.x, pos.y - 20.f },
                sf::Color(255, 215, 0),
                1.5f
            );
        } break;

        case GameEventType::CLEAR_INPUT: {
            std::cout << "[PresentationLayer] CLEAR_INPUT" << std::endl;
            uiManager.clearInput();
        } break;

        } // switch

        // Cutscene event — stop processing, remaining events wait for animation
        if (isCutscene(event.type)) {
            cutsceneBlocking = true;
            break;
        }
    } // while
}
