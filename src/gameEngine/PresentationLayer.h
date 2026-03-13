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

public:
    PresentationLayer(EventQueue& eq, AnimationManager& am,
                      UIManager& um, VisualState& vs, GameState& gs);

    // Called once per frame — drains the event queue and triggers animations/UI
    void processEvents();
};

#endif
