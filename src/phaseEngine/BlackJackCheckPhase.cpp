#include "BlackJackCheckPhase.h"
#include "../gameEngine/Game.h"

void BlackJackCheckPhase::onEnter() {
    std::cout << "\n=== ENTERING BLACKJACK CHECK PHASE ===\n" << std::endl;
    std::cout << "ROUND: " << roundManager.getRound() << std::endl;
    Player& currentPlayer = getCurrentPlayer();
    roundManager.updateGameState(PhaseName::BLACKJACK_CHECK_PHASE,currentPlayer.getId());
}


std::optional<PhaseName> BlackJackCheckPhase::onUpdate(){

    Player& currentPlayer = getCurrentPlayer();

    // check if current player has blackjack
    if (currentPlayer.calculateHandValue() == 21 && currentPlayer.getHandSize() == 2){
        if (currentPlayer.getHost() == 1){

            std::cout << "[BlackJackCheckPhase] Host player has Black Jack! Host player wins the round!" << std::endl;
            currentPlayer.gainPoint(players.size()-1);
            roundManager.updateGameState(PhaseName::ROUND_END, currentPlayer.getId());
            return PhaseName::ROUND_END;
        }
        else {

            std::cout << "[BlackJackCheckPhase] Player " << currentPlayer.getId() << " has Black Jack! +2 points" << std::endl;
            currentPlayer.flipAllCardsFaceUp();
            currentPlayer.gainPoint(2);
            currentPlayer.blackJackSet();
        }
    }

    //increment player id for next check
    incrementCurrentPlayerId();

    //if all players have been checked move to next phase
    if ( getCurrentPlayerId() == -1 ){
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