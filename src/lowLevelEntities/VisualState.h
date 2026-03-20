#ifndef VISUALSTATE_H
#define VISUALSTATE_H
#include <vector>
#include <functional>
#include <optional>
#include <algorithm>
#include <memory>
#include <iostream>

#include "Player.h"

#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <SFML/Network.hpp>
#include <SFML/Audio.hpp>
// ============================================================================
// Layout Constants
// ============================================================================
namespace UILayout {
    const sf::Color TABLE_GREEN    = sf::Color(34, 85, 34);
    const sf::Color CARD_WHITE     = sf::Color(245, 245, 245);
    const sf::Color CARD_HIGHLIGHT = sf::Color(255, 220, 50);
    const sf::Color CARD_TARGET    = sf::Color(255, 80, 80);
    const sf::Color BUTTON_NORMAL  = sf::Color(60, 60, 80);
    const sf::Color BUTTON_HOVER   = sf::Color(90, 90, 120);
    const sf::Color HUD_BG         = sf::Color(20, 20, 20, 180);

    const sf::Vector2f CARD_SIZE   = {42.f, 63.f};
    const sf::Vector2f BUTTON_SIZE = {120.f, 40.f};
    const float CARD_SPACING       = 49.f;
    const float DECK_X_RATIO       = 0.07f;  // deck X as fraction of window width
}
// ============================================================================
// Card Visual
// ============================================================================
struct CardVisual {
    CardVisual(sf::Sprite sprite)
        : cardSprite(std::move(sprite)) {}
    sf::Sprite cardSprite;
    int cardId=-1;
    int cardIndex=-1;      // index in player's hand
    int ownerId=-1;
    CardLocation location;
    bool highlighted = false;
    bool isTarget = false;
    bool faceUp = true;

    void draw(sf::RenderWindow& window);
    bool isClicked(sf::Vector2f mousePos);
};
// ============================================================================
// VisualState
// ============================================================================
class VisualState {
    private:
        sf::RenderWindow& window;
        GameState& gameState;
        std::vector<CardVisual> cardVisuals;
        sf::Texture cardTexture;
        sf::Font font;
        bool cheatOn=1;
        bool reconcileBlocked = false;
    public:
        VisualState(sf::RenderWindow& window, GameState& gameState);
        std::vector<CardVisual>& getCardVisuals() { return cardVisuals; }
        void setReconcileBlocked(bool blocked) { reconcileBlocked = blocked; }
        //setters
        void buildCardVisuals(Deck& deck, std::vector<Player>& players);
        void rebuildFromState(Deck& deck, std::vector<Player>& players);
        void reconcile(GameState& gs);  // lightweight sync: update metadata only, don't touch positions
        //getters
        sf::Font& getFont() { return font; }

        CardVisual& getCardVisual(int cardId) { 
            for (auto& card : cardVisuals) {
                if (card.cardId == cardId) {
                    return card;
                }
            }
            return cardVisuals[0];
        }
        sf::Vector2f getPlayerSeatPos(int playerId, int totalPlayers) {
            // Spread players evenly along the bottom, host at top center
            float w = window.getSize().x;
            float h = window.getSize().y;

            if (playerId == 0) {
                // Host sits at top
                return { w / 2.f - 100.f, 60.f };
            }

            float spacing = w / (float)(totalPlayers);
            return { spacing * playerId - 30.f, h - 130.f };
        }
};

#endif