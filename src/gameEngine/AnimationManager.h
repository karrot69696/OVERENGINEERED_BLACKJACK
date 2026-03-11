#ifndef ANIMATIONMANAGER_H
#define ANIMATIONMANAGER_H
#include <vector>
#include <functional>
#include <optional>
#include <algorithm>
#include <memory>
#include <iostream>

#include "../lowLevelEntities/VisualState.h"

#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <SFML/Network.hpp>
#include <SFML/Audio.hpp>
namespace AnimConfig {
    inline constexpr float PHASE_TEXT_DURATION = 1.0f;
    inline constexpr float CARD_DRAW_DURATION = 0.8f;
}
struct Animation
{
    std::function<void(float)> update;// float is the [progress] of the animation, from 0% to 100%
    float time = 0;
    float duration;
};

class AnimationManager
{
private:
    std::vector<Animation> animations;
    sf::RenderWindow& window;
    GameState& gameState;
    VisualState& visualState;
    std::unique_ptr<sf::Text> phaseText;
    std::vector<std::shared_ptr<sf::Text>> floatingTexts;
public:
    AnimationManager(sf::RenderWindow& window, GameState& gameState, VisualState& visualState) : 
    window(window), gameState(gameState), visualState(visualState) {}
    void add(Animation anim){
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
        //it = std::vector<Animation>::iterator - an interator that points to each element in the vector, works like a pointer
        for (auto it = animations.begin(); it != animations.end();)
        {
            it->time += dt;// [time] accumulates += [delta time]

            float t = it->time / it->duration; // [time] is divided by [duration] to get [0-100% progress]
            if (t > 1) t = 1; //capping time back to 1 in case [time] is longer than [duration]

            it->update(t); //updates the animation based on the current progress (0-100%)

            if (it->time >= it->duration) //erase animation when it reaches 100%
                it = animations.erase(it);
            else
                ++it; //iterates to next animation
        }
    }

    bool playing() const
    {
        return !animations.empty();
    }
    void addDrawAnimation(int playerId, int handIndex, int cardId, sf::Vector2f startPosition){

        CardVisual& card = visualState.getCardVisual(cardId);

        card.location = CardLocation::HAND;
        card.ownerId = playerId;
        card.cardIndex = handIndex;

        sf::Vector2f seatPos = visualState.getPlayerSeatPos(playerId, gameState.getAllPlayerInfo().size());
        sf::Vector2f endPosition = {
            seatPos.x + handIndex * UILayout::CARD_SPACING,
            seatPos.y
        };

        // Set center origin so rotation spins around the card center (frisbee)
        auto bounds = card.cardSprite.getLocalBounds();
        sf::Vector2f localCenter = {bounds.size.x / 2.f, bounds.size.y / 2.f};
        sf::Vector2f scale = card.cardSprite.getScale();
        sf::Vector2f worldOffset = {localCenter.x * scale.x, localCenter.y * scale.y};

        card.cardSprite.setOrigin(localCenter);

        // Convert top-left positions to center-based
        sf::Vector2f centerStart = startPosition + worldOffset;
        sf::Vector2f centerEnd = endPosition + worldOffset;

        auto func = [&card, centerStart, centerEnd](float t){
            // Mostly straight, slight slowdown at end
            float eased = t * (2.f - t);
            sf::Vector2f pos = centerStart + (centerEnd - centerStart) * eased;
            card.cardSprite.setPosition(pos);

            // Frisbee spin: one full rotation
            card.cardSprite.setRotation(sf::degrees(360.f * t));
        };

        Animation drawAnim = {func, 0, AnimConfig::CARD_DRAW_DURATION};
        add(drawAnim);

    }
    void addReturnToDeckAnimation(int cardId){

        CardVisual& card = visualState.getCardVisual(cardId);

        // Current hand position (already center-based from draw animation)
        sf::Vector2f startPosition = card.cardSprite.getPosition();

        // Deck position (top-left based: 50, windowH/2), convert to center
        auto bounds = card.cardSprite.getLocalBounds();
        sf::Vector2f localCenter = {bounds.size.x / 2.f, bounds.size.y / 2.f};
        sf::Vector2f scale = card.cardSprite.getScale();
        sf::Vector2f worldOffset = {localCenter.x * scale.x, localCenter.y * scale.y};

        float deckX = 50.f;
        float deckY = static_cast<float>(window.getSize().y) / 2.f;
        sf::Vector2f centerEnd = {deckX + worldOffset.x, deckY + worldOffset.y};

        // Reset visual state (card is logically already in deck)
        card.ownerId = -1;
        card.cardIndex = -1;
        card.location = CardLocation::DECK;

        auto func = [&card, startPosition, centerEnd](float t){
            float eased = t * (2.f - t);
            sf::Vector2f pos = startPosition + (centerEnd - startPosition) * eased;
            card.cardSprite.setPosition(pos);

            // Reverse frisbee spin
            card.cardSprite.setRotation(sf::degrees(-360.f * t));
        };

        Animation returnAnim = {func, 0, AnimConfig::CARD_DRAW_DURATION};
        add(returnAnim);

    }
    void spawnPhaseText(std::string text, float duration){

        phaseText = std::make_unique<sf::Text>(visualState.getFont(), text, 64.f);

        sf::Text* textPtr = phaseText.get();
        auto bounds = textPtr->getLocalBounds();
        textPtr->setOrigin({bounds.position.x + bounds.size.x / 2.f,
                        bounds.position.y + bounds.size.y / 2.f});
        auto func = [this, textPtr, duration](float t) {

            auto winSize = window.getSize();
            float windowW = static_cast<float>(winSize.x);
            float windowH = static_cast<float>(winSize.y);

            auto bounds = textPtr->getLocalBounds();
            float textW = bounds.size.x;

            float startX  = -textW;
            float centerX = windowW / 2.f;
            float endX    = windowW + textW * 2;
            float y       = windowH / 2.f;

            float x;
            sf::Vector2f scale = {1.f, 1.f};

            if (t < 0.4f) {
                float localT = t / 0.4f;
                float eased = 1 - (1 - localT) * (1 - localT);
                x = startX + (centerX - startX) * eased;
            }
            else if (t < 0.6f) {
                x = centerX;

                float localT = (t - 0.4f) / 0.2f;
                scale.x = 1.f + 0.2f * std::sin(localT * 3.14159f);
                scale.y = scale.x;
            }
            else {
                float localT = (t - 0.6f) / 0.4f;
                float eased = localT * localT;
                x = centerX + (endX - centerX) * eased;
            }

            textPtr->setPosition({x, y});
            textPtr->setScale(scale);
        };

        Animation phaseTransitionAnimation = { func, 0, duration};
        this->add(phaseTransitionAnimation);
    }

    void spawnFloatingText(const std::string& text, sf::Vector2f position, sf::Color color, float duration = 1.0f){
        auto ft = std::make_shared<sf::Text>(visualState.getFont(), text, 20.f);
        ft->setFillColor(color);
        ft->setPosition(position);
        floatingTexts.push_back(ft);

        auto ptr = ft; // shared_ptr keeps it alive
        sf::Vector2f startPos = position;

        auto func = [ptr, startPos, color](float t){
            float eased = 1.f - (1.f - t) * (1.f - t); // easeOutQuad
            ptr->setPosition({startPos.x, startPos.y - 40.f * eased});
            sf::Color c = color;
            c.a = static_cast<uint8_t>(255.f * (1.f - t));
            ptr->setFillColor(c);
        };

        Animation floatAnim = {func, 0, duration};
        add(floatAnim);
    }

    void renderPhaseText(){
        if(phaseText)
            window.draw(*phaseText);
    }

    void renderFloatingTexts(){
        // Remove fully faded texts
        floatingTexts.erase(
            std::remove_if(floatingTexts.begin(), floatingTexts.end(),
                [](const auto& ft){ return ft->getFillColor().a == 0; }),
            floatingTexts.end()
        );
        for (auto& ft : floatingTexts){
            window.draw(*ft);
        }
    }
};

#endif