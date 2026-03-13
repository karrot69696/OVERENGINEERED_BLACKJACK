#ifndef PRESENTATIONLAYER_H
#define PRESENTATIONLAYER_H

#include "EventQueue.h"

class AnimationManager;
class UIManager;
class VisualState;
class GameState;

class PresentationLayer {
private:
    EventQueue& eventQueue;
    AnimationManager& animationManager;
    UIManager& uiManager;
    VisualState& visualState;
    GameState& gameState;
    bool cutsceneBlocking = false;

    bool isCutscene(GameEventType type);

public:
    PresentationLayer(EventQueue& eq, AnimationManager& am,
                      UIManager& um, VisualState& vs, GameState& gs);

    // Called once per frame — processes events, pausing after cutscene events
    void processEvents();
};

#endif
