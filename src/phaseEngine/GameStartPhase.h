#ifndef GAMESTARTPHASE_H
#define GAMESTARTPHASE_H
#include "Phase.h"

class UIManager;
class RoundManager;
class SkillManager;

class GameStartPhase : public Phase {
    private:
        int dealRound = 0;    // which card round (0 to HAND_START_VALUE-1)
        int dealPlayer = 0;   // which player index within current round
    public:
        using Phase::Phase; // Inherit constructor
        void onEnter() override;
        std::optional<PhaseName> onUpdate() override;
        void onExit() override;
        PhaseName getClassPhaseName() const override { return PhaseName::GAME_START_PHASE; }
};

#endif