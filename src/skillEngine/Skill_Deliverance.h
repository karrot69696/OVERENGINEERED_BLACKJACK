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
        SkillName getSkillName() const override {
            return SkillName::DELIVERANCE;
        }
        
        std::string canUse(const SkillContext& context) override {
            // During your hit phase
            if (context.user.getId() != this->userId) {
                std::cout << "[SkillDeliverance - canUse] Can only be used in your turn" << std::endl;
                return "CAN ONLY BE USED IN YOUR TURN";
            }
            else if (context.state.getPhaseName() != PhaseName::PLAYER_HIT_PHASE 
                    && context.state.getPhaseName() != PhaseName::HOST_HIT_PHASE ) {
                std::cout << "[SkillDeliverance - canUse] Can only be used during hit phase" << std::endl;
                return "CAN ONLY BE USED DURING HIT PHASE";
            }
            if (context.user.getHandSize() == 0) {
                std::cout << "[SkillDeliverance - canUse] No card in hand" << std::endl;
                return "NO CARD IN HAND";
            }
            if (uses == 0) {
                std::cout << "[SkillDeliverance - canUse] Out of uses" << std::endl;
                return "OUT OF USES";
            }
            if (context.targetCards.empty()) {
                std::cout << "[SkillDeliverance - canUse] Invalid target card" << std::endl;
                return "INVALID TARGET CARD";
            }
            return "";
        }
        bool execute(SkillContext& context) override{
            // During your hit phase, 
            // you can return 1 card from you hand
            // to the bottom of the deck.
            context.user.returnCards(context.deck,context.targetCards, true); //true for bot deck return
            //context.deck.shuffle();
            uses--;
            return true;
        }
        bool activatePassive(GameState& gameState) override { 
            //At the beginning of your host phase, gain 1 uses
            if  (gameState.getPhaseName() == PhaseName::HOST_HIT_PHASE){
                if (!gameState.getPlayerInfo(userId).isHost) return false;
                    uses++;
                    return true;
            }
            return false;
        }

};

#endif