#ifndef PHASE_H
#define PHASE_H

#include <vector>
#include <functional>
#include <optional>
#include <algorithm>
#include <memory>
#include <iostream>

#include "../skillEngine/SkillManager.h"

#include "../lowLevelEntities/Player.h"

class UIManager;
class RoundManager;

class Phase {
    protected:
        UIManager& uiManager;
        RoundManager& roundManager;
    public:
        Phase(UIManager& uiManager, RoundManager& roundManager) : uiManager(uiManager), roundManager(roundManager){}
        virtual ~Phase() = default;
        virtual void onEnter(){};
        virtual std::optional<PhaseName> onUpdate(){return std::nullopt;};
        virtual void onExit(){};

        //helper functions to inherit
        Player& getCurrentPlayer();
        int getCurrentPlayerId();
        void incrementCurrentPlayerId();

};

#endif