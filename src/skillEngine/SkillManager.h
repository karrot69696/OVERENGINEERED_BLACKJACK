#ifndef SKILL_MANAGER_H
#define SKILL_MANAGER_H

#include "Skill.h"
#include "Skill_Deliverance.h"
#include "Skill_NeuralGambit.h"
#include <vector>
#include <memory>
#include <iostream>
#include "../lowLevelEntities/GameState.h"
struct SkillExecutionResult {
    SkillName name=SkillName::UNDEFINED;
    std::string errorMsg="";
};
// To add a new skill:
// 1. Create the skill class (Skill_Name.h)
// 2. Add a case in createSkills()
// Everything else (processSkill, getSkillUses, resetSkillUses, skillPassiveHandler) is generic.
class SkillManager {
    private:
        std::vector<std::unique_ptr<Skill>> skills;
    public:
        SkillManager() {};
        void createSkills(std::vector<Player>& players);
        SkillExecutionResult processSkill(SkillContext& context);
        std::string preValidateSkill(int playerId, const GameState& state);
        int getSkillUses(SkillName name);
        void resetSkillUses(std::vector<Player>& players);
        bool skillPassiveHandler(GameState& gameState);
};

#endif
