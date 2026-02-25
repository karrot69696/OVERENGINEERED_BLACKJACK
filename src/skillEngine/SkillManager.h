#ifndef SKILL_MANAGER_H
#define SKILL_MANAGER_H

#include "Skill.h"
#include "Skill_Deliverance.h"
#include "Skill_NeuralGambit.h"
#include <string>
#include <iostream>
//every time a new skill is added
// 0. Create the class for that skill
// 1. add them to property of SkillManager : SkillDeliverance* skillDeliverance;
// 2. modify createSkills, processSkill, getSkillUses, resetSkillUses
class SkillManager{
    private:
        SkillDeliverance* skillDeliverance;
        //other skills go here
    public:
        void createSkills(std::vector<Player>& players);
        bool processSkill(SkillContext& context){
            if (context.user.getSkillName()==SkillName::DELIVERANCE ){
                std::cout<< "[processSkill] Using "<< skillDeliverance->skillNameToString() 
                << std::endl;
                if ( skillDeliverance->canUse(context) ){
                    return skillDeliverance->execute(context);
                }
                return false;
            }

            std::cout << "[processSkill] Skill does not exist" << std::endl;
            return false;
        }
        int getSkillUses(SkillName name){
            switch (name){
                case SkillName::DELIVERANCE:
                    return skillDeliverance->getUses();
                break;
                default:
                    //std::cout << "[getSkillUses] Skill does not exist" << std::endl;
                    return 100;
            }
        }
        void resetSkillUses(std::vector<Player>& players){
            std::cout<< "Reset all skills' uses" << std::endl;
            for (auto& player : players){

                switch (player.getSkillName()){

                    case SkillName::DELIVERANCE:
                        if (player.getHost() && skillDeliverance){
                            skillDeliverance->resetUses();
                            skillDeliverance->gainUses(3);
                        }
                        else skillDeliverance->resetUses();
                    break;
                }
            }
        }


};

#endif