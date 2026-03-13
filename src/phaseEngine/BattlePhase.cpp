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

    //helper to get text position offset from seat
    auto getTextPos = [&](Player& p) -> sf::Vector2f {
        sf::Vector2f seat = visualState.getPlayerSeatPos(p.getId(), players.size());
        return {seat.x - 90.f, seat.y - 40.f};
    };
    auto getSeatPos = [&](Player& p) -> sf::Vector2f {
        sf::Vector2f seat = visualState.getPlayerSeatPos(p.getId(), players.size());
        return {seat.x - 50.f, seat.y - 10.f};
    };
    sf::Color textColor(245, 224, 32);

    //helper: award win to winner, loss to loser, play animations
    auto resolveWin = [&](Player& winner, Player& loser, const std::string& msg){
        std::cout << "[BattlePhase] " << msg << std::endl;
        winner.gainPoint();
        loser.gainLoss();
        animationManager.spawnFloatingText("+1 POINT", getTextPos(winner), textColor, AnimConfig::POINT_CHANGE_DURATION);
        animationManager.playShockAnimation(winner.getId(), loser.getId(), 0.7f);
        animationManager.playExplosionAnimation(getSeatPos(loser),3.0f, 0.8f);
    };

    //determine outcome
    bool hostBust = hostHandValue > GameConfig::BLACKJACK_VALUE;
    bool oppBust  = opponentHandValue > GameConfig::BLACKJACK_VALUE;

    if (hostBust) {
        resolveWin(opponent, host, "Host BUSTS");
    } else if (oppBust) {
        resolveWin(host, opponent, "Player " + std::to_string(opponent.getId()) + " BUSTS!");
    } else if (hostHandValue > opponentHandValue) {
        resolveWin(host, opponent, "Host WINS");
    } else if (hostHandValue < opponentHandValue) {
        resolveWin(opponent, host, "Player " + std::to_string(opponent.getId()) + " WINS");
    } else {
        std::cout << "[BattlePhase] TIE " << opponent.getId() << std::endl;
        host.gainLoss();
        animationManager.spawnFloatingText("+0", getTextPos(opponent), textColor, AnimConfig::POINT_CHANGE_DURATION);
        animationManager.spawnFloatingText("+0", getTextPos(host), textColor, AnimConfig::POINT_CHANGE_DURATION);
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