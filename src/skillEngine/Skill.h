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
    REACTIVE,
    FUSION
};

enum class ReactiveTrigger { NONE, ON_CARD_DRAWN, ON_HIT_PHASE_START };

struct ReactiveContext {
    ReactiveTrigger trigger;
    int drawerId;       // who drew the card
    int drawnCardId;    // which card was drawn
};
struct SkillContext {
    Player& user;
    std::vector<Player*> targetPlayers;
    std::vector<Card*> targetCards;
    Deck& deck;
    GameState& state;        // read/write if needed
    EventQueue& eventQueue;

    // Output: card IDs produced by execute() for skillProcessAftermath to emit events
    std::vector<int> returnedCardIds;
    std::vector<int> drawnCardIds;
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

        // Reactive skill interface — override in reactive subclasses
        virtual ReactiveTrigger getReactiveTrigger() const { return ReactiveTrigger::NONE; }
        virtual bool canReact(const ReactiveContext& ctx, const GameState& state) { return false; }

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