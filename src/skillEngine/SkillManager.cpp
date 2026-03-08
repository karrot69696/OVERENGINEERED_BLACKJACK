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

bool SkillManager::processSkill(SkillContext& context){
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

 int SkillManager::getSkillUses(SkillName name){
    switch (name){
        case SkillName::DELIVERANCE:
            return skillDeliverance->getUses();
        break;
        default:
            //std::cout << "[getSkillUses] Skill does not exist" << std::endl;
            return 100;
    }
}

void SkillManager::resetSkillUses(std::vector<Player>& players){
    std::cout<< "[SkillManager] Reset all skills' uses" << std::endl;
    for (auto& player : players){
        std::cout<<"player:"<<player.getId()<<std::endl;
        switch (player.getSkillName()){

            case SkillName::DELIVERANCE:
                if (player.getHost() && skillDeliverance){
                    skillDeliverance->resetUses();
                }
                else skillDeliverance->resetUses();
            break;
        }
    }
}

bool SkillManager::skillPassiveHandler(GameState& gameState){
    std::cout << "[SkillManager] Activating skill passive..."<<std::endl;
    if (skillDeliverance){
        if (skillDeliverance->activatePassive(gameState)){
            std::cout
            <<"[SkillManager] Deliverance passive activated, Player "
            << gameState.getCurrentPlayerId()<< " gains 3 uses" << std::endl;
            return true;
        }
    }
    return false;
}
// write a function that calculates factorial
