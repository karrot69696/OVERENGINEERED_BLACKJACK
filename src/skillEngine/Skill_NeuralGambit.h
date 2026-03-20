#ifndef SKILL_NEURALGAMBIT_H
#define SKILL_NEURALGAMBIT_H

#include "Skill.h"
#include <string>
#include <iostream>
#include <cstdlib>
class SkillNeuralGambit : public Skill{

    public:
        SkillNeuralGambit(int user) {
            userId = user;
            uses = 3;
        }

        std::string skillNameToString() const override {
            return "Neural Gambit";
        }
        
        skillType getType() const override {
            return skillType::ACTIVE;
        }

        int getSkillId() const override {
            return 1;
        }
        SkillName getSkillName() const override {
            return SkillName::NEURALGAMBIT;
        }

        
        std::string canUse(const SkillContext& context) override {
            if (context.user.getId() != this->userId)
                return "CAN ONLY BE USED IN YOUR TURN";
            if (context.state.getPhaseName() != PhaseName::PLAYER_HIT_PHASE
                    && context.state.getPhaseName() != PhaseName::HOST_HIT_PHASE)
                return "CAN ONLY BE USED DURING HIT PHASE";
            if (context.user.getHandSize() == 0)
                return "NO CARD IN HAND";
            if (uses == 0)
                return "OUT OF USES";
            // targetCards: [0]=user's card, [1]=target's card, [2]=chosen card to boost
            if ((int)context.targetCards.size() < 3)
                return "SELECT TWO CARDS FIRST";
            if (!context.targetPlayers.empty() && context.targetPlayers[0]->getHandSize() == 0)
                return "TARGET HAS NO CARDS";
            return "";
        }

        bool execute(SkillContext& context) override {
            // targetCards[0] = user's card, [1] = target's card, [2] = chosen card to boost
            Card* userCard   = context.targetCards[0];
            Card* targetCard = context.targetCards[1];
            Card* boostCard  = context.targetCards[2];

            int userVal   = static_cast<int>(userCard->getRank())   + userCard->getRankBonus();
            int targetVal = static_cast<int>(targetCard->getRank()) + targetCard->getRankBonus();
            int boost = std::abs(userVal - targetVal);

            boostCard->setRankBonus(boostCard->getRankBonus() + boost);

            std::cout << "[NeuralGambit] Card " << boostCard->getRankAsString()
                      << " boosted by +" << boost << " (new bonus: "
                      << boostCard->getRankBonus() << ")" << std::endl;

            uses--;
            return true;
        }

};

#endif