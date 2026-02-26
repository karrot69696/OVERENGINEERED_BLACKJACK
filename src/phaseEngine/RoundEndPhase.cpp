#include "RoundEndPhase.h"
#include "../gameEngine/Game.h"

void RoundEndPhase::onEnter() {
    std::cout << "\n=== ENTERING ROUND END PHASE ===\n" << std::endl;
}


std::optional<PhaseName> RoundEndPhase::onUpdate(){

    roundManager.incrementRound();

    if (roundManager.getRound() >= roundManager.getPlayers().size()){
        std::cout << "[RoundEndPhase] Maximum rounds reached. Ending game..." << std::endl;
        roundManager.updateGameState(PhaseName::GAME_OVER, 0);
        return PhaseName::GAME_OVER;
    }
    
    //New round
    std::cout << "[RoundEndPhase] Starting new round..." << std::endl;
    Player& hostPlayer = roundManager.getHostPlayer();
    std::vector<Player>& players = roundManager.getPlayers();


    //give host to next player
    players[hostPlayer.getId() + 1].setHost();

    std::cout << "[roundEndHandler] Player " 
    << hostPlayer.getId() + 1
    << " is the new host." << std::endl;

    hostPlayer.resetHost();

    //change all cards back to normal  //TO DO

    //reset skill uses
    roundManager.getSkillManager().resetSkillUses(players);

    //All players return cards to deck + reset blackjack
    for (auto& player : players){
        player.returnCards(roundManager.getDeck());
        player.blackJackSet(false);
    }

    roundManager.getDeck().flipAllCardsFaceDown();
    roundManager.getDeck().sortDeck();
    //deck.printDeck();

    //redeal cards
    roundManager.getDeck().shuffle();
    roundManager.dealCardsToPlayers(2);


    roundManager.updateGameState(PhaseName::BLACKJACK_CHECK_PHASE, 0);

    return PhaseName::BLACKJACK_CHECK_PHASE;
}


void RoundEndPhase::onExit() {
    std::cout << "\n=== EXITING ROUND END PHASE ===\n" << std::endl;
}