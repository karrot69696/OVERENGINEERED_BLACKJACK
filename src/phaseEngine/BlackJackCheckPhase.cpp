#include "BlackJackCheckPhase.h"
#include "../gameEngine/Game.h"

void BlackJackCheckPhase::onEnter() {
    std::cout << "\n=== ENTERING BLACKJACK CHECK PHASE ===\n" << std::endl;
    
    std::cout << "ROUND: " << roundManager.getRound() << std::endl;
    Player& currentPlayer = getCurrentPlayer();


    //spawn text
    std::string turnText = "BLACKJACK CHECK";
    eventQueue.push({GameEventType::PHASE_ANNOUNCED, PhaseAnnouncedEvent{turnText, AnimConfig::PHASE_TEXT_DURATION}});

    roundManager.updateGameState(PhaseName::BLACKJACK_CHECK_PHASE,currentPlayer.getId());

    // auto func = [&](float t) { 
    //     currentPlayer.flipAllCardsFaceUp(); 
    // };
    // Animation phaseTransitionAnimation = { func, 0, 1 };
    // animationManager.add(phaseTransitionAnimation);

}


std::optional<PhaseName> BlackJackCheckPhase::onUpdate(){

    

    Player& currentPlayer = getCurrentPlayer();

    // check if current player has blackjack
    if (currentPlayer.calculateHandValue() == 21 && currentPlayer.getHandSize() == 2){
        if (currentPlayer.getHost() == 1){

            std::cout << "[BlackJackCheckPhase] Host player has Black Jack! Host player wins the round!" << std::endl;
            currentPlayer.gainPoint(GameConfig::POINTS_GAIN_BLACKJACK);
            eventQueue.push({GameEventType::POINT_CHANGED, PointChangedEvent{
                currentPlayer.getId(), "BLACKJACK! +" + std::to_string((int)players.size()+2)}});
            roundManager.updateGameState(PhaseName::ROUND_END, currentPlayer.getId());
            return PhaseName::ROUND_END;
        }
        else {

            std::cout << "[BlackJackCheckPhase] Player " << currentPlayer.getId() << " has Black Jack! +2 points" << std::endl;
            std::vector<int> revealIds;
            for (int i = 0; i < currentPlayer.getHandSize(); i++) {
                Card* c = currentPlayer.getCardInHand(i);
                if (!c->isFaceUp()) revealIds.push_back(c->getId());
            }
            currentPlayer.flipAllCardsFaceUp();
            roundManager.updateGameState(PhaseName::BLACKJACK_CHECK_PHASE, currentPlayer.getId());
            if (!revealIds.empty()) {
                eventQueue.push({GameEventType::CARDS_REVEALED, CardsRevealedEvent{revealIds}});
            }
            currentPlayer.gainPoint(GameConfig::POINTS_GAIN_BLACKJACK);
            currentPlayer.blackJackSet();
            eventQueue.push({GameEventType::POINT_CHANGED, PointChangedEvent{
                currentPlayer.getId(), "BLACKJACK! +3"}});
        }
    }

    //increment player id for next check
    incrementCurrentPlayerId();

    //if all players have been checked move to next phase
    if ( allPlayersProcessed() ){
        std::cout << "[BlackJackCheckPhase] All players checked for blackjack. Moving to PLAYER_HIT_PHASE." << std::endl;
        roundManager.updateGameState(PhaseName::PLAYER_HIT_PHASE, 0);
        return PhaseName::PLAYER_HIT_PHASE;
    }
    
    return std::nullopt;
}


void BlackJackCheckPhase::onExit() {
    //reset current player index to 0 for next phase
    std::cout << "\n=== EXITING BLACKJACK CHECK PHASE ===\n" << std::endl;
}