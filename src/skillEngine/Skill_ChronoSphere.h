#ifndef SKILL_CHRONOSPHERE_H
#define SKILL_CHRONOSPHERE_H

#include "Skill.h"
#include <string>
#include <iostream>
#include <unordered_map>

class SkillChronoSphere : public Skill {
public:
    SkillChronoSphere(int user) {
        userId = user;
        uses = 3;
    }

    std::string skillNameToString() const override {
        return "Chronosphere";
    }

    skillType getType() const override {
        return skillType::FUSION;
    }

    SkillName getSkillName() const override {
        return SkillName::CHRONOSPHERE;
    }

    ReactiveTrigger getReactiveTrigger() const override {
        return ReactiveTrigger::ON_HIT_PHASE_START;
    }

    // Active path: click Skill button during your turn
    // Snapshot is free, rewind costs uses — only block if no valid action exists
    std::string canActivate(const GameState& state) override {
        PhaseName phase = state.getPhaseName();
        if (phase != PhaseName::PLAYER_HIT_PHASE && phase != PhaseName::HOST_HIT_PHASE)
            return "CAN ONLY BE USED DURING HIT PHASE";
        PlayerInfo info = state.getPlayerInfo(userId);
        if (info.cardsInHand.empty()) return "NO CARD IN HAND";
        // If snapshot exists and no uses left, can't rewind — but can still re-snapshot
        return "";
    }

    // Reactive path: ON_HIT_PHASE_START (own phase start)
    // Also checked for hand-value-change via tryChronoReactiveCheck (separate system)
    bool canReact(const ReactiveContext& ctx, const GameState& state) override {
        // React at any non-host player's hit phase start (not host hit phase)
        if (state.getPhaseName() != PhaseName::PLAYER_HIT_PHASE) return false;
        // Need cards to snapshot/rewind
        PlayerInfo info = state.getPlayerInfo(userId);
        if (info.cardsInHand.empty()) return false;
        return true;
    }

    // Called after choice is made (pendingChronoChoice determines path)
    std::string canUse(const SkillContext& context) override {
        if (context.state.pendingChronoChoice == ChronoChoice::SNAPSHOT) {
            if (context.user.getHandSize() == 0) return "NO CARDS TO SNAPSHOT";
            return "";
        }

        if (context.state.pendingChronoChoice == ChronoChoice::REWIND) {
            if (uses <= 0) return "OUT OF USES";
            if (!context.state.snapShotTaken) return "NO SNAPSHOT TO REWIND TO";

            // Check if deck has all needed ranks
            std::unordered_map<Rank, int> needed;
            for (auto r : context.state.ranksSnapShot) {
                needed[r]++;
            }
            for (Card* card : context.deck.getCards()) {
                Rank r = card->getRank();
                if (needed.count(r) && needed[r] > 0) {
                    needed[r]--;
                }
            }
            for (auto& [rank, count] : needed) {
                if (count > 0) return "NOT ENOUGH MATCHING CARDS IN DECK";
            }
            return "";
        }

        return "NO ACTION SELECTED";
    }

    // --- SNAPSHOT: copy current hand ranks into gameState (free, no use cost) ---
    bool executeSnapshot(SkillContext& context) {
        context.state.ranksSnapShot.clear();
        for (int i = 0; i < context.user.getHandSize(); i++) {
            context.state.ranksSnapShot.push_back(context.user.getCardInHand(i)->getRank());
        }
        context.state.snapShotTaken = true;

        std::cout << "[Chronosphere] P" << context.user.getId() << " snapshot taken: ";
        for (auto r : context.state.ranksSnapShot) std::cout << static_cast<int>(r) << " ";
        std::cout << std::endl;

        return true;
    }

    // --- REWIND: return all hand cards to deck, then pull matching ranks from deck (costs 1 use) ---
    bool executeRewind(SkillContext& context) {
        // Build frequency map of what we need from the snapshot
        std::unordered_map<Rank, int> needed;
        for (auto r : context.state.ranksSnapShot) {
            needed[r]++;
        }

        // Collect current hand cards to return to deck
        std::vector<Card*> cardsToReturn;
        for (int i = 0; i < context.user.getHandSize(); i++) {
            cardsToReturn.push_back(context.user.getCardInHand(i));
        }

        // Record returned card IDs for aftermath events
        for (auto* c : cardsToReturn) {
            context.returnedCardIds.push_back(c->getId());
        }

        // Return all current hand cards to bottom of deck
        context.user.returnCards(context.deck, cardsToReturn, true);

        // Search the deck for cards matching snapshot ranks, regardless of order
        auto& deckCards = context.deck.getCards();
        for (auto it = deckCards.begin(); it != deckCards.end(); ) {
            Rank r = (*it)->getRank();
            if (needed.count(r) && needed[r] > 0) {
                context.user.addCardToHand(*it);
                context.drawnCardIds.push_back((*it)->getId());
                it = deckCards.erase(it);
                needed[r]--;
            } else {
                ++it;
            }
        }

        uses--;

        std::cout << "[Chronosphere] P" << context.user.getId() << " rewound hand. Uses left: " << uses << std::endl;

        return true;
    }

    // execute is called by processSkill — routes based on pendingChronoChoice
    bool execute(SkillContext& context) override {
        if (context.state.pendingChronoChoice == ChronoChoice::SNAPSHOT) {
            return executeSnapshot(context);
        } else if (context.state.pendingChronoChoice == ChronoChoice::REWIND) {
            return executeRewind(context);
        }
        std::cout << "[Chronosphere] ERROR: no action selected" << std::endl;
        return false;
    }
};

#endif
