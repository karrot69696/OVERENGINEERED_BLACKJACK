#ifndef ROUNDENDPHASE_H
#define ROUNDENDPHASE_H
#include "Phase.h"

class UIManager;
class RoundManager;
class SkillManager;

class RoundEndPhase : public Phase {
    private:
        int returnCardIdx = 0;     // which card within current player
        int returnPlayerIdx = 0;   // which player we're returning cards from
        // Snapshot of card IDs per player, taken before returnCards clears hands
        std::vector<std::vector<int>> playerCardIds;
    public:
        using Phase::Phase; // Inherit constructor
        void onEnter() override;
        std::optional<PhaseName> onUpdate() override;
        void onExit() override;
        PhaseName getClassPhaseName() const override { return PhaseName::ROUND_END; }
};

#endif