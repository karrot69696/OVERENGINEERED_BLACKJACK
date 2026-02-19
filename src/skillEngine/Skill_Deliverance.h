#ifndef SKILL_DELIVERANCE_H
#define SKILL_DELIVERANCE_H

#include "Skill.h"
#include <string>
#include <iostream>
class SkillDeliverance : public Skill{

    public:
        SkillDeliverance(int user) {
            userId = user;
            uses = 3;
        }

        std::string skillNameToString() const override {
            return "Deliverance";
        }
        
        skillType getType() const override {
            return skillType::ACTIVE;
        }

        int getSkillId() const override {
            return 1;
        }

        
        bool canUse(const SkillContext& context) override {
            // At the end of any player's hit phase, 
            if (context.state.getPhase() != Phase::PLAYER_HIT_PHASE 
                    && context.state.getPhase() != Phase::HOST_HIT_PHASE ) {
                std::cout << "[SkillDeliverance - canUse] Can only be used during hit phase" << std::endl;
                return 0;
            }
            if (context.user.getHandSize() == 0) {
                std::cout << "[SkillDeliverance - canUse] No card in hand" << std::endl;
                return 0;
            }
            if (uses == 0) {
                std::cout << "[SkillDeliverance - canUse] Out of uses" << std::endl;
                return 0;
            }
            if (context.targetCards.empty()) {
                std::cout << "[SkillDeliverance - canUse] Invalid target card" << std::endl;
                return 0;
            }
            return 1;
        }
        bool execute(SkillContext& context) override{
            // At the end of any player's hit phase, 
            // you can return 1 card from you hand
            // to the deck, then shuffle it.
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