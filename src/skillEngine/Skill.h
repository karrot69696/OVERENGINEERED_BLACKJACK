#ifndef SKILL_H
#define SKILL_H
#include "../lowLevelEntities/GameState.h"
#include "../lowLevelEntities/Player.h"
#include "../lowLevelEntities/SkillDeck.h"
#include "../gameEngine/EventQueue.h"
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
    EventQueue& eventQueue;
};

class Skill{
    protected:
        int userId;
        int uses;
    public:
        virtual std::string canUse(const SkillContext& context) = 0;
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
        virtual SkillName getSkillName() const = 0;
        virtual void resetUses() { uses = 3; }
        virtual void gainUses(int usesGained) { uses += usesGained; }
        virtual bool activatePassive(GameState&) { return false; }

        // Pre-targeting validation: can this skill be activated at all right now?
        // Checks everything EXCEPT target-specific conditions.
        // Override in subclass for skill-specific pre-checks.
        virtual std::string canActivate(const GameState& state) {
            if (uses == 0) return "OUT OF USES";
            PhaseName phase = state.getPhaseName();
            if (phase != PhaseName::PLAYER_HIT_PHASE && phase != PhaseName::HOST_HIT_PHASE)
                return "CAN ONLY BE USED DURING HIT PHASE";
            PlayerInfo info = state.getPlayerInfo(userId);
            if (info.cardsInHand.empty()) return "NO CARD IN HAND";
            return "";
        }

};

#endif