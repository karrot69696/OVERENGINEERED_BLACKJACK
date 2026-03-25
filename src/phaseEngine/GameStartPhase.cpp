#include "GameStartPhase.h"
#include "../gameEngine/Game.h"
#include <random>

void GameStartPhase::onEnter() {
    std::cout << "\n=== ENTERING GAME START PHASE ===\n" << std::endl;

    // Randomly reassign skills each round if enabled
    if (GameSettings::instance().randomSkillsEachRound) {
        static const std::vector<SkillName> availableSkills = {
            SkillName::DELIVERANCE, SkillName::NEURALGAMBIT,
            SkillName::FATALDEAL, SkillName::CHRONOSPHERE,
            SkillName::DESTINYDEFLECT
        };
        std::vector<SkillName> pool = availableSkills;
        static std::mt19937 rng(std::random_device{}());
        std::shuffle(pool.begin(), pool.end(), rng);

        for (int i = 0; i < (int)players.size(); i++) {
            SkillName assigned = pool[i % pool.size()];
            players[i].setSkillName(assigned);
            std::cout << "[GameStartPhase] P" << players[i].getId()
                      << " assigned " << gameState.skillNameToString(assigned) << std::endl;
        }
        skillManager.recreateSkills(players);
    }

    eventQueue.push({GameEventType::PHASE_ANNOUNCED, PhaseAnnouncedEvent{"GAME START", AnimConfig::PHASE_TEXT_DURATION}});
    if (GameSettings::instance().randomSkillsEachRound){
        eventQueue.push({GameEventType::PHASE_ANNOUNCED, PhaseAnnouncedEvent{"CHAOS PREVAILS! >_<", AnimConfig::PHASE_TEXT_DURATION}});
    }
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

        //emit draw event BEFORE updateGameState so event+state are consistent
        eventQueue.push({GameEventType::CARD_DRAWN, CardDrawnEvent{
            player.getId(),
            player.getHandSize() - 1,
            drawnCard->getId()
        }});

        roundManager.updateGameState(PhaseName::GAME_START_PHASE, player.getId());

        // advance to next player, wrap to next round
        dealPlayer++;
        if (dealPlayer >= (int)players.size()) {
            dealPlayer = 0;
            dealRound++;
        }
        std::cout<<"\nstaying in GAME_START_PHASE...."<<std::endl;
        return std::nullopt; // stay in this phase
    }


        auto getRandomDealerText = []() -> std::string {
        static const std::vector<std::string> texts = {
            "New host - The table chooses you.",
            "New host - All eyes on the dealer.",
            "New host - You hold the house now.",
            "New host - Fate appoints its dealer.",
            "New host - The game flows through you.",
            "New host - The house has a new face.",
            "New host - You are the hand behind the deck.",
            "New host - Cards answer to you now.",
            "New host - The deck bends to your will.",
            "New host - You deal… destiny follows.",
            "New host - Control the table.",
            "New host - Command granted.",
            "New host - You set the pace.",
            "New host - Authority established.",
            "New host - The round begins with you.",
            "New host - Destiny names its dealer.",
            "New host - The table awaits your command.",
            "New host - From your hands, chaos or order.",
            "New host - The game breathes at your signal.",
            "New host - Congrats—you’re the problem now."
        };

        static std::mt19937 rng(std::random_device{}());
        return texts[std::uniform_int_distribution<>(0, texts.size() - 1)(rng)];
    };
    eventQueue.push({GameEventType::POINT_CHANGED, PointChangedEvent{gameState.getHostPlayerId(), getRandomDealerText()}});
    roundManager.updateGameState(PhaseName::BLACKJACK_CHECK_PHASE, 0);
    return PhaseName::BLACKJACK_CHECK_PHASE;
}


void GameStartPhase::onExit() {
    //reset current player index to 0 for next phase
    std::cout << "\n=== EXITING GAME START PHASE ===\n" << std::endl;
}