#ifndef VISUALSTATE_H
#define VISUALSTATE_H
#include <vector>
#include <functional>
#include <optional>
#include <algorithm>
#include <memory>
#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>

#include "Player.h"

#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <SFML/Network.hpp>
#include <SFML/Audio.hpp>

// ============================================================================
// Hot-reloadable layout config — reads from assets/layout.txt every call
// ============================================================================
struct LayoutConfig {
    float topY = 90.f;
    float botY_offset = 110.f;
    float midY_offset = 30.f;
    float offsetX = 50.f;
    float seatRightX_offset = 200.f;

    void loadFromFile(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) return;
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;
            auto eq = line.find('=');
            if (eq == std::string::npos) continue;
            std::string key = line.substr(0, eq);
            float val = std::stof(line.substr(eq + 1));
            if (key == "topY") topY = val;
            else if (key == "botY_offset") botY_offset = val;
            else if (key == "midY_offset") midY_offset = val;
            else if (key == "offsetX") offsetX = val;
            else if (key == "seatRightX_offset") seatRightX_offset = val;
        }
    }
};
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

    // Scale-aware layout helpers — all base values are for 683x384
    inline float S() { return GameSettings::instance().S; }
    inline sf::Vector2f CARD_SIZE()   { float s = S(); return {42.f * s, 63.f * s}; }
    inline sf::Vector2f BUTTON_SIZE() { float s = S(); return {120.f * s, 40.f * s}; }
    inline float CARD_SPACING()       { return 41.f * S(); }
    inline float DECK_X_RATIO()       { return 0.07f; }  // ratio, no scaling needed
    inline float HAND_OFFSET_X()      { return 18.f * S(); }
    inline float HAND_OFFSET_Y()      { return 26.f * S(); }
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
    int pinCount = 0;      // >0 means pinned: reconcile + enforceVisibility skip this card
    bool isPinned() const { return pinCount > 0; }
    void pin() { pinCount++; }
    void unpin() { if (pinCount > 0) pinCount--; }
    int rankBonus = 0;
    void draw(sf::RenderWindow& window);
    bool isClicked(sf::Vector2f mousePos);
};
// ============================================================================
// Player Visual
// ============================================================================
struct PlayerVisual {
    PlayerVisual(sf::Sprite sprite)
        : playerSprite(std::move(sprite)) {}
    sf::Sprite playerSprite;
    int playerId = -1;
    bool isTarget = false;
    bool highlighted = false;
    bool isHost = false;
    SkillName skillName = SkillName::UNDEFINED;
    int skillUses = 100;
    int points = 0;
    sf::Vector2f seatPostion = {0.f, 0.f};

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
        bool cheatOn=0;
        bool reconcileBlocked = false;
        int localPlayerId = 0;  // cards owned by this player always show face-up
        LayoutConfig layoutConfig;
    public:
        VisualState(sf::RenderWindow& window, GameState& gameState);
        std::vector<CardVisual>& getCardVisuals() { return cardVisuals; }
        void setReconcileBlocked(bool blocked) { reconcileBlocked = blocked; }
        void setLocalPlayerId(int id) { localPlayerId = id; }
        int getLocalPlayerId() const { return localPlayerId; }
        //setters
        void buildCardVisuals(Deck& deck, std::vector<Player>& players);
        void rebuildFromState(Deck& deck, std::vector<Player>& players);
        void reconcile(GameState& gs);  // lightweight sync: update metadata only, don't touch positions
        //getters
        sf::Font& getFont() { return font; }

        void flipCardVisualFaceUp(int cardId);
        void flipCardVisualFaceDown(int cardId);
        void enforceVisibility();  // iron rule: local player's cards face-up, others follow game logic
        CardVisual& getCardVisual(int cardId) {
            for (auto& card : cardVisuals) {
                if (card.cardId == cardId) {
                    return card;
                }
            }
            return cardVisuals[0];
        }
        sf::Vector2f getPlayerSeatPos(int playerId, int totalPlayers) {
            layoutConfig.loadFromFile("assets/layout.txt");
            float S = GameSettings::instance().S;

            float w = static_cast<float>(window.getSize().x);
            float h = static_cast<float>(window.getSize().y);
            float topY = layoutConfig.topY * S;
            float botY = h - layoutConfig.botY_offset * S;
            float midY = h / 2.f - layoutConfig.midY_offset * S;
            float offX = layoutConfig.offsetX * S;

            auto rowX = [&](int i, int n) -> float {
                float spacing = w / static_cast<float>(n + 1);
                return spacing * (i + 1) - offX;
            };

            int visualIndex = (playerId - localPlayerId + totalPlayers) % totalPlayers;

            switch (totalPlayers) {
                case 2:
                    if (visualIndex == 0) return { rowX(0, 1), botY };
                    return { rowX(0, 1), topY };

                case 3:
                    // YOU bottom, others go right -> left on top
                    if (visualIndex == 0) return { rowX(0, 1), botY };
                    return { rowX(2 - visualIndex, 2), topY };

                case 4:
                    // Bottom: YOU (left), next (right)
                    // Top: next (right), next (left)
                    if (visualIndex < 2)
                        return { rowX(visualIndex, 2), botY };
                    return { rowX(3 - visualIndex, 2), topY };

                case 5:
                    // Bottom: YOU, next
                    // Top: next → next → next (right → left)
                    if (visualIndex < 2)
                        return { rowX(visualIndex, 2), botY };
                    return { rowX(4 - visualIndex, 3), topY };

                case 6:
                    // Bottom: YOU → right → far-right
                    // Top: far-right → left → far-left
                    if (visualIndex < 3)
                        return { rowX(visualIndex, 3), botY };
                    return { rowX(5 - visualIndex, 3), topY };

                case 7:
                    // Bottom: 3
                    // Top: 3 (reversed)
                    // Last: right side
                    if (visualIndex < 3)
                        return { rowX(visualIndex, 3), botY };
                    if (visualIndex < 6)
                        return { rowX(5 - visualIndex, 3), topY };
                    return { w - layoutConfig.seatRightX_offset * S, midY };

                default:
                    if (visualIndex == 0) return { rowX(0, 1), botY };
                    return { rowX((totalPlayers - 1) - visualIndex, totalPlayers - 1), topY };
            }
        }
};

#endif