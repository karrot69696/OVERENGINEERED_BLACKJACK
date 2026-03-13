#include "Phase.h"
#include "../gameEngine/Game.h"

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

    if(player._isBot()){
       player.setPendingAction(player.botMode(gameState));
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
        } break;

        case PlayerAction::SKILL_REQUEST:

            if((int)gameState.pendingTarget.targetPlayerIds.size() > 0 ||
            (int)gameState.pendingTarget.targetCards.size() > 0){
                std::cout << "[Phase][turnHandler] Processing skill for player " << player.getId() << std::endl;
                skillHandler(player);

                roundManager.updateGameState(
                    player.getHost() ? PhaseName::HOST_HIT_PHASE : PhaseName::PLAYER_HIT_PHASE,
                    player.getHost() ? opponent.getId() : player.getId()
                );
            }
        break;

        case PlayerAction::STAND:
            roundManager.updateGameState(
                player.getHost() ? PhaseName::HOST_HIT_PHASE : PhaseName::PLAYER_HIT_PHASE,
                player.getHost() ? opponent.getId() : player.getId()
            );
            player.setPendingAction(PlayerAction::IDLE);
            return false;
        break;

        case PlayerAction::IDLE:
        break;
    }

    return true;
}

void Phase::skillHandler(Player& player){

    std::vector<Player*> actualTargets;
    std::vector<Card*> actualTargetCards;

    for (int id : gameState.pendingTarget.targetPlayerIds) {
        for (auto& p : players) {
            if (p.getId() == id) {
                actualTargets.push_back(&p);
            }
        }
    }

    for (Card card : gameState.pendingTarget.targetCards) {
        for (auto& player : players) {
            for (int i = 0; i < player.getHandSize(); i++) {
                Card* playerCard = player.getCardInHand(i);
                if (playerCard->getSuit() == card.getSuit() && playerCard->getRank() == card.getRank()) {
                    std::cout << "[skillHandler] Found target card: "
                    << playerCard->getRankAsString()
                    << playerCard->getSuitAsString()
                    << " in player "
                    << player.getId() << "'s hand." << std::endl;
                    actualTargetCards.push_back(playerCard);
                }
            }
        }
    }

    std::vector<int> targetCardIds;
    for (Card* card : actualTargetCards) {
        targetCardIds.push_back(card->getId());
    }

    SkillContext context{
        player,
        actualTargets,
        actualTargetCards,
        deck,
        gameState
    };

    if(!skillManager.processSkill(context)){
        std::cout << "[skillHandler] Skill processing failed for player " << player.getId() << std::endl;
    } else {
        roundManager.updateGameState(gameState.getPhaseName(), player.getId());
        for (int cardId : targetCardIds) {
            for (auto* card : deck.getCards()) {
                if (card->getId() == cardId && card->getOwnerId() == -1) {
                    // Emit spin event — PresentationLayer handles the full chain:
                    // deliverance effect + spin → return to deck → reposition
                    eventQueue.push({GameEventType::CARD_SPIN, CardSpinEvent{
                        cardId, player.getId()
                    }});
                    break;
                }
            }
        }
    }

    gameState.pendingTarget = {};
    eventQueue.push({GameEventType::REQUEST_ACTION_INPUT, RequestActionInputEvent{player.getId()}});
    player.setPendingAction(PlayerAction::IDLE);
}
