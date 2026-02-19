#ifndef SKILL_H
#define SKILL_H
#include "../lowLevelEntities/GameState.h"
#include "../lowLevelEntities/Player.h"
#include <string>
#include <iostream>
enum class skillType {
    ACTIVE,
    REACTIVE
};
struct SkillContext {
    Player& user;
    std::vector<Player*> targetPlayers;
    std::vector<Card*> targetCards;
    Deck& deck;
    GameState& state;        // read/write if needed
};

class Skill{
    protected:
        int userId;
        int uses;
    public:
        virtual bool canUse(const SkillContext& context) = 0;
        virtual bool execute(SkillContext& context) = 0;
        virtual std::string skillNameToString() const = 0;
        virtual skillType getType() const = 0;
        virtual int getSkillId() const = 0;
        int getUserId(){
            return userId;
        }
        int getUses(){
            return uses;
        }

};

#endif