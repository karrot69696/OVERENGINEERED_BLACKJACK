#ifndef ANIMATIONMANAGER_H
#define ANIMATIONMANAGER_H
#include <vector>
#include <functional>
#include <optional>
#include <algorithm>
#include <memory>
#include <iostream>

#include "../lowLevelEntities/GameState.h"
#include "../lowLevelEntities/Player.h"

#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <SFML/Network.hpp>
#include <SFML/Audio.hpp>
struct Animation
{
    std::function<void(float)> update;
    float time = 0;
    float duration;
};

class AnimationManager
{
private:
    std::vector<Animation> animations;
    sf::RenderWindow& window;
    GameState& gameState;
public:
    AnimationManager(sf::RenderWindow& window, GameState& gameState) : window(window), gameState(gameState) {}
    void add(Animation anim)
    {
        animations.push_back(anim);
    }

/**
 * @brief Updates all animations in the manager.
 *
 * This function updates all animations in the manager by incrementing their
 * time by the given delta time. It then calls the update function of each
 * animation with a value between 0 and 1. If an animation has finished
 * (i.e., its time is greater than or equal to its duration), it is removed from
 * the manager.
 *
 * @param dt The delta time to increment the time of each animation.
 */
    void update(float dt)
    {
        
        for (auto it = animations.begin(); it != animations.end();)
        {
            it->time += dt;

            float t = it->time / it->duration;
            if (t > 1) t = 1;

            it->update(t);

            if (it->time >= it->duration)
                it = animations.erase(it);
            else
                ++it;
        }
    }

    bool playing() const
    {
        return !animations.empty();
    }
};

#endif