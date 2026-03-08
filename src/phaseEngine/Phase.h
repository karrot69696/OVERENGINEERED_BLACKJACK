#ifndef PHASE_H
#define PHASE_H

#include <vector>
#include <functional>
#include <optional>
#include <algorithm>
#include <memory>
#include <iostream>

#include "../lowLevelEntities/Player.h"

class UIManager;
class RoundManager;
class SkillManager;

class Phase {
    protected:
        UIManager& uiManager;
        RoundManager& roundManager;
        SkillManager& skillManager;
        GameState& gameState;
        std::vector<Player>& players;
        Deck& deck;
    public:
        Phase(UIManager& uiManager,
          RoundManager& roundManager,
          SkillManager& skillManager,
          GameState& gameState);

        virtual ~Phase() = default;
        virtual void onEnter(){};
        virtual std::optional<PhaseName> onUpdate(){return std::nullopt;};
        virtual void onExit(){};

        //helper functions to inherit
        Player& getCurrentPlayer();
        int getCurrentPlayerId();
        void incrementCurrentPlayerId();
        //ULTRA IMPORTANT TURN HANDLER
        bool turnHandler(Player& player, Player& opponent);
        void skillHandler(Player& player);
};

#endif