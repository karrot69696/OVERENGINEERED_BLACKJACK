#ifndef UIMANAGER_H
#define UIMANAGER_H
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


// ============================================================================
// Simple Button
// ============================================================================
struct Button {
    sf::RectangleShape shape;
    sf::Text label;
    std::function<void()> onClick;
    bool visible = true;

    Button(sf::Font& font, const std::string& text, sf::Vector2f pos, sf::Vector2f size);
    void draw(sf::RenderWindow& window);
    bool isClicked(sf::Vector2f mousePos);
};
// ============================================================================
// UIManager
// ============================================================================
class UIManager {
    private:

        sf::RenderWindow& window;
        GameState& gameState;
        VisualState& visualState;
        std::vector<CardVisual>& cardVisuals;

        // Action menu buttons
        std::vector<Button> actionButtons;
        bool showActionMenu = false;
        bool showTargetingOverlay_Deliverance = false;
        bool showTargetingOverlay_NeuralGambit = false;
        int activePlayerId = -1;

        enum class NgStep { PICK_PLAYER, WAITING_FOR_PICKS, PICK_BOOST_CARD };
        NgStep ngStep = NgStep::PICK_PLAYER;
        std::vector<int> ngTargetPlayerIds = {};
        std::vector<int> ngTargetCardIds = {};     // [0],[1] = the two revealed card IDs for boost pick

        // Pick-card overlay (shown to target players or skill user for boost pick)
        bool showPickCardOverlay = false;
        std::vector<int> pickCardAllowedIds;

        // Reactive skill prompt overlay (Yes/No with timer)
        bool showReactivePrompt = false;
        std::string reactivePromptSkillName;
        std::string reactivePromptExtraInfo;
        float reactivePromptDuration = 5.f;
        sf::Clock reactivePromptClock;
        sf::Font font;
        sf::Texture cardTexture;
        sf::Texture tableTexture;
        sf::Texture playerIcon;
        sf::Texture cardRankPatch;
        std::unique_ptr<sf::Sprite> cardRankPatchSprite;
        std::vector<PlayerVisual> playerVisuals;
        sf::Vector2f mousePos = {0.f, 0.f};
        int hoveredCardId = -1;
        int hoveredPlayerId = -1;
        sf::Clock shakeClock;
        int cheatOn = 0;
        bool showDebugTooltip = false;
        int gameLogMode = 0;  // 0=off, 1=recent (fading), 2=full history
        std::vector<int> borrowedPlayerVisualIds;

    public:
        UIManager(sf::RenderWindow& window, GameState& gameState,VisualState& visualState, std::vector<CardVisual>& cardVisuals);

        // Call once per frame
        void handleEvent(const std::optional<sf::Event>& event);
        void render();

        int getActivePlayerId() const { return activePlayerId; }
        std::vector<PlayerVisual>& getPlayerVisuals() { return playerVisuals; }
        std::vector<int>& getBorrowedPlayerVisualIds() { return borrowedPlayerVisualIds; }
        // Called by game logic to tell UI what input is needed
        void requestActionInput(int playerId);                          // show Hit/Stand/Skill buttons
        void requestTargetInput(int playerId);                          // show card targeting overlay
        void clearInput();                                              // hide all overlays

        // Targeting state
        PlayerTargeting pendingTargeting;
        void confirmTargeting();

        // Layout helpers
        sf::Color getPhaseNameColor();

        // Callbacks — set these from RoundManager
        std::function<void(PlayerAction)> onActionChosen;
        std::function<void(PlayerTargeting)> onTargetChosen;
        std::function<void(bool accepted)> onReactiveResponse;
        
        // Called by Phase::ngTickPending — target player must pick one of allowedCardIds
        void requestPickCard(const std::vector<int>& allowedCardIds);
        // Called by Phase::ngTickPending — skill user picks which of two cards gets boosted
        void requestBoostPickInput(int card1Id, int card2Id);

        // Reactive skill prompt (Yes/No with timer countdown)
        void requestReactivePrompt(const std::string& skillName, std::string& extraInfo, float timerDuration);
        void hideReactivePrompt();

        // Sub-renderers
        void renderTable();
        void renderCards();
        void renderHUD();
        void renderPlayerVisuals();
        void renderActionMenu();
        void renderTargetingOverlay_Deliverance();
        void renderTargetingOverlay_NeuralGambit();
        void renderPickCardOverlay();
        void renderReactivePrompt();
        void renderHoverTooltip();
        void renderGameLog();
};

#endif