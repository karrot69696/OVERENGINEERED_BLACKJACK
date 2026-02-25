#include "PlayerHitPhase.h"
#include "../gameEngine/Game.h"

void PlayerHitPhase::onEnter() {
    std::cout << "\n=== ENTERING PLAYER HIT PHASE ===\n" << std::endl;

    //get current player
    Player& currentPlayer = getCurrentPlayer();

    //update player info in game state
    roundManager.updateGameState(PhaseName::PLAYER_HIT_PHASE,currentPlayer.getId());
}

std::optional<PhaseName> PlayerHitPhase::onUpdate() {

    //get current player
    Player& currentPlayer = getCurrentPlayer();
    // skip player that is not host or blackjacked
    if (currentPlayer.getHost() == 1 || currentPlayer.getBlackJacked() == 1){

        moveToNextPlayer();
        //if all players have been checked, reset to first player and move to next phase
        if (getCurrentPlayer().getId() == -1){
            roundManager.updateGameState(PhaseName::HOST_HIT_PHASE, 0);
            return PhaseName::HOST_HIT_PHASE;
        }
    }

    // if turnHandler returns false -> player stands -> move to next player
    else if(!roundManager.turnHandler(currentPlayer, currentPlayer)){
        std::cout << "[playerHitHandler] Player " << currentPlayer.getId() << " has finished their turn." << std::endl;
        
        //indexing next player
        moveToNextPlayer();

        //if all players have been checked, reset to first player and move to next phase
        if (getCurrentPlayer().getId() == -1){
            std::cout << "[playerHitHandler] Moving to HOST_HIT_PHASE\n";
            roundManager.updateGameState(PhaseName::HOST_HIT_PHASE, 0);
            return PhaseName::HOST_HIT_PHASE;
        }
    }
    return std::nullopt ;
}

void PlayerHitPhase::onExit() {
    std::cout << "\n=== EXITING PLAYER HIT PHASE ===\n" << std::endl;
    
    //get current player
    Player& hostPlayer = roundManager.getHostPlayer();

    // Set up callback for HOST action input during the next phase
    uiManager.onActionChosen = [&](PlayerAction chosenAction){
        std::cout << "[PlayerHitPhase][onExit] Host " << hostPlayer.getId() << " chose action: " 
                    << (chosenAction == PlayerAction::HIT ? 
                        "HIT" : chosenAction == PlayerAction::STAND ? "STAND" : "SKILL_REQUEST") 
                    << std::endl;

        hostPlayer.setPendingAction(chosenAction);
    };

    //update game state for next phase
    roundManager.updateGameState(PhaseName::HOST_HIT_PHASE, 0);
}