#ifndef ROUNDENDPHASE_H
#define ROUNDENDPHASE_H
#include "Phase.h"

class UIManager;
class RoundManager;

class RoundEndPhase : public Phase {
    public:
        using Phase::Phase; // Inherit constructor
        void onEnter() override;
        std::optional<PhaseName> onUpdate() override;
        void onExit() override;
};

#endif