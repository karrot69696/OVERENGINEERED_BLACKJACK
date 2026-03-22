#ifndef PHASE_H
#define PHASE_H

#include <vector>
#include <functional>
#include <optional>
#include <algorithm>
#include <memory>
#include <iostream>

#include "../lowLevelEntities/VisualState.h"
#include "../gameEngine/EventQueue.h"
#include "../skillEngine/SkillManager.h"
#include "../skillEngine/Skill.h"
class UIManager;
class RoundManager;
class SkillManager;
class Phase {
    protected:
        UIManager& uiManager;      // kept for input callbacks (onActionChosen, onTargetChosen)
        EventQueue& eventQueue;     // replaces AnimationManager& — phases emit events, not animation calls
        RoundManager& roundManager;
        SkillManager& skillManager;
        GameState& gameState;
        VisualState& visualState;
        std::vector<Player>& players;
        Deck& deck;

        // Multi-actor skill state (NeuralGambit and future cross-player input skills)
        struct NgPendingState {
            int skillUserId;
            int firstTargetId;
            int secondTargetId;
            int firstCardId  = -1;
            int secondCardId = -1;
            bool firstRequestSent    = false;
            bool secondRequestSent   = false;
            bool revealSent          = false;
            bool boostRequested      = false;
            bool waitingForLocalPick = false;
            bool firstWasFaceDown    = false;
            bool secondWasFaceDown   = false;
        };
        std::optional<NgPendingState> ngPending;
        void ngTickPending(Player& skillUser);

        // Reactive skill check state (fires after events like card draws)
        struct ReactiveCheckState {
            ReactiveTrigger trigger;
            int drawnCardId;
            int drawerId;
            int actingPlayerId;       // who to re-prompt when all reactive checks done
            std::string extraInfo;
            
            struct QueueEntry { int skillOwnerId; SkillName skillName; };
            std::vector<QueueEntry> queue;
            int currentIndex = 0;

            enum Step { PROMPTING, WAITING_RESPONSE, PICKING_CARD, EXECUTING, DONE };
            Step step = PROMPTING;

            bool requestSent = false;
            bool waitingForLocalPick = false;
            float promptTimer = 0.f;
            static constexpr float PROMPT_TIMEOUT = 5.0f;
        };
        std::optional<ReactiveCheckState> reactiveCheck;
        void reactiveTickPending();
        bool startReactiveCheck(ReactiveTrigger trigger, int drawnCardId, int drawerId, int actingPlayerId);

    public:
        Phase(UIManager& uiManager,
            EventQueue& eventQueue,
            RoundManager& roundManager,
            SkillManager& skillManager,
            GameState& gameState, VisualState& visualState);

        virtual ~Phase() = default;
        virtual void onEnter(){};
        virtual std::optional<PhaseName> onUpdate(){return std::nullopt;};
        virtual void onExit(){};
        virtual PhaseName getClassPhaseName() const = 0;
        //helper functions to inherit
        Player& getCurrentPlayer();
        int getCurrentPlayerId();
        void incrementCurrentPlayerId();
        bool allPlayersProcessed();
        //ULTRA IMPORTANT TURN HANDLER
        bool turnHandler(Player& player, Player& opponent);
        void skillHandler(Player& player);
        void skillProcessAftermath(SkillContext& context, SkillExecutionResult skillResult);
        void processPassiveSkills(int priorityPlayerId);
        void blackJackAndQuintetCheck(std::vector<Player>& players);
};

#endif
