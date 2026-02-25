#include "BattlePhase.h"
#include "../gameEngine/Game.h"

void BattlePhase::onEnter() {
    std::cout << "\n=== ENTERING BATTLE PHASE ===\n" << std::endl;

    Player& currentPlayer = getCurrentPlayer();
    roundManager.updateGameState(PhaseName::BATTLE_PHASE,currentPlayer.getId());
}


std::optional<PhaseName> BattlePhase::onUpdate(){

    //get host player and current player
    Player& host = roundManager.getHostPlayer();
    Player& opponent = getCurrentPlayer();

    if (host.getId() == opponent.getId()) {
        throw std::runtime_error("Host player cannot battle themselves.");
    }
    //calculate hand values
    int opponentHandValue = opponent.calculateHandValue();
    int hostHandValue = host.calculateHandValue() ;

    std::cout << "[BattlePhase] HOST [" << hostHandValue
                << "] vs Player " << opponent.getId() 
                <<" [" << opponentHandValue << "]" << std::endl;
    //start battle, reprint hand value after host's turn since manipulation might
    //have happenned here
    roundManager.updateGameState(PhaseName::BATTLE_PHASE,opponent.getId());
    host.flipAllCardsFaceUp();
    opponentHandValue = opponent.calculateHandValue();
    hostHandValue = host.calculateHandValue() ;
    std::cout << "[BattlePhase] HOST [" << hostHandValue
                << "] vs Player " << opponent.getId() <<" [" << opponentHandValue <<"]"<< std::endl;
    
    //determine outcome
    if (hostHandValue > GameConfig::BLACKJACK_VALUE){
        std::cout << "[BattlePhase] Host BUSTS" << std::endl;
        host.gainLoss();
        opponent.gainPoint();
    }
    else if (opponentHandValue > GameConfig::BLACKJACK_VALUE){
        std::cout << "[BattlePhase] Player " << opponent.getId() << " BUSTS!" << std::endl;
        host.gainPoint();
        opponent.gainLoss();
    }
    else if (hostHandValue > opponentHandValue){
        std::cout << "[BattlePhase] Host WINS " << std::endl;
        host.gainPoint();
        opponent.gainLoss();
    }
    else if (hostHandValue < opponentHandValue){
        std::cout << "[BattlePhase] Player " << opponent.getId() << " WINS" << std::endl;
        opponent.gainPoint();
        host.gainLoss();
    }
    else {
        host.gainLoss();
        std::cout << "[BattlePhase] TIE " << opponent.getId() << std::endl;
    }
    host.gainBattleCount();
    opponent.gainBattleCount();


    //after battle, indexing next player
    incrementCurrentPlayerId();

    if (getCurrentPlayerId() == -1){
        std::cout<< "[BattlePhase] All battles resolved for round " << round << "." << std::endl;
        std::cout << "Moving to ROUND_END phase." << std::endl;
        roundManager.updateGameState(PhaseName::ROUND_END, 0);
        return PhaseName::ROUND_END; 
    }

    //go back to host hit after battle
    roundManager.updateGameState(PhaseName::HOST_HIT_PHASE, getCurrentPlayer().getId());
    return PhaseName::HOST_HIT_PHASE;
}


void BattlePhase::onExit() {
    std::cout << "\n=== EXITING BATTLE PHASE ===\n" << std::endl;
}