#include "PlayerHitPhase.h"
#include "../gameEngine/Game.h"
////////////////////////////////////////////////
//                  ON ENTER
////////////////////////////////////////////////
void PlayerHitPhase::onEnter() {
    std::cout << "\n=== ENTERING PLAYER HIT PHASE ===\n" << std::endl;

    // Skip host and blackjacked players before pushing any events
    while (!allPlayersProcessed()) {
        Player& p = getCurrentPlayer();
        if (!p.getHost() && !p.getBlackJacked()) break;
        std::cout << "[PlayerHitPhase] Skipping P" << p.getId()
                  << (p.getHost() ? " (host)" : " (blackjacked)") << std::endl;
        incrementCurrentPlayerId();
    }

    // All players skipped — onUpdate will transition to HOST_HIT
    if (allPlayersProcessed()) return;

    Player& currentPlayer = getCurrentPlayer();

    //spawn text
    std::string turnText = "PLAYER " + std::to_string(currentPlayer.getId());
    eventQueue.push({GameEventType::PHASE_ANNOUNCED, PhaseAnnouncedEvent{turnText, AnimConfig::PHASE_TEXT_DURATION}});

    //update player info in game state
    roundManager.updateGameState(PhaseName::PLAYER_HIT_PHASE, currentPlayer.getId());

    // Initialize Chronosphere hand-value tracking baseline (track the CHRONO OWNER's value)
    gameState.chronoTrackedHandValue = -1;
    for (auto& p : players) {
        if (p.getSkillName() == SkillName::CHRONOSPHERE && p.getHandSize() > 0) {
            gameState.chronoTrackedHandValue = p.calculateHandValue();
            break;
        }
    }

    // Fire ON_HIT_PHASE_START reactive check (Chronosphere snapshot prompt)
    // If it fires, reactiveTickPending will push REQUEST_ACTION_INPUT when done
    if (!startReactiveCheck(ReactiveTrigger::ON_HIT_PHASE_START,
            -1, currentPlayer.getId(), currentPlayer.getId())) {
        //UI prompt for player action input (callbacks wired once in Game.cpp)
        eventQueue.push({GameEventType::REQUEST_ACTION_INPUT, RequestActionInputEvent{currentPlayer.getId()}});
    }
}

////////////////////////////////////////////////
//                  ON UPDATE
////////////////////////////////////////////////
std::optional<PhaseName> PlayerHitPhase::onUpdate() {

    

    // All players were skipped in onEnter
    if (allPlayersProcessed()) {
        roundManager.updateGameState(PhaseName::HOST_HIT_PHASE, 0);
        return PhaseName::HOST_HIT_PHASE;
    }

    Player& currentPlayer = getCurrentPlayer();

    // if turnHandler returns false -> player stands -> move to next player
    if (!turnHandler(currentPlayer, currentPlayer)){
        std::cout << "[playerHitHandler] Player " << currentPlayer.getId() << " has finished their turn." << std::endl;
        
        //indexing next player
        incrementCurrentPlayerId();

        //if all players have been checked, reset to first player and move to next phase
        if (allPlayersProcessed()){
            std::cout << "[playerHitHandler] Moving to HOST_HIT_PHASE\n";
            roundManager.updateGameState(PhaseName::HOST_HIT_PHASE, 0);
            return PhaseName::HOST_HIT_PHASE;
        }
        // Re-enter phase for next player (triggers onExit→onEnter with fresh UI prompts)
        roundManager.updateGameState(PhaseName::PLAYER_HIT_PHASE, gameState.getCurrentPlayerId());
        return PhaseName::PLAYER_HIT_PHASE;
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