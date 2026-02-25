#ifndef SKILL_NEURALGAMBIT_H
#define SKILL_NEURALGAMBIT_H

#include "Skill.h"
#include <string>
#include <iostream>
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

        
        bool canUse(const SkillContext& context) override {
            // At the end of any player's hit phase, 
            if (context.state.getPhaseName() != PhaseName::PLAYER_HIT_PHASE 
                    && context.state.getPhaseName() != PhaseName::HOST_HIT_PHASE ) {
                std::cout << "[SkillNeuralGambit - canUse] Can only be used during hit phase" << std::endl;
                return 0;
            }
            if (context.user.getHandSize() == 0) {
                std::cout << "[SkillNeuralGambit - canUse] No card in hand" << std::endl;
                return 0;
            }
            if (uses == 0) {
                std::cout << "[SkillNeuralGambit - canUse] Out of uses" << std::endl;
                return 0;
            }
            if (context.targetCards.empty()) {
                std::cout << "[SkillNeuralGambit - canUse] Invalid target card" << std::endl;
                return 0;
            }
            return 1;
        }
        bool execute(SkillContext& context) override{
            // During your hit phase, you can choose another player, 
            // you and that player each choose one card from your hand
            // and reveal them, then you choose one of the cards revealed, 
            // that card rank is increased by X 
            // (X is the difference between the revealed cards)
            context.user.returnCards(context.deck,context.targetCards);
            context.deck.shuffle();
            uses--;
            return true;
        }
        void resetUses(){
            uses = 3;
        }
        void gainUses(int usesGained){
            uses+=usesGained;
        }
        

};

#endif