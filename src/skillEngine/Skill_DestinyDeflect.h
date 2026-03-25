#ifndef SKILL_DESTINYDEFLECT_H
#define SKILL_DESTINYDEFLECT_H

#include "Skill.h"
#include <string>
#include <iostream>

class SkillDestinyDeflect : public Skill {
public:
    SkillDestinyDeflect(int user) {
        userId = user;
        uses = 3;
    }

    std::string skillNameToString() const override {
        return "Destiny Deflect";
    }

    skillType getType() const override {
        return skillType::REACTIVE;
    }

    SkillName getSkillName() const override {
        return SkillName::DESTINYDEFLECT;
    }

    ReactiveTrigger getReactiveTrigger() const override {
        return ReactiveTrigger::ON_CARD_DRAWN;
    }

    std::string canActivate(const GameState& state) override {
        return "REACTIVE ONLY - TRIGGERS ON CARD DRAW";
    }

    bool canReact(const ReactiveContext& ctx, const GameState& state) override {
        if (uses <= 0) return false;
        // Fires on ALL draws including self (unlike Fatal Deal which excludes self)
        return true;
    }

    std::string canUse(const SkillContext& context) override {
        if (uses <= 0) return "OUT OF USES";
        if ((int)context.targetPlayers.size() < 2) return "SELECT A TARGET PLAYER";
        if (context.targetCards.empty()) return "NO CARD TO REDIRECT";
        return "";
    }

    // targetPlayers[0] = the drawer (who originally got the card)
    // targetPlayers[1] = the redirect target (who receives the card)
    // targetCards[0]   = the drawn card
    bool execute(SkillContext& context) override {
        Card* drawnCard = context.targetCards[0];
        Player& drawer = *context.targetPlayers[0];
        Player& target = *context.targetPlayers[1];

        // Move card from drawer's hand to target's hand
        drawer.removeCardFromHand(drawnCard->getId());
        target.addCardToHand(drawnCard);

        // Card goes face-down after redirect — only visible to new owner
        if (drawnCard->isFaceUp()) drawnCard->flip();

        std::cout << "[DestinyDeflect] P" << context.user.getId()
                  << " redirected card " << drawnCard->getRankAsString() << drawnCard->getSuitAsString()
                  << " id:" << drawnCard->getId()
                  << " from P" << drawer.getId()
                  << " to P" << target.getId()
                  << std::endl;

        uses--;
        return true;
    }
};

#endif
