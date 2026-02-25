#ifndef BATTLEPHASE_H
#define BATTLEPHASE_H
#include "Phase.h"

class UIManager;
class RoundManager;

class BattlePhase : public Phase {
    public:
        using Phase::Phase; // Inherit constructor
        void onEnter() override;
        std::optional<PhaseName> onUpdate() override;
        void onExit() override;
};

#endif