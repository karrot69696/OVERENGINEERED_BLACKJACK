#ifndef ANIMATIONMANAGER_H
#define ANIMATIONMANAGER_H
#include <array>
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
    inline constexpr float PHASE_TEXT_DURATION = 0.9f;
    inline constexpr float CARD_DRAW_DURATION = 0.4f;
    inline constexpr float CARD_RETURN_DURATION = 0.2f;
    inline constexpr float POINT_CHANGE_DURATION = 1.7f;
}
struct Animation
{
    std::function<void(float)> update;// float is the [progress] of the animation, from 0% to 100%
    std::function<void()> onFinish;   // called once when animation completes, before removal
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
    sf::Texture shockTexture;
    std::unique_ptr<sf::Sprite> shockSprite;
    sf::Texture explosionTexture;
    std::unique_ptr<sf::Sprite> explosionSprite;
    sf::Texture holyTexture; 
    std::unique_ptr<sf::Sprite> holySprite;

public:
    AnimationManager(sf::RenderWindow& window, GameState& gameState, VisualState& visualState) : 
    window(window), gameState(gameState), visualState(visualState) {
        std::filesystem::path shockTextPath = "../assets/images/cartoon-shock-animation-frames.png";
        shockTexture.loadFromFile(shockTextPath);
        std::filesystem::path explosionPath = "../assets/images/Explosion02_spritesheet.png";
        explosionTexture.loadFromFile(explosionPath);
        holyTexture.loadFromFile("../assets/images/holySpellEffect.png"); 
    }
    void add(Animation anim){
        animations.push_back(anim);
    }
    void render(){
        renderFloatingTexts();
        renderPhaseText();
        renderShockAnimation();
        renderExplosionAnimation();
        renderHolyAnimation();
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
        // Collect onFinish callbacks to run AFTER the loop,
        // because onFinish may call add() which push_back's into this vector,
        // invalidating iterators mid-loop
        std::vector<std::function<void()>> pendingCallbacks;

        for (auto it = animations.begin(); it != animations.end();)
        {
            it->time += dt;

            float t = it->time / it->duration;
            if (t > 1) t = 1;

            it->update(t);

            if (it->time >= it->duration) {
                if (it->onFinish) pendingCallbacks.push_back(std::move(it->onFinish));
                it = animations.erase(it);
            }
            else
                ++it;
        }

        // Now safe to run callbacks (they may add new animations)
        for (auto& cb : pendingCallbacks) cb();
    }

    bool playing() const
    {
        return !animations.empty();
    }
    void addDrawAnimation(int playerId, int handIndex, int cardId){

        CardVisual& card = visualState.getCardVisual(cardId);
        sf::Vector2f startPosition = card.cardSprite.getPosition();
        // Visual state (location, ownerId, cardIndex) is set by PresentationLayer before this call

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

        Animation drawAnim = {func, nullptr, 0, AnimConfig::CARD_DRAW_DURATION};
        add(drawAnim);

    }
    void addReturnToDeckAnimation(int cardId, std::function<void()> onFinish = nullptr){

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

        // Visual state (ownerId, cardIndex, location) is set by PresentationLayer before this call

        auto func = [&card, startPosition, centerEnd, this](float t){
            float eased = easeInCubic(t);
            sf::Vector2f pos = startPosition + (centerEnd - startPosition) * eased;
            card.cardSprite.setPosition(pos);

            // Reverse frisbee spin
            card.cardSprite.setRotation(sf::degrees(-360.f * eased));
        };

        Animation returnAnim = {func, std::move(onFinish), 0, AnimConfig::CARD_RETURN_DURATION};
        add(returnAnim);

    }
    void addSpinAnimation(int cardId, std::function<void()> onFinish = nullptr){
        CardVisual& card = visualState.getCardVisual(cardId);
        sf::Vector2f startPosition = card.cardSprite.getPosition();

        auto bounds = card.cardSprite.getLocalBounds();
        sf::Vector2f localCenter = {bounds.size.x / 2.f, bounds.size.y / 2.f};
        sf::Vector2f scale = card.cardSprite.getScale();
        sf::Vector2f worldOffset = {localCenter.x * scale.x, localCenter.y * scale.y};

        card.cardSprite.setOrigin(localCenter);

        auto func = [this,&card, startPosition](float t){
            float ease = easeOutCubic(t);
            // Reverse frisbee spin
            card.cardSprite.setRotation(sf::degrees(-720.f * ease));
        };

        Animation spinAnim = {func, std::move(onFinish), 0, AnimConfig::CARD_DRAW_DURATION};
        add(spinAnim);
    }
    void repositionHand(int playerId){
        int totalPlayers = (int)gameState.getAllPlayerInfo().size();
        sf::Vector2f seatPos = visualState.getPlayerSeatPos(playerId, totalPlayers);

        // Collect remaining cards for this player (returned cards already have ownerId=-1)
        std::vector<CardVisual*> handCards;
        for (auto& cv : visualState.getCardVisuals()) {
            if (cv.ownerId == playerId) {
                handCards.push_back(&cv);
            }
        }
        // Sort by current index so order is preserved
        std::sort(handCards.begin(), handCards.end(),
            [](CardVisual* a, CardVisual* b){ return a->cardIndex < b->cardIndex; });

        // Reassign sequential indices and slide to correct positions
        for (int i = 0; i < (int)handCards.size(); i++) {
            CardVisual& cv = *handCards[i];
            cv.cardIndex = i;

            auto bounds = cv.cardSprite.getLocalBounds();
            sf::Vector2f localCenter = {bounds.size.x / 2.f, bounds.size.y / 2.f};
            sf::Vector2f scale = cv.cardSprite.getScale();
            sf::Vector2f worldOffset = {localCenter.x * scale.x, localCenter.y * scale.y};

            sf::Vector2f startPos = cv.cardSprite.getPosition();
            sf::Vector2f endPos = {
                seatPos.x + i * UILayout::CARD_SPACING + worldOffset.x,
                seatPos.y + worldOffset.y
            };

            // Skip if already at target position (within tolerance)
            float dx = endPos.x - startPos.x;
            float dy = endPos.y - startPos.y;
            if (dx * dx + dy * dy < 1.f) continue;

            auto func = [&cv, startPos, endPos](float t){
                float eased = t * (2.f - t);
                sf::Vector2f pos = startPos + (endPos - startPos) * eased;
                cv.cardSprite.setPosition(pos);
            };

            Animation slideAnim = {func, nullptr, 0, 0.3f};
            add(slideAnim);
        }
    }
    float easeOutCubic(float t)
    {
        return 1 - std::pow(1 - t, 3);
    }

    float easeInCubic(float t)
    {
        return t * t * t;
    }

    float easeInOutCubic(float t)
    {
        return t < 0.5f
            ? 4 * t * t * t
            : 1 - std::pow(-2 * t + 2, 3) / 2;
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

            float flyInEnd   = 0.2f;
            float holdEnd    = 0.8f;
            float flyOutEnd  = 1.0f;

            if (t < flyInEnd)
            {
                float localT = t / flyInEnd;
                float eased = easeOutCubic(localT);
                x = startX + (centerX - startX) * eased;
            }
            else if (t < holdEnd)
            {
                x = centerX;

                float holdDuration = holdEnd - flyInEnd;
                float localT = (t - flyInEnd) / holdDuration;

                scale.x = 1.f + 0.12f * std::sin(localT * 3.14159f);
                scale.y = scale.x;
            }
            else
            {
                float flyOutDuration = flyOutEnd - holdEnd;
                float localT = (t - holdEnd) / flyOutDuration;

                float eased = easeInCubic(localT);
                x = centerX + (endX - centerX) * eased;
            }

            textPtr->setPosition({x, y});
            textPtr->setScale(scale);
        };

        Animation phaseTransitionAnimation = {func, nullptr, 0, duration};
        this->add(phaseTransitionAnimation);
    }

    void spawnFloatingText(const std::string& text, sf::Vector2f position, sf::Color color, float duration = 1.0f){
        auto ft = std::make_shared<sf::Text>(visualState.getFont(), text, 20.f);
        ft->setFillColor(color);
        ft->setPosition(position);
        floatingTexts.push_back(ft);
        auto bounds = ft->getLocalBounds();
        ft->setOrigin({bounds.position.x + bounds.size.x / 2.f,
                        bounds.position.y + bounds.size.y / 2.f});
        auto ptr = ft; // shared_ptr keeps it alive
        sf::Vector2f startPos = position;

        auto func = [this,ptr, startPos, color](float t){

            float flyInEnd   = 0.33f;
            float holdEnd    = 0.66f;
            float flyOutEnd  = 1.0f;

            float startY  = startPos.y;
            float centerY = startPos.y - 10.f;
            float endY    = startPos.y - 20.f;

            float y = startY;
            float x = startPos.x;
            sf::Color c = color;
            float peakScale = 1.1f;
            sf::Vector2f scale = {1.f, 1.f};
            if (t < flyInEnd)
            {
                float localT = t / flyInEnd;
                float eased = easeInCubic(localT);

                y = startY + (centerY - startY) * eased;

                float s = 1.f + (peakScale - 1.f) * eased;
                scale = {s, s};
                c.a = static_cast<uint8_t>(255.f * localT);

            }
            else if (t < holdEnd)
            {
                y = centerY;
                scale = {peakScale, peakScale};
            }
            else
            {
                float flyOutDuration = flyOutEnd - holdEnd;
                float localT = (t - holdEnd) / flyOutDuration;
                float eased = easeOutCubic(localT);

                c.a = static_cast<uint8_t>(255.f * (1.f - localT));
                y = centerY + (endY - centerY) * eased;

                float s = peakScale * (1.f - eased);
                scale = {s, s};
            }


            //ptr->setPosition({startPos.x, startPos.y - 40.f * eased});
            ptr->setScale(scale);
            ptr->setPosition({x, y});
            ptr->setFillColor(c);

        };

        Animation floatAnim = {func, nullptr, 0, duration};
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
    // Generic spritesheet frame animation helper
    // stepX = distance between frame origins on the sheet (defaults to frameW if 0)
    void playSpriteAnimation(sf::Sprite* sprite, int frameCount,
                             int startX, int startY, int frameW, int frameH,
                             float duration, int stepX = 0,
                             std::function<void()> onFinish = nullptr)
    {
        if (stepX == 0) stepX = frameW;
        sprite->setTextureRect(sf::IntRect({startX, startY}, {frameW, frameH}));

        auto func = [sprite, frameCount, startX, startY, frameW, frameH, stepX, duration](float t){
            float elapsed = t * duration;
            float frameDuration = duration / frameCount;
            int idx = std::min(int(elapsed / frameDuration), frameCount - 1);
            int x = startX + idx * stepX;
            sprite->setTextureRect(sf::IntRect({x, startY}, {frameW, frameH}));
        };

        Animation anim = {func, std::move(onFinish), 0, duration};
        add(anim);
    }

    void playShockAnimation(int winnerId, int loserId, float duration = 1.0f){

        int totalPlayers = (int)gameState.getAllPlayerInfo().size();
        sf::Vector2f offSet = {-30.f, 0.f};
        sf::Vector2f winnerPos = visualState.getPlayerSeatPos(winnerId, totalPlayers) + offSet;
        sf::Vector2f loserPos  = visualState.getPlayerSeatPos(loserId, totalPlayers) + offSet;
        sf::Vector2f midpoint  = (winnerPos + loserPos) / 2.f;

        sf::Vector2f dir = winnerPos - loserPos;
        float distance = std::sqrt(dir.x * dir.x + dir.y * dir.y);
        float angleDeg = std::atan2(dir.y, dir.x) * 180.f / 3.14159f + 90.f + 180.f;

        shockSprite = std::make_unique<sf::Sprite>(shockTexture);

        const int frameW = 307, frameH = 895, frameCount = 7;
        const int frameY = 221;
        const std::array<int, 7> frameXs = {1741, 1483, 1183, 896, 586, 306, 7};

        shockSprite->setOrigin({frameW / 2.f, frameH / 2.f});
        shockSprite->setPosition(midpoint);
        shockSprite->setRotation(sf::degrees(angleDeg));
        shockSprite->setScale({0.3f, distance / (float)frameH});
        shockSprite->setTextureRect(sf::IntRect({frameXs[0], frameY}, {frameW, frameH}));

        sf::Sprite* spritePtr = shockSprite.get();

        // Shock spritesheet has irregular frame positions, so custom update
        auto func = [spritePtr, frameXs, frameW, frameH, frameY, frameCount](float t)
        {
            float loopT = std::fmod(t * 2.f, 1.f);
            int idx = std::min((int)std::floor(loopT * frameCount), frameCount - 1);

            spritePtr->setTextureRect(
                sf::IntRect({frameXs[idx], frameY}, {frameW, frameH})
            );
        };

        Animation shockAnim = {func, [this](){ shockSprite.reset(); }, 0, duration};
        add(shockAnim);
    }

    void renderShockAnimation(){
        if (shockSprite)
            window.draw(*shockSprite);
    }

    void playExplosionAnimation(sf::Vector2f position, float scale = 2.f, float duration = 0.6f){
        explosionSprite = std::make_unique<sf::Sprite>(explosionTexture);

        explosionSprite->setOrigin({51 / 2.f, 65 / 2.f});
        explosionSprite->setPosition(position);
        explosionSprite->setScale({scale, scale});
        //a lambda function that plays once when animation is finished
        //called by AnimationManager::update()-> if (time > duration) -> run onFinish() -> erase(animation)
        auto onDone = [this](){ explosionSprite.reset(); };
        playSpriteAnimation(explosionSprite.get(), 
        12, //frameCount
        6, //startX
        0, //startY
        51, //frameW
        65, //frameH
        duration, 
        65, //stepX
        onDone); 
    }

    void playDeliveranceEffect(sf::Vector2f position, float scale = 2.f, float duration = 0.6f){

        holySprite = std::make_unique<sf::Sprite>(holyTexture);
        holySprite->setOrigin({32,32}); 
        holySprite->setPosition(position); 
        holySprite->setScale({scale,scale}); 
        auto onDone = [this](){ holySprite.reset(); };
        playSpriteAnimation(holySprite.get(), 
        19, //frameCount
        0, //startX
        0, //startY
        64, //frameW
        64, //frameH
        duration, 
        64, //stepX
        onDone);
        
    }
    void renderExplosionAnimation(){
        if (explosionSprite)
            window.draw(*explosionSprite);
    }
    void renderHolyAnimation(){
        if (holySprite)
            window.draw(*holySprite);
    }
};

#endif