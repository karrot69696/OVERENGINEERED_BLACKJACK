#ifndef BLACKJACKCHECKPHASE_H
#define BLACKJACKCHECKPHASE_H
#include "Phase.h"

class UIManager;
class RoundManager;
class SkillManager;

class BlackJackCheckPhase : public Phase {
    public:
        using Phase::Phase; // Inherit constructor
        void onEnter() override;
        std::optional<PhaseName> onUpdate() override;
        void onExit() override;
        PhaseName getClassPhaseName() const override { return PhaseName::BLACKJACK_CHECK_PHASE; }
};

#endif