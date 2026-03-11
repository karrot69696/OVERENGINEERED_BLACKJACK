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
        int activePlayerId = -1;
        sf::Font font;
        sf::Texture cardTexture;
        sf::Texture tableTexture;
        sf::Texture playerIcon;
        std::vector<sf::Sprite> playerSprite;
        
    public:
        UIManager(sf::RenderWindow& window, GameState& gameState,VisualState& visualState, std::vector<CardVisual>& cardVisuals);

        // Call once per frame
        void handleEvent(const std::optional<sf::Event>& event);
        void render();
        int cheatOn = 1;
        // Called by game logic to tell UI what input is needed
        void requestActionInput(int playerId);                          // show Hit/Stand/Skill buttons
        void requestTargetInput(int playerId);                          // show card targeting overlay
        void clearInput();                                              // hide all overlays
        //void buildCardVisuals();
        //void targetStateHandler();

        // Targeting state
        PlayerTargeting pendingTargeting;
        void confirmTargeting();

        // Layout helpers
        sf::Color getPhaseNameColor();

        // Callbacks — set these from RoundManager
        std::function<void(PlayerAction)> onActionChosen;
        std::function<void(PlayerTargeting)> onTargetChosen;
        
        // Sub-renderers
        void renderTable();
        void renderCards();
        void renderHUD();
        void renderActionMenu();
        void renderTargetingOverlay_Deliverance();
};

#endif