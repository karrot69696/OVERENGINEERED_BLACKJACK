#ifndef SKILL_MANAGER_H
#define SKILL_MANAGER_H

#include "Skill.h"
#include "Skill_Deliverance.h"
#include "Skill_NeuralGambit.h"
#include <string>
#include <iostream>
#include "../lowLevelEntities/GameState.h"

//every time a new skill is added
// 0. Create the class for that skill
// 1. add them to property of SkillManager : SkillDeliverance* skillDeliverance;
// 2. modify createSkills, processSkill, getSkillUses, resetSkillUses
class SkillManager{
    private:
        SkillDeliverance* skillDeliverance;
        //other skills go here
    public:
        SkillManager(){};
        void createSkills(std::vector<Player>& players);
        bool processSkill(SkillContext& context);
        int getSkillUses(SkillName name);
        void resetSkillUses(std::vector<Player>& players);
        bool skillPassiveHandler(GameState& gameState);


};

#endif