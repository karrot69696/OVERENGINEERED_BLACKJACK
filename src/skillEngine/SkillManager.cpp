#include "SkillManager.h"

void SkillManager::createSkills(std::vector<Player>& players){
    for (auto & player : players){
        switch (player.getSkillName()){
            case SkillName::DELIVERANCE:
                skillDeliverance = new SkillDeliverance(player.getId());
                std::cout<<"[createSkills] Created [Deliverance] for player "<< player.getId()<<std::endl;
            break;
            default:
                std::cout <<"[createSkills] Skill: "<<player.skillNameToString() <<" does not exist"<<std::endl;
        }
        
        //run other ifs here to create other skills
    }
}