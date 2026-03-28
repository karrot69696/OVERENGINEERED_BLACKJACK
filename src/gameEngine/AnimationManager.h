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
    inline constexpr float POINT_CHANGE_DURATION = 2.0f;
    inline constexpr float CARD_FLIP_DURATION = 1.0f;
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
    sf::Texture fatalDealTexture;
    std::unique_ptr<sf::Sprite> fatalDealSprite1;
    std::unique_ptr<sf::Sprite> fatalDealSprite2;
    sf::Texture neuralGambitTexture;
    std::unique_ptr<sf::Sprite> neuralGambitSprite1;
    std::unique_ptr<sf::Sprite> neuralGambitSprite2;
    sf::Texture chronosphereTexture;
    std::unique_ptr<sf::Sprite> chronosphereSprite;
    sf::Texture destinyDeflectTexture;
    std::unique_ptr<sf::Sprite> destinyDeflectSprite;
    PlayerVisual* borrowedPlayerVisual = nullptr;
public:
    AnimationManager(sf::RenderWindow& window, GameState& gameState, VisualState& visualState) : 
    window(window), gameState(gameState), visualState(visualState) {
        std::filesystem::path shockTextPath = "assets/images/cartoon-shock-animation-frames.png";
        shockTexture.loadFromFile(shockTextPath);
        std::filesystem::path explosionPath = "assets/images/Explosion02_spritesheet.png";
        explosionTexture.loadFromFile(explosionPath);
        holyTexture.loadFromFile("assets/images/holySpellEffect.png");
        fatalDealTexture.loadFromFile("assets/images/fatalDealEffect.png");
        neuralGambitTexture.loadFromFile("assets/images/neuralGambitEffect.png");
        chronosphereTexture.loadFromFile("assets/images/chronoSphereEffect.png");
        destinyDeflectTexture.loadFromFile("assets/images/destinyDeflectEffect.png");
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
        renderFatalDealEffect();
        renderNeuralGambitEffect();
        renderChronosphereEffect();
        renderDestinyDeflectEffect();
        renderBorrowedPlayerVisual();
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
        auto pos = card.cardSprite.getPosition();
        auto localBounds  = card.cardSprite.getLocalBounds();
        card.cardSprite.setOrigin({localBounds.size.x / 2.f, localBounds.size.y / 2.f});
        card.cardSprite.setPosition(pos);
        sf::Vector2f startPosition = pos;
        // Visual state (location, ownerId, cardIndex) is set by PresentationLayer before this call

        sf::Vector2f seatPos = visualState.getPlayerSeatPos(playerId, gameState.getAllPlayerInfo().size());
        sf::Vector2f endPosition = {
            seatPos.x + UILayout::HAND_OFFSET_X() + handIndex * UILayout::CARD_SPACING(),
            seatPos.y + UILayout::HAND_OFFSET_Y()
        };

        // Set center origin so rotation spins around the card center (frisbee)
        // setOrigin needs local coords; worldOffset needs world (scaled) coords
        
        //auto globalBounds = card.cardSprite.getGlobalBounds();
       
        //sf::Vector2f worldOffset = {globalBounds.size.x / 2.f, globalBounds.size.y / 2.f};

       

        // Convert top-left positions to center-based
        //  sf::Vector2f centerStart = startPosition + worldOffset;
        //  sf::Vector2f centerEnd = endPosition + worldOffset;
         sf::Vector2f centerStart = startPosition ;
         sf::Vector2f centerEnd = endPosition ;


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

        // Current position (center-origin based)
        sf::Vector2f startPosition = card.cardSprite.getPosition();

        // Deck position — same as where deck cards sit in rebuildFromState
        float deckX = static_cast<float>(window.getSize().x) * UILayout::DECK_X_RATIO();
        float deckY = static_cast<float>(window.getSize().y) / 2.f;
        sf::Vector2f centerEnd = {deckX, deckY};

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
    // Teleport swap animation: shake in place → shrink to nothing → pop in at target position
    void addTeleportSwapAnimation(int cardId, sf::Vector2f targetPos, int targetIndex, std::function<void()> onFinish = nullptr) {
        CardVisual& card = visualState.getCardVisual(cardId);
        sf::Vector2f origPos = card.cardSprite.getPosition();
        sf::Vector2f origScale = card.cardSprite.getScale();
        sf::Vector2f endPos = targetPos;

        // Ensure center origin
        auto localBounds = card.cardSprite.getLocalBounds();
        card.cardSprite.setOrigin({localBounds.size.x / 2.f, localBounds.size.y / 2.f});
        card.cardSprite.setPosition(origPos);

        // Phase 1: shake in place, then shrink to nothing
        auto phase1 = [&card, origPos, origScale](float t) {
            if (t < 0.7f) {
                float localT = t / 0.7f;
                float intensity = localT * 4.f;
                float shakeX = std::sin(localT * 40.f) * intensity + std::sin(localT * 80.f) * intensity * 0.5f;
                float shakeY = std::cos(localT * 40.f) * intensity * 0.7f;
                card.cardSprite.setPosition({origPos.x + shakeX, origPos.y + shakeY});
                card.cardSprite.setScale(origScale);
            } else {
                float localT = (t - 0.7f) / 0.3f;
                float eased = localT * localT;
                float s = 1.f - eased; // 1.0 → 0.0
                card.cardSprite.setPosition(origPos);
                card.cardSprite.setScale({origScale.x * s, origScale.y * s});
            }
        };

        // Phase 2 (chained): appear at new position with pop-in
        auto phase2 = [&card, endPos, origScale](float t) {
            card.cardSprite.setPosition(endPos);
            float eased;
            if (t < 0.5f) {
                float lt = t / 0.5f;
                eased = lt * lt * 1.15f;
            } else {
                float lt = (t - 0.5f) / 0.5f;
                eased = 1.15f + (1.f - 1.15f) * lt;
            }
            card.cardSprite.setScale({origScale.x * eased, origScale.y * eased});
        };

        auto phase2Finish = [&card, targetIndex, origScale, onFinish]() {
            card.cardIndex = targetIndex;
            card.cardSprite.setScale(origScale);
            card.cardSprite.setRotation(sf::degrees(0));
            if (onFinish) onFinish();
        };

        auto chainPhase2 = [this, phase2, phase2Finish]() {
            add({phase2, phase2Finish, 0, 0.25f});
        };

        add({phase1, chainPhase2, 0, 0.45f});
    }

    void addSpinAnimation(int cardId, std::function<void()> onFinish = nullptr){
        CardVisual& card = visualState.getCardVisual(cardId);
        sf::Vector2f startPosition = card.cardSprite.getPosition();

        auto bounds = card.cardSprite.getLocalBounds();
        card.cardSprite.setOrigin({bounds.size.x / 2.f, bounds.size.y / 2.f});
        card.cardSprite.setPosition(startPosition);

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

            auto startPos = cv.cardSprite.getPosition();
            auto localBounds = cv.cardSprite.getLocalBounds();
            cv.cardSprite.setOrigin({localBounds.size.x / 2.f, localBounds.size.y / 2.f});
            cv.cardSprite.setPosition(startPos);        
            sf::Vector2f endPos = {
                seatPos.x + UILayout::HAND_OFFSET_X() + i * UILayout::CARD_SPACING(),
                seatPos.y + UILayout::HAND_OFFSET_Y()
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

    void playCardFlip(int cardId, std::function<void()> onFinish = nullptr) {
        CardVisual& card = visualState.getCardVisual(cardId);
        if (card.faceUp) {
            if (onFinish) onFinish();
            return; // already face-up, nothing to animate
        }
        sf::Vector2f origScale = card.cardSprite.getScale();

        // Ensure center origin for symmetric scaling
        auto flipPos = card.cardSprite.getPosition();
        auto localBounds = card.cardSprite.getLocalBounds();
        card.cardSprite.setOrigin({localBounds.size.x / 2.f, localBounds.size.y / 2.f});
        card.cardSprite.setPosition(flipPos);

        auto flipped = std::make_shared<bool>(false);

        auto func = [this,
                    &card,
                    origScale,
                    cardId,
                    flipped,
                    startPos = card.cardSprite.getPosition()](float t)
        {
            float scaleX = origScale.x;
            float scaleY = origScale.y;

            // --- 1. RISE + FLIP (0 → 0.45) — scale up toward camera, flip midway ---
            if (t < 0.45f) {
                float localT = t / 0.45f;
                float grow = 1.f + 0.35f * easeOutCubic(localT);

                if (localT <= 0.45f) {
                    float s = 1.f - easeInCubic(localT / 0.45f);
                    scaleX *= grow * s;
                } else {
                    if (!*flipped) {
                        visualState.flipCardVisualFaceUp(cardId);
                        *flipped = true;
                    }
                    float s = easeOutCubic((localT - 0.45f) / 0.55f);
                    scaleX *= grow * s;
                }
                scaleY *= grow;
            }

            // --- 2. SLAM (0.45 → 0.6) — snap back past normal, impact squash ---
            else if (t < 0.6f) {
                float localT = (t - 0.45f) / 0.15f;
                float e = localT * localT;
                scaleX *= 1.35f + (0.88f - 1.35f) * e;
                scaleY *= 1.35f + (0.92f - 1.35f) * e;
            }

            // --- 3. SETTLE (0.6 → 1.0) — spring back to normal ---
            else {
                float localT = (t - 0.6f) / 0.4f;
                float spring = std::exp(-6.f * localT) * std::sin(localT * 14.f);
                scaleX *= 1.f + (0.88f - 1.f) * (1.f - easeOutCubic(localT)) + spring * 0.04f;
                scaleY *= 1.f + (0.92f - 1.f) * (1.f - easeOutCubic(localT)) + spring * 0.03f;
            }

            card.cardSprite.setPosition(startPos);
            card.cardSprite.setScale({scaleX, scaleY});
        };

        auto finishCb = [&card, origScale, startPos = card.cardSprite.getPosition(), onFinish]() {
            card.cardSprite.setPosition(startPos);
            card.cardSprite.setScale(origScale);
            if (onFinish) onFinish();
        };

        add({func, finishCb, 0, AnimConfig::CARD_FLIP_DURATION});
    }

    void spawnPhaseText(std::string text, float duration){

        float S = GameSettings::instance().S;
        phaseText = std::make_unique<sf::Text>(visualState.getFont(), text, (unsigned int)(32.f * S));

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

    void spawnFloatingText(const std::string& text, sf::Vector2f position, sf::Color color, float duration = 2.0f){
        float S = GameSettings::instance().S;

        int len = (int)text.size();
        float fontSize = 12.f * S;
        sf::Color colorOverride = color;

        bool isNewHost = text.rfind("New host - ", 0) == 0;
        if (isNewHost)
        {
            fontSize = 10.f * S;
            colorOverride = sf::Color(255, 215, 0);
        }

        auto ft = std::make_shared<sf::Text>(visualState.getFont(), text, fontSize);
        ft->setFillColor(colorOverride);
        ft->setOutlineThickness(0.1f);
        ft->setOutlineColor(sf::Color::Black);

        // Center origin
        auto bounds = ft->getLocalBounds();
        ft->setOrigin({
            bounds.position.x + bounds.size.x / 2.f,
            bounds.position.y + bounds.size.y / 2.f
        });

        // --- SCREEN INFO ---
        float winW = static_cast<float>(window.getSize().x);
        float winH = static_cast<float>(window.getSize().y);

        // --- ANIMATION CONSTANTS ---
        float floatDistance = 25.f * S;
        float peakScale = 2.0f;
        float maxShake = 3.f; // from your animation

        // --- SAFE BOUNDS (worst case) ---
        float padding = 2.f * S;

        float halfW = (bounds.size.x * peakScale) / 2.f + padding + maxShake;
        float halfH = (bounds.size.y * peakScale) / 2.f + padding;

        // --- SMART CLAMP ---
        sf::Vector2f startPos = position;

        // Clamp X (considering shake)
        startPos.x = std::clamp(startPos.x, halfW, winW - halfW);

        // Clamp Y (considering upward movement)
        float minY = floatDistance + halfH;   // so top never clips
        float maxY = winH - halfH;            // so bottom never clips
        startPos.y = std::clamp(startPos.y, minY, maxY);

        ft->setPosition(startPos);
        floatingTexts.push_back(ft);

        auto ptr = ft;

        auto func = [this, ptr, startPos, colorOverride, floatDistance, peakScale](float t)
        {
            float winW = static_cast<float>(window.getSize().x);
            float winH = static_cast<float>(window.getSize().y);

            // --- POSITION ---
            float yOffset = floatDistance * easeOutCubic(t);
            float y = startPos.y - yOffset;

            // --- SCALE ---
            float scaleT = std::min(t * 3.0f, 1.0f);
            float s;

            if (scaleT < 0.7f)
            {
                float local = scaleT / 0.7f;
                s = 1.f + (peakScale - 1.f) * easeInCubic(local);
            }
            else
            {
                float local = (scaleT - 0.7f) / 0.7f;
                s = peakScale - 0.15f * easeInCubic(local);
            }

            // --- ALPHA ---
            float fadeStart = 0.3f;
            float alpha = 1.f;

            if (t > fadeStart)
            {
                float fadeT = (t - fadeStart) / (1.f - fadeStart);
                alpha = 1.f - easeInCubic(fadeT);
            }

            sf::Color c = colorOverride;
            c.a = static_cast<uint8_t>(255.f * alpha);

            // --- SHAKE ---
            float shake = (1.f - t) * 3.f * std::sin(t * 40.f);
            float x = startPos.x + shake;

            // --- APPLY ---
            ptr->setScale({s, s});
            ptr->setPosition({x, y});
            ptr->setFillColor(c);

            // --- FINAL SAFETY CLAMP (optional but bulletproof) ---
            auto gb = ptr->getGlobalBounds();

            float halfW = gb.size.x / 2.f;
            float halfH = gb.size.y / 2.f;

            if (x - halfW < 0.f) x = halfW;
            if (x + halfW > winW) x = winW - halfW;
            if (y - halfH < 0.f) y = halfH;
            if (y + halfH > winH) y = winH - halfH;

            ptr->setPosition({x, y});
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
        float S = GameSettings::instance().S;
        int totalPlayers = (int)gameState.getAllPlayerInfo().size();
        sf::Vector2f offSet = {-30.f * S, 0.f};
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

    void playExplosionAnimation(sf::Vector2f position, float scale = 2.f, float duration = 0.8f){
        float S = GameSettings::instance().S;
        explosionSprite = std::make_unique<sf::Sprite>(explosionTexture);

        explosionSprite->setOrigin({51 / 2.f, 65 / 2.f});
        explosionSprite->setPosition(position);
        explosionSprite->setScale({scale * S, scale * S});
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

    void playDeliveranceEffect(sf::Vector2f position, float scale = 2.f, float duration = 0.8f){
        float S = GameSettings::instance().S;
        holySprite = std::make_unique<sf::Sprite>(holyTexture);
        holySprite->setOrigin({32,32});
        holySprite->setPosition(position);
        holySprite->setScale({scale * S, scale * S}); 
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

    void playFatalDealInitialEffect(sf::Vector2f position, float scale = 2.f, float duration = 0.6f){
        float S = GameSettings::instance().S;
       if(!fatalDealSprite1) fatalDealSprite1 = std::make_unique<sf::Sprite>(fatalDealTexture);
        fatalDealSprite1->setOrigin({32,32});
        fatalDealSprite1->setPosition(position);
        fatalDealSprite1->setScale({scale * S, scale * S}); 
        auto onDone = [this](){ fatalDealSprite1.reset(); };
        playSpriteAnimation(fatalDealSprite1.get(),19,0,512,64,64, duration, 64, onDone);
    }

    void renderExplosionAnimation(){
        if (explosionSprite)
            window.draw(*explosionSprite);
    }
    void renderHolyAnimation(){
        if (holySprite)
            window.draw(*holySprite);
    }
    void playFatalDealEffect(sf::Vector2f pos1, sf::Vector2f pos2, float scale = 2.f, float duration = 1.0f){
        float S = GameSettings::instance().S;
        if(!fatalDealSprite1) fatalDealSprite1 = std::make_unique<sf::Sprite>(fatalDealTexture);
        fatalDealSprite1->setOrigin({32,32});
        fatalDealSprite1->setPosition(pos1);
        fatalDealSprite1->setScale({scale * S, scale * S});
        auto onDone1 = [this](){ fatalDealSprite1.reset(); };
        playSpriteAnimation(fatalDealSprite1.get(),
            19, 0, 512, 64, 64, duration, 64, onDone1);

        fatalDealSprite2 = std::make_unique<sf::Sprite>(fatalDealTexture);
        fatalDealSprite2->setOrigin({32,32});
        fatalDealSprite2->setPosition(pos2);
        fatalDealSprite2->setScale({scale * S, scale * S});
        auto onDone2 = [this](){ fatalDealSprite2.reset(); };
        playSpriteAnimation(fatalDealSprite2.get(),
            19, 0, 512, 64, 64, duration, 64, onDone2);
    }
    void playNeuralGambitEffect(int cardId1, int cardId2, int boostCardId, int boostAmount,
        float scale = 2.f, float duration = 0.8f, std::function<void()> onFinish = nullptr)
    {
        float S = GameSettings::instance().S;
        CardVisual& cardCv1 = visualState.getCardVisual(cardId1);
        CardVisual& cardCv2 = visualState.getCardVisual(cardId2);
        CardVisual& boostCv = visualState.getCardVisual(boostCardId);

        sf::Vector2f pos1 = cardCv1.cardSprite.getPosition();
        sf::Vector2f pos2 = cardCv2.cardSprite.getPosition();
        sf::Vector2f posBoosted = boostCv.cardSprite.getPosition();

        auto bounds = cardCv1.cardSprite.getLocalBounds();
        cardCv1.cardSprite.setOrigin({bounds.size.x / 2.f, bounds.size.y / 2.f});
        cardCv1.cardSprite.setPosition(pos1);
        bounds = cardCv2.cardSprite.getLocalBounds();
        cardCv2.cardSprite.setOrigin({bounds.size.x / 2.f, bounds.size.y / 2.f});
        cardCv2.cardSprite.setPosition(pos2);
        bounds = boostCv.cardSprite.getLocalBounds();
        boostCv.cardSprite.setOrigin({bounds.size.x / 2.f, bounds.size.y / 2.f});
        boostCv.cardSprite.setPosition(posBoosted);
        sf::Vector2f origScale = boostCv.cardSprite.getScale();

        // Phase 2: shake boosted card, then pop-scale it
        auto phase2 = [this, posBoosted, &boostCv, origScale](float t){
            if (t < 0.7f) {
                // Shake in place
                float localT = t / 0.7f;
                float intensity = localT * 4.f;
                float shakeX = std::sin(localT * 40.f) * intensity + std::sin(localT * 80.f) * intensity * 0.5f;
                float shakeY = std::cos(localT * 40.f) * intensity * 0.7f;
                boostCv.cardSprite.setPosition({posBoosted.x + shakeX, posBoosted.y + shakeY});
                boostCv.cardSprite.setScale(origScale);
            } else {
                // Pop-scale: overshoot then settle
                float localT = (t - 0.7f) / 0.3f;
                float eased;
                if (localT < 0.5f) {
                    float lt = localT / 0.5f;
                    eased = lt * lt * 1.15f;
                } else {
                    float lt = (localT - 0.5f) / 0.5f;
                    eased = 1.15f + (1.f - 1.15f) * lt;
                }
                boostCv.cardSprite.setPosition(posBoosted);
                boostCv.cardSprite.setScale({origScale.x * eased, origScale.y * eased});
            }
        };
        auto phase2Finish = [this, &boostCv, origScale, posBoosted, boostAmount, onFinish]() {
            boostCv.cardSprite.setScale(origScale);
            boostCv.cardSprite.setRotation(sf::degrees(0));
            spawnFloatingText(
                "+" + std::to_string(boostAmount),
                { posBoosted.x, posBoosted.y - 20.f },
                sf::Color(255, 215, 0),
                AnimConfig::POINT_CHANGE_DURATION
            );
            if (onFinish) onFinish();
        };

        neuralGambitSprite1 = std::make_unique<sf::Sprite>(neuralGambitTexture);
        neuralGambitSprite1->setOrigin({32,32});
        neuralGambitSprite1->setPosition(pos1);
        neuralGambitSprite1->setScale({scale * S, scale * S});
        auto onDone1 = [this, cardId1, boostCardId, phase2, phase2Finish](){
            neuralGambitSprite1.reset();
            if (cardId1 == boostCardId){
                add({phase2, phase2Finish, 0, 0.7f});
            }
        };
        playSpriteAnimation(neuralGambitSprite1.get(),
            19, 0, 124, 64, 64, duration, 64, onDone1);

        neuralGambitSprite2 = std::make_unique<sf::Sprite>(neuralGambitTexture);
        neuralGambitSprite2->setOrigin({32,32});
        neuralGambitSprite2->setPosition(pos2);
        neuralGambitSprite2->setScale({scale * S, scale * S});
        auto onDone2 = [this, cardId2, boostCardId, phase2, phase2Finish](){
            neuralGambitSprite2.reset();
            if (cardId2 == boostCardId){
                add({phase2, phase2Finish, 0, 0.7f});
            }
        };
        playSpriteAnimation(neuralGambitSprite2.get(),
            19, 0, 124, 64, 64, duration, 64, onDone2);
    }
    void shakeBorrowedPlayerVisual(PlayerVisual& playerVisual, float duration, std::function<void()> onFinish = nullptr){
        sf::Vector2f pos = playerVisual.playerSprite.getPosition();
        sf::Vector2f origScale = playerVisual.playerSprite.getScale();
        // Phase 2: shake boosted card, then pop-scale it
        auto phase1 = [this, pos, &playerVisual, origScale](float t){
            if (t < 0.7f) {
                float localT = t / 0.7f;
                float intensity = localT * 4.f;

                float shakeX = std::sin(localT * 40.f) * intensity
                            + std::sin(localT * 80.f) * intensity * 0.5f;
                float shakeY = std::cos(localT * 40.f) * intensity * 0.7f;

                playerVisual.playerSprite.setPosition({pos.x + shakeX, pos.y + shakeY});
            } else {
                // reset cleanly after shake
                playerVisual.playerSprite.setPosition(pos);
            }
        };

        auto phase1Finish = [this, &playerVisual, origScale, pos, onFinish]() {
            playerVisual.playerSprite.setScale(origScale);
            playerVisual.playerSprite.setRotation(sf::degrees(0));
            if (onFinish) {
                onFinish();
                borrowedPlayerVisual = nullptr;
            }
        };
        add({phase1, phase1Finish, 0, duration});

        borrowedPlayerVisual = &playerVisual;
    }
    void renderFatalDealEffect(){
        if (fatalDealSprite1)
            window.draw(*fatalDealSprite1);
        if (fatalDealSprite2)
            window.draw(*fatalDealSprite2);
    }
    void renderNeuralGambitEffect(){
        if (neuralGambitSprite1)
            window.draw(*neuralGambitSprite1);
        if (neuralGambitSprite2)
            window.draw(*neuralGambitSprite2);
    }
    void renderBorrowedPlayerVisual(){
        if (borrowedPlayerVisual) window.draw(borrowedPlayerVisual->playerSprite);
    }
    // Chronosphere effect — reuses holy spritesheet as placeholder
    void playChronosphereEffect(sf::Vector2f position, bool isSnapshot,
                                float scale = 2.f, float duration = 0.8f)
    {
        float S = GameSettings::instance().S;
        chronosphereSprite = std::make_unique<sf::Sprite>(chronosphereTexture);
        chronosphereSprite->setOrigin({32, 32});
        chronosphereSprite->setPosition(position);
        chronosphereSprite->setScale({scale * S, scale * S});

        auto onDone = [this](){ chronosphereSprite.reset(); };
        playSpriteAnimation(chronosphereSprite.get(),
            19, 0, 64, 64, 64, duration, 64, onDone);
    }
    void renderChronosphereEffect(){
        if (chronosphereSprite)
            window.draw(*chronosphereSprite);
    }

    // Destiny Deflect effect — reuses holy spritesheet with cyan tint
    void playDestinyDeflectEffect(sf::Vector2f position, float scale = 2.f, float duration = 0.8f) {
        float S = GameSettings::instance().S;
        destinyDeflectSprite = std::make_unique<sf::Sprite>(destinyDeflectTexture);
        destinyDeflectSprite->setOrigin({32, 32});
        destinyDeflectSprite->setPosition(position);
        destinyDeflectSprite->setScale({scale * S, scale * S});
        auto onDone = [this](){ destinyDeflectSprite.reset(); };
        playSpriteAnimation(destinyDeflectSprite.get(),
            19, 0, 320, 64, 64, duration, 64, onDone);
    }

    void renderDestinyDeflectEffect() {
        if (destinyDeflectSprite)
            window.draw(*destinyDeflectSprite);
    }

    // Redirect animation: slide card from current position to target player's hand
    void addRedirectAnimation(int cardId, int toPlayerId, int handIndex, int fromPlayerId) {
        CardVisual& card = visualState.getCardVisual(cardId);
        sf::Vector2f startPos = card.cardSprite.getPosition();

        int numPlayers = (int)gameState.getAllPlayerInfo().size();
        sf::Vector2f seatPos = visualState.getPlayerSeatPos(toPlayerId, numPlayers);
        sf::Vector2f endPos = {
            seatPos.x + UILayout::HAND_OFFSET_X() + handIndex * UILayout::CARD_SPACING(),
            seatPos.y + UILayout::HAND_OFFSET_Y()
        };

        auto localBounds = card.cardSprite.getLocalBounds();
        card.cardSprite.setOrigin({localBounds.size.x / 2.f, localBounds.size.y / 2.f});
        card.cardSprite.setPosition(startPos);

        float origScaleX = card.cardSprite.getScale().x;
        float origScaleY = card.cardSprite.getScale().y;

        Animation anim;
        anim.time = 0;
        anim.duration = AnimConfig::CARD_DRAW_DURATION;
        anim.update = [this, cardId, startPos, endPos](float t) {
            float easedT = t * t * (3.f - 2.f * t); // smoothstep
            CardVisual& cv = visualState.getCardVisual(cardId);
            float cx = startPos.x + (endPos.x - startPos.x) * easedT;
            float cy = startPos.y + (endPos.y - startPos.y) * easedT;
            cv.cardSprite.setPosition({cx, cy});
        };
        anim.onFinish = [this, cardId, origScaleX, origScaleY, fromPlayerId]() {
            CardVisual& cv = visualState.getCardVisual(cardId);
            cv.cardSprite.setScale({origScaleX, origScaleY});
            // Reposition source player's remaining hand inline — avoids a
            // separate HAND_REPOSITIONED cutscene that would clearInput()
            repositionHand(fromPlayerId);
        };
        add(anim);
    }
};

#endif