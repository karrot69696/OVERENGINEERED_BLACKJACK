#ifndef HOSTHITPHASE_H
#define HOSTHITPHASE_H
#include "Phase.h"

class UIManager;
class RoundManager;
class SkillManager;

class HostHitPhase : public Phase {
    public:
        using Phase::Phase; // Inherit constructor
        void onEnter() override;
        std::optional<PhaseName> onUpdate() override;
        void onExit() override;
        PhaseName getClassPhaseName() const override { return PhaseName::HOST_HIT_PHASE; }
};

#endif