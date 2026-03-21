#ifndef SKILL_FATALDEAL_H
#define SKILL_FATALDEAL_H

#include "Skill.h"
#include <string>
#include <iostream>

class SkillFatalDeal : public Skill {
public:
    SkillFatalDeal(int user) {
        userId = user;
        uses = 3;
    }

    std::string skillNameToString() const override {
        return "Fatal Deal";
    }

    skillType getType() const override {
        return skillType::REACTIVE;
    }

    int getSkillId() const override {
        return 2;
    }

    SkillName getSkillName() const override {
        return SkillName::FATALDEAL;
    }

    ReactiveTrigger getReactiveTrigger() const override {
        return ReactiveTrigger::ON_CARD_DRAWN;
    }

    std::string canActivate(const GameState& state) override {
        return "REACTIVE ONLY - TRIGGERS ON CARD DRAW";
    }

    bool canReact(const ReactiveContext& ctx, const GameState& state) override {
        if (uses <= 0) return false;
        if (ctx.drawerId == userId) return false;  // don't react to own draws
        PlayerInfo ownerInfo = state.getPlayerInfo(userId);
        if (ownerInfo.cardsInHand.empty()) return false;  // need a card to swap
        return true;
    }

    std::string canUse(const SkillContext& context) override {
        if (uses <= 0) return "OUT OF USES";
        if ((int)context.targetCards.size() < 2) return "SELECT A CARD TO SWAP";
        return "";
    }

    // targetCards[0] = the drawn card (in drawer's hand)
    // targetCards[1] = Fatal Deal user's card to give away
    // targetPlayers[0] = the drawer
    bool execute(SkillContext& context) override {
        Card* drawnCard = context.targetCards[0];
        Card* chosenCard = context.targetCards[1];
        Player& drawer = *context.targetPlayers[0];

        // Direct swap between hands
        drawer.removeCardFromHand(drawnCard->getId());
        context.user.removeCardFromHand(chosenCard->getId());

        context.user.addCardToHand(drawnCard);
        drawer.addCardToHand(chosenCard);

        // Cards go face-down after swap — only visible to their new owner
        if (drawnCard->isFaceUp()) drawnCard->flip();
        if (chosenCard->isFaceUp()) chosenCard->flip();

        std::cout << "[FatalDeal] P" << context.user.getId()
                  << " swapped card " << chosenCard->getRankAsString() << chosenCard->getSuitAsString() << " id:" << chosenCard->getId()
                  << " with P" << drawer.getId()
                  << "'s drawn " << drawnCard->getRankAsString() << drawnCard->getSuitAsString()<< " id:" << drawnCard->getId()
                  << std::endl;

        uses--;
        return true;
    }
};

#endif
