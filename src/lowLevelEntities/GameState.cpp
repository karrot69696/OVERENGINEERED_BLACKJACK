#include "GameState.h"

void GameState::setPhaseName(PhaseName newPhaseName,int newCurrentPlayerId){
    phase = newPhaseName;
    currentPlayerId = newCurrentPlayerId;
    if (newCurrentPlayerId == 0) playersProcessed = 0;
}
PhaseName GameState::getPhaseName() const{
    return phase;
}

void GameState::setAllPlayerInfo(std::vector<PlayerInfo> allPlayersInfo){
    playersInfo=allPlayersInfo;
}

void GameState::printAllInfo(){
    std::cout << "========================================" << std::endl;
    std::cout << "GAME STATE INFORMATION" << std::endl;
    std::cout << "========================================" << std::endl;

    std::cout << "Current PhaseName       : " << phaseToString(phase) << std::endl;
    std::cout << "Current Player ID   : " << currentPlayerId << std::endl;
    std::cout << "Deck Count          : " << getDeckCount() << std::endl;

    std::cout << std::endl;
    std::cout << "------------ Players Info ------------" << std::endl;

    for (const auto& player : playersInfo) {
        std::cout << "Player ID     : " << player.playerId << std::endl;
        std::cout << "Cards in hand:" << std::endl;
        for (const auto& card : player.cardsInHand) {
            std::cout   << card.getRankAsString()
                        << " of "
                        << card.getSuitAsString()
                        << ( card.isFaceUp() ? " (f.up)" : " (f.down)" )
                        << std::endl;     
        }
        std::cout << "Skill         : " << skillNameToString(player.skill) << std::endl;
        std::cout << "Skill Uses    : " << player.skillUses << std::endl;
        std::cout << "Points        : " << player.points << std::endl;
        std::cout << "--------------------------------------" << std::endl;
    }

    std::cout << std::endl;
    std::cout << "------------ Revealed Cards ------------" << std::endl;
    std::cout << "========================================" << std::endl;
}