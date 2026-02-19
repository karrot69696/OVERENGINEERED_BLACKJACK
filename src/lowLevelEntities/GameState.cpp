#include "GameState.h"

std::string GameState::revealCardRankToString(const RevealedCardInfo& info) {
    if (info.rank == Rank::Ace) return "A";
    if (info.rank == Rank::Two) return "2";
    if (info.rank == Rank::Three) return "3";
    if (info.rank == Rank::Four) return "4";
    if (info.rank == Rank::Five) return "5";
    if (info.rank == Rank::Six) return "6";
    if (info.rank == Rank::Seven) return "7";
    if (info.rank == Rank::Eight) return "8";
    if (info.rank == Rank::Nine) return "9";
    if (info.rank == Rank::Ten) return "10";
    if (info.rank == Rank::Jack) return "J";
    if (info.rank == Rank::Queen) return "Q";
    if (info.rank == Rank::King) return "K"; 
    return "?";   
}

std::string GameState::revealCardSuitToString(const RevealedCardInfo& info) {
    if (info.suit == Suit::Hearts) return "H";
    if (info.suit == Suit::Diamonds) return "D";
    if (info.suit == Suit::Clubs) return "C";
    if (info.suit == Suit::Spades) return "S";
    return "?";   
}

void GameState::updatePhase(Phase newPhase,int newCurrentPlayerId){
    phase = newPhase;
    currentPlayerId = newCurrentPlayerId;
}

void GameState::updatePhase(Phase newPhase){
    phase = newPhase;
}
Phase GameState::getPhase(){
    return phase;
}

void GameState::updateAllPlayerInfo(std::vector<PlayerInfo> allPlayersInfo){
    playersInfo=allPlayersInfo;
}

void GameState::printAllInfo(){
    std::cout << "========================================" << std::endl;
    std::cout << "GAME STATE INFORMATION" << std::endl;
    std::cout << "========================================" << std::endl;

    std::cout << "Current Phase       : " << phaseToString(phase) << std::endl;
    std::cout << "Current Player ID   : " << currentPlayerId << std::endl;
    std::cout << "Deck Count          : " << deckCount << std::endl;

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