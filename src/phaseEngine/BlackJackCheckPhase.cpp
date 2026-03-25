#include "BlackJackCheckPhase.h"
#include "../gameEngine/Game.h"

static const std::vector<std::string> bjCheckPhrases = {
    "BLACKJACK! DIDN'T EVEN SHUFFLE PROPERLY",
    "BLACKJACK! SKILL ISSUE FOR EVERYONE ELSE",
    "BLACKJACK! THE TUTORIAL BOSS",
    "BLACKJACK! BORN WITH A SILVER HAND",
    "BLACKJACK! GG GO NEXT",
    "BLACKJACK! THEY INSTALLED HACKS BEFORE ROUND 1",
    "BLACKJACK! IMAGINE NEEDING MORE THAN 2 CARDS",
    "BLACKJACK! ACTUAL MAIN CHARACTER",
    "BLACKJACK! THE AUDACITY",
    "BLACKJACK! DEAL WITH IT. LITERALLY.",
};

void BlackJackCheckPhase::onEnter() {
    std::cout << "\n=== ENTERING BLACKJACK CHECK PHASE ===\n" << std::endl;

    std::cout << "ROUND: " << roundManager.getRound() << std::endl;
    Player& currentPlayer = getCurrentPlayer();


    //spawn text
    std::string turnText = "BLACKJACK CHECK";
    eventQueue.push({GameEventType::PHASE_ANNOUNCED, PhaseAnnouncedEvent{turnText, AnimConfig::PHASE_TEXT_DURATION}});

    roundManager.updateGameState(PhaseName::BLACKJACK_CHECK_PHASE,currentPlayer.getId());

    // auto func = [&](float t) { 
    //     currentPlayer.flipAllCardsFaceUp(); 
    // };
    // Animation phaseTransitionAnimation = { func, 0, 1 };
    // animationManager.add(phaseTransitionAnimation);

}


std::optional<PhaseName> BlackJackCheckPhase::onUpdate(){

    

    Player& currentPlayer = getCurrentPlayer();

    // check if current player has blackjack
    if (currentPlayer.calculateHandValue() == 21 && currentPlayer.getHandSize() == 2){
        if (currentPlayer.getHost() == 1){

            std::cout << "[BlackJackCheckPhase] Host player has Black Jack! Host player wins the round!" << std::endl;
            currentPlayer.gainPoint(GameConfig::POINTS_GAIN_BLACKJACK);
            const std::string& phrase = bjCheckPhrases[std::rand() % bjCheckPhrases.size()];
            eventQueue.push({GameEventType::POINT_CHANGED, PointChangedEvent{
                currentPlayer.getId(), phrase + " +" + std::to_string((int)players.size()+2)}});
            roundManager.updateGameState(PhaseName::ROUND_END, currentPlayer.getId());
            return PhaseName::ROUND_END;
        }
        else {

            std::cout << "[BlackJackCheckPhase] Player " << currentPlayer.getId() << " has Black Jack! +2 points" << std::endl;
            std::vector<int> revealIds;
            for (int i = 0; i < currentPlayer.getHandSize(); i++) {
                Card* c = currentPlayer.getCardInHand(i);
                if (!c->isFaceUp()) revealIds.push_back(c->getId());
            }
            currentPlayer.flipAllCardsFaceUp();
            roundManager.updateGameState(PhaseName::BLACKJACK_CHECK_PHASE, currentPlayer.getId());
            if (!revealIds.empty()) {
                eventQueue.push({GameEventType::CARDS_REVEALED, CardsRevealedEvent{revealIds}});
            }
            currentPlayer.gainPoint(GameConfig::POINTS_GAIN_BLACKJACK);
            currentPlayer.blackJackSet();
            auto currentIds = [&]() {
                std::vector<int> ids;
                for (int i = 0; i < currentPlayer.getHandSize(); i++)
                    ids.push_back(currentPlayer.getCardInHand(i)->getId());
                std::sort(ids.begin(), ids.end());
                return ids;
            }();
            currentPlayer.lastBlackjackHand = currentIds;
            const std::string& phrase2 = bjCheckPhrases[std::rand() % bjCheckPhrases.size()];
            eventQueue.push({GameEventType::POINT_CHANGED, PointChangedEvent{
                currentPlayer.getId(), phrase2 + " +" + std::to_string(GameConfig::POINTS_GAIN_BLACKJACK)}});
        }
    }

    //increment player id for next check
    incrementCurrentPlayerId();

    //if all players have been checked move to next phase
    if ( allPlayersProcessed() ){
        std::cout << "[BlackJackCheckPhase] All players checked for blackjack. Moving to PLAYER_HIT_PHASE." << std::endl;

        // Destiny Deflect passive: peek top 3 deck cards for the owner
        for (auto& player : players) {
            if (player.getSkillName() == SkillName::DESTINYDEFLECT) {
                auto& deckCards = deck.getCards();
                int peekCount = std::min(3, (int)deckCards.size());
                if (peekCount > 0) {
                    DeckPeekEvent peekEvent;
                    peekEvent.playerId = player.getId();
                    for (int i = 0; i < peekCount; i++) {
                        Card* c = deckCards[(int)deckCards.size() - 1 - i]; // top of deck
                        peekEvent.cardIds.push_back(c->getId());
                        peekEvent.ranks.push_back(static_cast<uint8_t>(c->getRank()));
                        peekEvent.suits.push_back(static_cast<uint8_t>(c->getSuit()));
                    }
                    eventQueue.push({GameEventType::DECK_PEEK, peekEvent});
                    std::cout << "[BlackJackCheckPhase] Destiny Deflect passive: P" << player.getId()
                              << " peeks at top " << peekCount << " deck cards" << std::endl;
                }
                break; // only one Destiny Deflect owner
            }
        }

        roundManager.updateGameState(PhaseName::PLAYER_HIT_PHASE, 0);
        return PhaseName::PLAYER_HIT_PHASE;
    }
    
    return std::nullopt;
}


void BlackJackCheckPhase::onExit() {
    //reset current player index to 0 for next phase
    std::cout << "\n=== EXITING BLACKJACK CHECK PHASE ===\n" << std::endl;
}