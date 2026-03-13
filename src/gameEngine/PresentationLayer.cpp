#include "PresentationLayer.h"
#include "AnimationManager.h"
#include "UIManager.h"
#include "../lowLevelEntities/VisualState.h"

PresentationLayer::PresentationLayer(EventQueue& eq, AnimationManager& am,
                                     UIManager& um, VisualState& vs, GameState& gs)
    : eventQueue(eq), animationManager(am), uiManager(um), visualState(vs), gameState(gs)
{}

void PresentationLayer::processEvents() {
    auto events = eventQueue.drain();
    for (auto& event : events) {
        switch (event.type) {

        case GameEventType::CARD_DRAWN: {
            auto& e = std::get<CardDrawnEvent>(event.data);
            // Set visual state BEFORE animation reads positions
            CardVisual& card = visualState.getCardVisual(e.cardId);
            card.location = CardLocation::HAND;
            card.ownerId = e.playerId;
            card.cardIndex = e.handIndex;
            animationManager.addDrawAnimation(e.playerId, e.handIndex, e.cardId);
        } break;

        case GameEventType::CARD_RETURNED: {
            auto& e = std::get<CardReturnedEvent>(event.data);
            // Set visual state BEFORE animation
            CardVisual& card = visualState.getCardVisual(e.cardId);
            card.ownerId = -1;
            card.cardIndex = -1;
            card.location = CardLocation::DECK;
            animationManager.addReturnToDeckAnimation(e.cardId);
        } break;

        case GameEventType::CARD_SPIN: {
            auto& e = std::get<CardSpinEvent>(event.data);
            CardVisual& cv = visualState.getCardVisual(e.cardId);
            int cardId = e.cardId;
            int playerId = e.playerId;

            // Play deliverance VFX alongside spin
            animationManager.playDeliveranceEffect(cv.cardSprite.getPosition());

            // Chain: spin → return to deck → reposition hand
            animationManager.addSpinAnimation(cardId, [this, cardId, playerId](){
                CardVisual& card = visualState.getCardVisual(cardId);
                card.ownerId = -1;
                card.cardIndex = -1;
                card.location = CardLocation::DECK;
                animationManager.addReturnToDeckAnimation(cardId, [this, playerId](){
                    animationManager.repositionHand(playerId);
                });
            });
        } break;

        case GameEventType::HAND_REPOSITIONED: {
            auto& e = std::get<HandRepositionedEvent>(event.data);
            animationManager.repositionHand(e.playerId);
        } break;

        case GameEventType::BATTLE_WIN: {
            auto& e = std::get<BattleWinEvent>(event.data);
            (void)e; // battle logic is in the individual events below
        } break;

        case GameEventType::BATTLE_TIE: {
            auto& e = std::get<BattleTieEvent>(event.data);
            (void)e;
        } break;

        case GameEventType::POINT_CHANGED: {
            auto& e = std::get<PointChangedEvent>(event.data);
            sf::Vector2f seat = visualState.getPlayerSeatPos(
                e.playerId, (int)gameState.getAllPlayerInfo().size());
            sf::Vector2f textPos = {seat.x - 90.f, seat.y - 40.f};
            sf::Color textColor(245, 224, 32);
            animationManager.spawnFloatingText(e.text, textPos, textColor, AnimConfig::POINT_CHANGE_DURATION);
        } break;

        case GameEventType::PHASE_ANNOUNCED: {
            auto& e = std::get<PhaseAnnouncedEvent>(event.data);
            animationManager.spawnPhaseText(e.text, e.duration);
        } break;

        case GameEventType::SHOCK_EFFECT: {
            auto& e = std::get<ShockEffectEvent>(event.data);
            animationManager.playShockAnimation(e.winnerId, e.loserId, e.duration);
        } break;

        case GameEventType::EXPLOSION_EFFECT: {
            auto& e = std::get<ExplosionEffectEvent>(event.data);
            sf::Vector2f seat = visualState.getPlayerSeatPos(
                e.targetPlayerId, (int)gameState.getAllPlayerInfo().size());
            sf::Vector2f pos = {seat.x - 50.f, seat.y - 10.f};
            animationManager.playExplosionAnimation(pos, e.scale, e.duration);
        } break;

        case GameEventType::DELIVERANCE_EFFECT: {
            auto& e = std::get<DeliveranceEffectEvent>(event.data);
            CardVisual& cv = visualState.getCardVisual(e.cardId);
            animationManager.playDeliveranceEffect(cv.cardSprite.getPosition());
        } break;

        case GameEventType::REQUEST_ACTION_INPUT: {
            auto& e = std::get<RequestActionInputEvent>(event.data);
            uiManager.requestActionInput(e.playerId);
        } break;

        case GameEventType::REQUEST_TARGET_INPUT: {
            auto& e = std::get<RequestTargetInputEvent>(event.data);
            uiManager.requestTargetInput(e.playerId);
        } break;

        case GameEventType::CLEAR_INPUT: {
            uiManager.clearInput();
        } break;

        } // switch
    } // for
}
