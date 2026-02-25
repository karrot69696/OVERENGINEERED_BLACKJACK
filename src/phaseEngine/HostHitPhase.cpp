#include "HostHitPhase.h"
#include "../gameEngine/Game.h"

void HostHitPhase::onEnter() {
    std::cout << "\n=== ENTERING HOST HIT PHASE ===\n" << std::endl;

    //get host player and current player
    Player& hostPlayer = roundManager.getHostPlayer();
    Player& currentPlayer = getCurrentPlayer();

    //update player info in game state
    roundManager.updateGameState(PhaseName::HOST_HIT_PHASE,currentPlayer.getId());
}

std::optional<PhaseName> HostHitPhase::onUpdate() {

    //get host player and current player
    Player& hostPlayer = roundManager.getHostPlayer();
    Player& currentPlayer = getCurrentPlayer();

    //prepare for battle, update game state, turn handler
    currentPlayer.flipAllCardsFaceUp();

    // handle player that is not host or blackjacked
    if (currentPlayer.getHost() == 1 || currentPlayer.getBlackJacked() == 1){

        incrementCurrentPlayerId();
        //if all players have been checked, reset to first player and move to next phase
        if (getCurrentPlayerId() == -1){
            roundManager.updateGameState(PhaseName::ROUND_END, 0);
            return PhaseName::ROUND_END;
        }
    }
    else if(!roundManager.turnHandler(hostPlayer, currentPlayer)){

        roundManager.updateGameState(PhaseName::BATTLE_PHASE, currentPlayer.getId());
        return PhaseName::BATTLE_PHASE;
    }

    return std::nullopt;
}

void HostHitPhase::onExit() {
    std::cout << "\n=== EXITING HOST HIT PHASE ===\n" << std::endl;
}