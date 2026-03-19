#ifndef PLAYERHITPHASE_H
#define PLAYERHITPHASE_H
#include "Phase.h"
class UIManager;
class RoundManager;
class SkillManager;

class PlayerHitPhase :public Phase {
    public:
        using Phase::Phase; // Inherit constructor
        void onEnter() override;
        std::optional<PhaseName> onUpdate() override;
        void onExit() override;
        PhaseName getClassPhaseName() const override { return PhaseName::PLAYER_HIT_PHASE; }
};

#endif