#include "PlayerHitPhase.h"
#include "../gameEngine/Game.h"
////////////////////////////////////////////////
//                  ON ENTER
////////////////////////////////////////////////
void PlayerHitPhase::onEnter() {
    std::cout << "\n=== ENTERING PLAYER HIT PHASE ===\n" << std::endl;

    //get current player
    Player& currentPlayer = getCurrentPlayer();

    //spawn text
    std::string turnText = "PLAYER " + std::to_string(currentPlayer.getId());
    eventQueue.push({GameEventType::PHASE_ANNOUNCED, PhaseAnnouncedEvent{turnText, AnimConfig::PHASE_TEXT_DURATION}});

    //update player info in game state
    roundManager.updateGameState(PhaseName::PLAYER_HIT_PHASE,currentPlayer.getId());

    //UI prompt for player action input (callbacks wired once in Game.cpp)
    eventQueue.push({GameEventType::REQUEST_ACTION_INPUT, RequestActionInputEvent{currentPlayer.getId()}});
}

////////////////////////////////////////////////
//                  ON UPDATE
////////////////////////////////////////////////
std::optional<PhaseName> PlayerHitPhase::onUpdate() {

    

    //get current player
    Player& currentPlayer = getCurrentPlayer();
 
    // skip player that is not host or blackjacked
    if (currentPlayer.getHost() == 1 || currentPlayer.getBlackJacked() == 1){

        incrementCurrentPlayerId();
        //if all players have been checked, reset to first player and move to next phase
        if (getCurrentPlayerId() == -1){
            roundManager.updateGameState(PhaseName::HOST_HIT_PHASE, 0);
            return PhaseName::HOST_HIT_PHASE;
        }
        return PhaseName::PLAYER_HIT_PHASE;
    }

    // if turnHandler returns false -> player stands -> move to next player
    else if(!turnHandler(currentPlayer, currentPlayer)){
        std::cout << "[playerHitHandler] Player " << currentPlayer.getId() << " has finished their turn." << std::endl;
        
        //indexing next player
        incrementCurrentPlayerId();

        //if all players have been checked, reset to first player and move to next phase
        if (getCurrentPlayerId()== -1){
            std::cout << "[playerHitHandler] Moving to HOST_HIT_PHASE\n";
            roundManager.updateGameState(PhaseName::HOST_HIT_PHASE, 0);
            return PhaseName::HOST_HIT_PHASE;
        }
    }
    return std::nullopt ;
}

////////////////////////////////////////////////
//                  ON EXIT
////////////////////////////////////////////////
void PlayerHitPhase::onExit() {
    std::cout << "\n=== EXITING PLAYER HIT PHASE ===\n" << std::endl;
    uiManager.clearInput();
}