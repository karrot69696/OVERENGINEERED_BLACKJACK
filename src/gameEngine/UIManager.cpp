#include "UIManager.h"
#include <map>
// ============================================================================
// Button
// ============================================================================
Button::Button(sf::Font& font, const std::string& text, sf::Vector2f pos, sf::Vector2f size)
    : label(font, text, 16)
{
    shape.setPosition(pos);
    shape.setSize(size);
    shape.setFillColor(UILayout::BUTTON_NORMAL);
    shape.setOutlineThickness(1.f);
    shape.setOutlineColor(sf::Color::White);

    label = sf::Text(font, text, 16);
    // Center label in button
    sf::FloatRect bounds = label.getLocalBounds();
    label.setPosition({
        pos.x + size.x / 2.f - bounds.size.x / 2.f,
        pos.y + size.y / 2.f - bounds.size.y / 2.f - 4.f
    });
}

void Button::draw(sf::RenderWindow& window) {
    if (!visible) return;
    window.draw(shape);
    window.draw(label);
}

bool Button::isClicked(sf::Vector2f mousePos) {
    return visible && shape.getGlobalBounds().contains(mousePos);
}
// ============================================================================
// UIManager
// ============================================================================
UIManager::UIManager(sf::RenderWindow& window, GameState& gameState,VisualState& visualState, std::vector<CardVisual>& cardVisuals)
    : window(window), gameState(gameState),visualState(visualState), cardVisuals(cardVisuals) {

    font = visualState.getFont();
    
    // Build action buttons (hidden by default)
    float btnY = window.getSize().y - 60.f;
    float btnStartX = window.getSize().x / 2.f - 190.f;

    actionButtons.emplace_back(font, "Hit",   sf::Vector2f{btnStartX,         btnY}, UILayout::BUTTON_SIZE);
    actionButtons.emplace_back(font, "Stand", sf::Vector2f{btnStartX + 130.f, btnY}, UILayout::BUTTON_SIZE);
    actionButtons.emplace_back(font, "Skill", sf::Vector2f{btnStartX + 260.f, btnY}, UILayout::BUTTON_SIZE);

    actionButtons[0].onClick = [this]() {
        if (onActionChosen) onActionChosen(PlayerAction::HIT);
    };
    actionButtons[1].onClick = [this]() {
        if (onActionChosen) onActionChosen(PlayerAction::STAND);
    };
    actionButtons[2].onClick = [this]() {
        if (onActionChosen) onActionChosen(PlayerAction::SKILL_REQUEST);
        // Switch to targeting mode
        showActionMenu = false;
        showTargetingOverlay_Deliverance = true;
    };

    std::filesystem::path tableTexturePath = "../assets/images/crazyJackBG.png";
    if (!tableTexture.loadFromFile(tableTexturePath)) {
        std::cerr << "[UIManager] Failed to load tableTexture from assets/images/crazyJackBG.png" << std::endl;
    }

    clearInput();

    std::filesystem::path playerIconPath = "../assets/images/playerIcon.png";
    if (!playerIcon.loadFromFile(playerIconPath)) {
        std::cerr << "[UIManager] Failed to load playerIcon from assets/images/playerIcon.png" << std::endl;
    }

    // Player 0: start x=288 y=160, crop 96x96
    sf::Vector2i cropSize = { 195, 195 };
    sf::Sprite p0(playerIcon);
    p0.setTextureRect(sf::IntRect({772, 194},cropSize));
    playerSprite.push_back(p0);

    // Player 1: start x=64 y=224, crop 96x96
    sf::Sprite p1(playerIcon);
    p1.setTextureRect(sf::IntRect({62, 212}, cropSize));
    playerSprite.push_back(p1);

    // Player 2: start x=512 y=224, crop 96x96
    sf::Sprite p2(playerIcon);
    p2.setTextureRect(sf::IntRect({536, 240}, cropSize));
    playerSprite.push_back(p2);
}

// ============================================================================
// Public Interface
// ============================================================================
void UIManager::requestActionInput(int playerId) {
    std::cout << "[UIManager] Request Action Input Menu" << std::endl;
    activePlayerId = playerId;
    showActionMenu = true;
    showTargetingOverlay_Deliverance = false;
    pendingTargeting = {};
}

void UIManager::requestTargetInput(int playerId) {
    std::cout << "[UIManager] Request Target Input Menu" << std::endl;
    activePlayerId = playerId;
    showActionMenu = false;
    showTargetingOverlay_Deliverance = true;
    pendingTargeting = {};
}

void UIManager::clearInput() {
    std::cout << "[UIManager] Clear Input Menu" << std::endl;
    showActionMenu = false;
    showTargetingOverlay_Deliverance = false;
    activePlayerId = -1;
    pendingTargeting = {};
}

// ============================================================================
// Event Handling
// ============================================================================
void UIManager::handleEvent(const std::optional<sf::Event>& event) {
    if (!event.has_value()) return;

    // Track mouse position for hover effects
    if (const auto* mouseMoved = event->getIf<sf::Event::MouseMoved>()) {
        mousePos = {(float)mouseMoved->position.x, (float)mouseMoved->position.y};
    }

    if (const auto* mouseBtn = event->getIf<sf::Event::MouseButtonPressed>()) {
        if (mouseBtn->button != sf::Mouse::Button::Left) return;
        std::cout << "[UIManager] Mouse click at (" << mouseBtn->position.x << ", " << mouseBtn->position.y << ")\n";
        sf::Vector2f mousePos = { (float)mouseBtn->position.x, (float)mouseBtn->position.y };

        // Action menu clicks
        if (showActionMenu) {
            for (auto& btn : actionButtons) {
                if (btn.isClicked(mousePos) && btn.onClick) {
                    std::cout << "[UIManager] Button '" << btn.label.getString().toAnsiString() << "' clicked\n";
                    btn.onClick();
                    return;
                }
            }
        }

        // Card targeting clicks
        if (showTargetingOverlay_Deliverance) {
            for (auto& cv : cardVisuals) {

                if (    cv.ownerId == activePlayerId
                        && cv.isClicked(mousePos)
                       ) {

                    std::cout << "[UIManager] Card has been clicked: ownerId=" << cv.ownerId << ", cardIndex=" << cv.cardIndex << std::endl;

                    // Clicking the already-targeted card: deselect
                    if (cv.isTarget) {
                        cv.isTarget = false;
                        pendingTargeting = {};
                        return;
                    }

                    // Deselect any previously targeted card
                    for (auto& other : cardVisuals) other.isTarget = false;
                    pendingTargeting = {};

                    // Select clicked card
                    PlayerTargeting target;
                    Card chosenCard(
                        gameState.getCardSuit(activePlayerId, cv.cardIndex),
                        gameState.getCardRank(activePlayerId, cv.cardIndex),
                        gameState.isCardFaceUp(activePlayerId, cv.cardIndex)
                    );
                    chosenCard.setOwnerId(cv.ownerId);
                    chosenCard.setHandIndex(cv.cardIndex);
                    target.targetCards.push_back(chosenCard);
                    target.targetPlayerIds.push_back(cv.ownerId);
                    pendingTargeting = target;
                    cv.isTarget = true;
                    return;
                }
            }

            // Confirm button — bottom right
            sf::Vector2f confirmPos = {
                window.getSize().x - 150.f,
                window.getSize().y - 60.f
            };
            sf::FloatRect confirmBounds(confirmPos, UILayout::BUTTON_SIZE);
            if (confirmBounds.contains(mousePos)) {
                confirmTargeting();
            }
        }
    }
}

void UIManager::confirmTargeting() {
    for (auto& cv : cardVisuals) cv.isTarget=false;
    if (onTargetChosen) onTargetChosen(pendingTargeting);
}

// ============================================================================
// Render
// ============================================================================
void UIManager::render() {
    //buildCardVisuals();
    renderTable();
    renderCards();
    renderHUD();
    if (showActionMenu)      renderActionMenu();
    if (showTargetingOverlay_Deliverance) renderTargetingOverlay_Deliverance();
}

void UIManager::renderTable() {
    sf::Sprite table(tableTexture);
    float w = window.getSize().x;
    float h = window.getSize().y;
    float scaleX = w / tableTexture.getSize().x;
    float scaleY = h / tableTexture.getSize().y;
    table.setScale({scaleX,scaleY});
    window.draw(table);
}

void UIManager::renderCards() {
    for (auto& cv : cardVisuals) {
        window.draw(cv.cardSprite);
    }
}

void UIManager::renderHUD() {
    // PhaseName label top-left
    sf::Text phaseText(font, gameState.phaseToString(gameState.getPhaseName()), 16);
    phaseText.setFillColor(getPhaseNameColor());
    phaseText.setPosition({10.f, 10.f});
    window.draw(phaseText);

    // Per-player icon + points + seat label
    auto players = gameState.getAllPlayerInfo();
    for (auto& info : players) {
        sf::Vector2f seatPos = visualState.getPlayerSeatPos(info.playerId, (int)players.size());

        // Draw player icon
        sf::Vector2f iconScale = {0.37f, 0.37f};
        if (info.playerId < (int)playerSprite.size()) {
            playerSprite[info.playerId].setPosition({ seatPos.x - 86.f, seatPos.y - 26.f });
            playerSprite[info.playerId].setScale(iconScale);
            window.draw(playerSprite[info.playerId]);
        }

        std::string label = "P" + std::to_string(info.playerId)
                          + "  " + std::to_string(info.points) + "pts"
                          + "  [" + gameState.skillNameToString(info.skill)
                          + " x" + std::to_string(info.skillUses) + "]";

        sf::Text playerLabel(font, label, 14);
        playerLabel.setFillColor(sf::Color::White);
        playerLabel.setPosition({ seatPos.x, seatPos.y - 20.f });
        window.draw(playerLabel);
    }
}

void UIManager::renderActionMenu() {
    // Dark strip at bottom
    sf::RectangleShape strip({ (float)window.getSize().x, 70.f });
    strip.setFillColor(UILayout::HUD_BG);
    strip.setPosition({ 0.f, (float)window.getSize().y - 70.f });
    window.draw(strip);

    sf::Text prompt(font, "Player " + std::to_string(activePlayerId) + " - choose action:", 16);
    prompt.setFillColor(sf::Color::Yellow);
    prompt.setPosition({ 10.f, (float)window.getSize().y - 65.f });
    window.draw(prompt);

    for (auto& btn : actionButtons) btn.draw(window);
}

void UIManager::renderTargetingOverlay_Deliverance() {
    hoveredCardId = -1;

    // Highlight cards belonging to the active player
    for (auto& cv : cardVisuals) {
        if (cv.ownerId == activePlayerId) {
            auto cardBounds = cv.cardSprite.getGlobalBounds();
            bool hovered = cardBounds.contains(mousePos);

            if (hovered) hoveredCardId = cv.cardId;

            // Shake effect for targeted cards
            if (cv.isTarget) {
                float elapsed = shakeClock.getElapsedTime().asSeconds();
                float shakeOffsetX =
                    std::sin(elapsed * 40.f) * 3.f +
                    std::sin(elapsed * 80.f) * 3.f;

                float shakeOffsetY =
                    std::cos(elapsed * 40.f) * 3.f +
                    std::sin(elapsed * 80.f) * 3.f;
                sf::Vector2f origPos = cv.cardSprite.getPosition();
                cv.cardSprite.setPosition({origPos.x + shakeOffsetX, origPos.y + shakeOffsetY});
                window.draw(cv.cardSprite);
                cv.cardSprite.setPosition(origPos);
            }
            // Hover effect: scale up slightly
            else if (hovered && !cv.isTarget) {
                sf::Vector2f origScale = cv.cardSprite.getScale();
                float hoverScale = 1.15f;
                cv.cardSprite.setScale(origScale * hoverScale);
                cardBounds = cv.cardSprite.getGlobalBounds();
                window.draw(cv.cardSprite);
                cv.cardSprite.setScale(origScale);
            }

            sf::RectangleShape highlight({cardBounds.size.x, cardBounds.size.y});
            highlight.setPosition({cardBounds.position.x, cardBounds.position.y});
            highlight.setFillColor(sf::Color::Transparent);
            highlight.setOutlineThickness(hovered ? 5.f : 3.f);
            highlight.setOutlineColor(cv.isTarget ? UILayout::CARD_TARGET :
                                      hovered ? sf::Color(255, 240, 100) : UILayout::CARD_HIGHLIGHT);
            window.draw(highlight);
        }
    }

    // Confirm button
    Button confirmBtn(font, "Confirm",
        { window.getSize().x - 150.f, (float)window.getSize().y - 60.f },
        UILayout::BUTTON_SIZE);
    confirmBtn.shape.setFillColor(sf::Color(50, 150, 50));
    confirmBtn.draw(window);

    sf::Text prompt(font, "Click a card, then Confirm", 16);
    prompt.setFillColor(sf::Color::Yellow);
    prompt.setPosition({ 10.f, (float)window.getSize().y - 65.f });
    window.draw(prompt);
}
// ============================================================================
// Layout Helpers
// ============================================================================

sf::Color UIManager::getPhaseNameColor() {
    switch (gameState.getPhaseName()) {
        case PhaseName::BATTLE_PHASE:          return sf::Color(200, 50, 50);
        case PhaseName::PLAYER_HIT_PHASE:      return sf::Color(50, 200, 50);
        case PhaseName::HOST_HIT_PHASE:        return sf::Color(200, 150, 50);
        case PhaseName::BLACKJACK_CHECK_PHASE: return sf::Color(200, 200, 50);
        case PhaseName::ROUND_END:             return sf::Color(150, 150, 150);
        default:                           return sf::Color::White;
    }
}