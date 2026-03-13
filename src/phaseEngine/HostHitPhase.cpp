#include "HostHitPhase.h"
#include "../gameEngine/Game.h"

void HostHitPhase::onEnter() {
    std::cout << "\n=== ENTERING HOST HIT PHASE ===\n" << std::endl;


    //get host player and current player
    Player& hostPlayer = roundManager.getHostPlayer();
    Player& currentPlayer = getCurrentPlayer();

    //spawn text
    std::string turnText = 
    "HOST " + std::to_string(hostPlayer.getId()) 
    + " PHASE";

    eventQueue.push({GameEventType::PHASE_ANNOUNCED, 
        PhaseAnnouncedEvent{turnText, AnimConfig::PHASE_TEXT_DURATION}});

    //set callback for host action input during host hit phase
    uiManager.onActionChosen = [&](PlayerAction chosenAction){
        std::cout << "[HostHitPhase] Host " << hostPlayer.getId() << " chose action: " 
                        << (chosenAction == PlayerAction::HIT ? 
                            "HIT" : chosenAction == PlayerAction::STAND ? "STAND" : "SKILL_REQUEST") 
                        << std::endl;

        hostPlayer.setPendingAction(chosenAction);
    
        // If host chooses skill, switch to targeting mode immediately
        if (chosenAction == PlayerAction::SKILL_REQUEST) {
            // Switch to targeting mode
            //UI prompt for skill target input
            uiManager.requestTargetInput(hostPlayer.getId());
        }
    };
    
    //populate callback for skill target input during host hit phase
    uiManager.onTargetChosen = [&](PlayerTargeting chosenTarget){
        if (chosenTarget.targetPlayerIds.empty() && chosenTarget.targetCards.empty()){
            eventQueue.push({GameEventType::REQUEST_ACTION_INPUT, RequestActionInputEvent{hostPlayer.getId()}});
            getCurrentPlayer().setPendingAction(PlayerAction::IDLE);
        }
        else {
            std::cout << "[uiManager.onTargetChosen] Host " << hostPlayer.getId() << " chose skill target: " 
                        << ((int)chosenTarget.targetPlayerIds.size() > 0 ? 
                            "Player " + chosenTarget.targetPlayerIds[0] : 
                            "Card " + chosenTarget.targetCards[0].getRankAsString() 
                            + " of " 
                            + chosenTarget.targetCards[0].getSuitAsString())
                        << std::endl;
            gameState.pendingTarget = chosenTarget;
        }
    };

    //UI prompt for host action input (queued after PHASE_ANNOUNCED so it shows after animation)
    eventQueue.push({GameEventType::REQUEST_ACTION_INPUT, RequestActionInputEvent{hostPlayer.getId()}});

    //update player info in game state
    roundManager.updateGameState(PhaseName::HOST_HIT_PHASE,currentPlayer.getId());

    //check and execute skill passive if existed
    if(skillManager.skillPassiveHandler(this->gameState)){
        std::cout << "[HostHitPhase] Passive handler done" << std::endl;
        roundManager.updateGameState(PhaseName::HOST_HIT_PHASE,currentPlayer.getId());
    }
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
    // run [turnHandler], if turnHandler returns false -> player stands -> move to next player
    else if(!turnHandler(hostPlayer, currentPlayer)){

        roundManager.updateGameState(PhaseName::BATTLE_PHASE, currentPlayer.getId());
        return PhaseName::BATTLE_PHASE;
    }

    return std::nullopt;
}

void HostHitPhase::onExit() {
    std::cout << "\n=== EXITING HOST HIT PHASE ===\n" << std::endl;
    uiManager.clearInput();
}