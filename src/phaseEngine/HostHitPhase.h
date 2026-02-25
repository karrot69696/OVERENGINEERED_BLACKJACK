#ifndef HOSTHITPHASE_H
#define HOSTHITPHASE_H
#include "Phase.h"

class UIManager;
class RoundManager;

class HostHitPhase : public Phase {
    public:
        using Phase::Phase; // Inherit constructor
        void onEnter() override;
        std::optional<PhaseName> onUpdate() override;
        void onExit() override;
};

#endif