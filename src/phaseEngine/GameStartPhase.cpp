#include "GameStartPhase.h"
#include "../gameEngine/Game.h"

void GameStartPhase::onEnter() {
    std::cout << "\n=== ENTERING GAME START PHASE ===\n" << std::endl;
    eventQueue.push({GameEventType::PHASE_ANNOUNCED, PhaseAnnouncedEvent{"GAME START", AnimConfig::PHASE_TEXT_DURATION}});
}


std::optional<PhaseName> GameStartPhase::onUpdate(){
    //shuffle deck
    deck.shuffle();
    // Deal one card per update call; animation blocking in RoundManager::update()
    // ensures each card animates before the next is dealt

    if (dealRound < GameConfig::HAND_START_VALUE) {
        Player& player = players[dealPlayer];

        //logic
        Card* drawnCard = deck.draw();
        player.addCardToHand(drawnCard);
        roundManager.updateGameState(PhaseName::GAME_START_PHASE, player.getId());

        //emit draw event
        eventQueue.push({GameEventType::CARD_DRAWN, CardDrawnEvent{
            player.getId(),
            player.getHandSize() - 1,
            drawnCard->getId()
        }});

        // advance to next player, wrap to next round
        dealPlayer++;
        if (dealPlayer >= (int)players.size()) {
            dealPlayer = 0;
            dealRound++;
        }

        return std::nullopt; // stay in this phase
    }

    roundManager.updateGameState(PhaseName::BLACKJACK_CHECK_PHASE, 0);
    return PhaseName::BLACKJACK_CHECK_PHASE;
}


void GameStartPhase::onExit() {
    //reset current player index to 0 for next phase
    std::cout << "\n=== EXITING GAME START PHASE ===\n" << std::endl;
}