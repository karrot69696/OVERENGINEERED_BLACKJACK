#include "RoundEndPhase.h"
#include "../gameEngine/Game.h"

void RoundEndPhase::onEnter() {
    std::cout << "\n=== ENTERING ROUND END PHASE ===\n" << std::endl;
    animationManager.spawnPhaseText("ROUND END",AnimConfig::PHASE_TEXT_DURATION);

    roundManager.incrementRound();

    if (roundManager.getRound() >= (int)players.size()){
        std::cout << "[RoundEndPhase] Maximum rounds reached. Ending game..." << std::endl;
        roundManager.updateGameState(PhaseName::GAME_OVER, 0);
        return;
    }

    //New round setup (runs once)
    std::cout << "[RoundEndPhase] Starting new round..." << std::endl;
    Player& hostPlayer = roundManager.getHostPlayer();

    //give host to next player
    players[hostPlayer.getId() + 1].setHost();
    std::cout << "[roundEndHandler] Player "
    << hostPlayer.getId() + 1
    << " is the new host." << std::endl;
    hostPlayer.resetHost();

    //reset skill uses
    skillManager.resetSkillUses(players);
    std::cout<<"[RoundEndPhase] Skill uses reset\n";

    // Snapshot card IDs per player BEFORE returning them
    playerCardIds.clear();
    for (auto& player : players){
        std::vector<int> ids;
        for (int i = 0; i < player.getHandSize(); i++){
            ids.push_back(player.getCardInHand(i)->getId());
        }
        playerCardIds.push_back(ids);
    }

    // Return all cards to deck + reset blackjack
    for (auto& player : players){
        player.returnCards(deck);
        player.blackJackSet(false);
    }

    returnPlayerIdx = 0;
    returnCardIdx = 0;
}


std::optional<PhaseName> RoundEndPhase::onUpdate(){

    // If game over was set in onEnter
    if (roundManager.getRound() >= (int)players.size()){
        return PhaseName::GAME_OVER;
    }

    // Animate one card returning per update call
    if (returnPlayerIdx < (int)playerCardIds.size()) {
        auto& cardIds = playerCardIds[returnPlayerIdx];

        if (returnCardIdx < (int)cardIds.size()) {
            animationManager.addReturnToDeckAnimation(cardIds[returnCardIdx]);

            returnCardIdx++;
            if (returnCardIdx >= (int)cardIds.size()) {
                returnCardIdx = 0;
                returnPlayerIdx++;
            }
            return std::nullopt; // stay in this phase
        }

        // Player had no cards, skip to next
        returnCardIdx = 0;
        returnPlayerIdx++;
        return std::nullopt;
    }

    // All cards returned, prepare next round
    std::cout<<"[RoundEndPhase] Cards returned\n";
    deck.flipAllCardsFaceDown();
    deck.sortDeck();
    deck.shuffle();
    roundManager.updateGameState(PhaseName::GAME_START_PHASE, 0);
    return PhaseName::GAME_START_PHASE;
}


void RoundEndPhase::onExit() {
    std::cout << "\n=== EXITING ROUND END PHASE ===\n" << std::endl;
}
