#ifndef UIMANAGER_H
#define UIMANAGER_H

#include <SFML/Graphics.hpp>
#include <vector>
#include <functional>
#include "../lowLevelEntities/GameState.h"
#include "../lowLevelEntities/Player.h"

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
// Card Visual
// ============================================================================
class CardVisual {
    public:
        CardVisual();
        sf::RectangleShape shape;
        sf::Text rankSuitText;
        int cardIndex;      // index in player's hand
        int ownerId;
        bool highlighted = false;
        bool isTarget = false;

        void draw(sf::RenderWindow& window);
        bool isClicked(sf::Vector2f mousePos);
};

// ============================================================================
// UIManager
// ============================================================================
class UIManager {
public:
    UIManager(sf::RenderWindow& window, GameState& gameState);

    // Call once per frame
    void handleEvent(const sf::Event& event);
    void render();

    // Called by game logic to tell UI what input is needed
    void requestActionInput(int playerId);                          // show Hit/Stand/Skill buttons
    void requestTargetInput(int playerId);                          // show card targeting overlay
    void clearInput();                                              // hide all overlays

    // Callbacks — set these from RoundManager
    std::function<void(PlayerAction)> onActionChosen;
    std::function<void(PlayerTargeting)> onTargetChosen;

private:
    sf::RenderWindow& window;
    GameState& gameState;
    sf::Font font;

    // Sub-renderers
    void renderTable();
    void renderHands();
    void renderHUD();
    void renderActionMenu();
    void renderTargetingOverlay();

    // Action menu buttons
    std::vector<Button> actionButtons;
    bool showActionMenu = false;
    bool showTargetingOverlay = false;
    int activePlayerId = -1;

    // Card visuals built each frame from GameState
    std::vector<CardVisual> cardVisuals;
    void buildCardVisuals();

    // Targeting state
    PlayerTargeting pendingTargeting;
    void confirmTargeting();

    // Layout helpers
    sf::Vector2f getPlayerSeatPos(int playerId, int totalPlayers);
    sf::Color getPhaseColor();
};

#endif