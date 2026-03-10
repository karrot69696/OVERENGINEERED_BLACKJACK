#include "RoundEndPhase.h"
#include "../gameEngine/Game.h"

void RoundEndPhase::onEnter() {
    std::cout << "\n=== ENTERING ROUND END PHASE ===\n" << std::endl;
}


std::optional<PhaseName> RoundEndPhase::onUpdate(){

    roundManager.incrementRound();

    if (roundManager.getRound() >= (int)players.size()){
        std::cout << "[RoundEndPhase] Maximum rounds reached. Ending game..." << std::endl;
        roundManager.updateGameState(PhaseName::GAME_OVER, 0);
        return PhaseName::GAME_OVER;
    }
    
    //New round
    std::cout << "[RoundEndPhase] Starting new round..." << std::endl;
    Player& hostPlayer = roundManager.getHostPlayer();

    //give host to next player
    players[hostPlayer.getId() + 1].setHost();

    std::cout << "[roundEndHandler] Player " 
    << hostPlayer.getId() + 1
    << " is the new host." << std::endl;

    hostPlayer.resetHost();

    //change all cards back to normal  //TO DO

    //reset skill uses
    skillManager.resetSkillUses(players);
    std::cout<<"[RoundEndPhase] Skill uses reset\n";

    //All players return cards to deck + reset blackjack
    for (auto& player : players){
        player.returnCards(deck);
        player.blackJackSet(false);
    }

    std::cout<<"[RoundEndPhase] Cards returned\n";

    deck.flipAllCardsFaceDown();
    deck.sortDeck();
    //deck.printDeck();

    //redeal cards
    deck.shuffle();
    roundManager.dealCardsToPlayers(2);


    roundManager.updateGameState(PhaseName::BLACKJACK_CHECK_PHASE, 0);

    return PhaseName::BLACKJACK_CHECK_PHASE;
}


void RoundEndPhase::onExit() {
    std::cout << "\n=== EXITING ROUND END PHASE ===\n" << std::endl;
}