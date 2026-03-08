#include "UIManager.h"
#include <map>


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

    const sf::Vector2f CARD_SIZE   = {60.f, 90.f};
    const sf::Vector2f BUTTON_SIZE = {120.f, 40.f};
    const float CARD_SPACING       = 70.f;
}

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
// CardVisual
// ============================================================================
void CardVisual::draw(sf::RenderWindow& window) {
    window.draw(cardSprite);
}

bool CardVisual::isClicked(sf::Vector2f mousePos) {
    return cardSprite.getGlobalBounds().contains(mousePos);
}

// ============================================================================
// UIManager
// ============================================================================
UIManager::UIManager(sf::RenderWindow& window, GameState& gameState)
    : window(window), gameState(gameState) {

    std::filesystem::path fontPath =            "../assets/fonts/PixeloidSans.ttf";
    std::filesystem::path cardTexturePath =     "../assets/fonts/CuteCards.png";

    if (!font.openFromFile(fontPath)) {
        std::cerr << "[UIManager] Failed to load font from assets/fonts/PixeloidSans.ttf" << std::endl;
    }

    if (!cardTexture.loadFromFile(cardTexturePath)) {
        std::cerr << "[UIManager] Failed to load card texture from assets/fonts/CuteCards.png" << std::endl;
    }

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

    clearInput();
}

// ============================================================================
// Public Interface
// ============================================================================
void UIManager::requestActionInput(int playerId) {
    activePlayerId = playerId;
    showActionMenu = true;
    showTargetingOverlay_Deliverance = false;
    pendingTargeting = {};
}

void UIManager::requestTargetInput(int playerId) {
    activePlayerId = playerId;
    showActionMenu = false;
    showTargetingOverlay_Deliverance = true;
    pendingTargeting = {};
}

void UIManager::clearInput() {
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
                        //if one card exists clicking won't work
                       ) {

                    std::cout << "[UIManager] Card has been clicked: ownerId=" << cv.ownerId << ", cardIndex=" << cv.cardIndex << std::endl;
                    //if not targetted and no pendingTarget
                    if (!cv.isTarget && pendingTargeting.targetCards.empty()) {
                        // Create the exact copy of the card chosen and store it in target.targetCards
                            PlayerTargeting target;
                            Card chosenCard(
                                gameState.getCardSuit(activePlayerId, cv.cardIndex),
                                gameState.getCardRank(activePlayerId, cv.cardIndex),
                                gameState.isCardFaceUp(activePlayerId, cv.cardIndex)
                            );

                            chosenCard.setOwnerId(cv.ownerId);
                            chosenCard.setHandIndex(cv.cardIndex);

                            target.targetCards.push_back(chosenCard);
                            
                        //store the ownerId
                            target.targetPlayerIds.push_back(cv.ownerId);

                        //finish one target
                            pendingTargeting = target;
                            cv.isTarget = true;
                    }
                    else if ( pendingTargeting.targetCards.size() > 0 ){
                        if (cv.ownerId == pendingTargeting.targetCards.front().getOwnerId()
                            && cv.cardIndex == pendingTargeting.targetCards.front().getHandIndex()){
                                
                                pendingTargeting= {};
                                cv.isTarget = false;
                            }
                    }
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
    buildCardVisuals();
    renderTable();
    renderHands();
    renderHUD();
    if (showActionMenu)      renderActionMenu();
    if (showTargetingOverlay_Deliverance) renderTargetingOverlay_Deliverance();
}

void UIManager::renderTable() {
    sf::RectangleShape table({ (float)window.getSize().x, (float)window.getSize().y });
    table.setFillColor(UILayout::TABLE_GREEN);
    window.draw(table);
}

void UIManager::renderHands() {
    for (auto& cv : cardVisuals) {
        window.draw(cv.cardSprite);
    }
}

void UIManager::renderHUD() {
    // PhaseName label top-left
    sf::Text phaseText(font, gameState.phaseToString(gameState.getPhaseName()), 16);
    phaseText.setFillColor(sf::Color::White);
    phaseText.setPosition({10.f, 10.f});
    window.draw(phaseText);

    // Per-player points + seat label
    auto players = gameState.getAllPlayerInfo(); // you'll add this getter (see notes)
    for (auto& info : players) {
        sf::Vector2f seatPos = getPlayerSeatPos(info.playerId, (int)players.size());

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
    // Highlight cards belonging to the active player
    for (auto& cv : cardVisuals) {
        if (cv.ownerId == activePlayerId) {
            sf::RectangleShape highlight(UILayout::CARD_SIZE + sf::Vector2f{6.f, 6.f});
            highlight.setPosition(cv.cardSprite.getPosition() - sf::Vector2f{3.f, 3.f});
            highlight.setFillColor(sf::Color::Transparent);
            highlight.setOutlineThickness(2.f);
            highlight.setOutlineColor(cv.isTarget ? UILayout::CARD_TARGET : UILayout::CARD_HIGHLIGHT);
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
// Card Visual Builder
// ============================================================================
// Sprite sheet: 15 columns (A,2..10,J,Q,K,Joker,Back), 4 rows (Spades,Diamonds,Clubs,Hearts)
static int cardSpriteCol(Rank rank) {
    return static_cast<int>(rank) - 1; // Ace=1 -> col 0, King=13 -> col 12
}
static int cardSpriteRow(Suit suit) {
    switch (suit) {
        case Suit::Spades:   return 0;
        case Suit::Diamonds: return 1;
        case Suit::Clubs:    return 2;
        case Suit::Hearts:   return 3;
        default:             return 0;
    }
}

void UIManager::buildCardVisuals() {
    // Save isTarget state before rebuilding
    std::map<std::pair<int,int>, bool> targetState;
    for (auto& cv : cardVisuals) {
        targetState[{cv.ownerId, cv.cardIndex}] = cv.isTarget;
    }

    cardVisuals.clear();
    auto players = gameState.getAllPlayerInfo();

    sf::Vector2u texSize = cardTexture.getSize();
    int cellW = (int)texSize.x / 15;
    int cellH = (int)texSize.y / 4;
    float scaleX = UILayout::CARD_SIZE.x / cellW;
    float scaleY = UILayout::CARD_SIZE.y / cellH;

    for (auto& player : players) {
        sf::Vector2f seatPos = getPlayerSeatPos(player.playerId, (int)players.size());

        for (int i = 0; i < (int)player.cardsInHand.size(); i++) {
            const Card& card = player.cardsInHand[i];
            bool showFace = cheatOn || card.isFaceUp();

            int col, row;
            if (showFace) {
                col = cardSpriteCol(card.getRank());
                row = cardSpriteRow(card.getSuit());
            } else {
                col = 14; row = 2; // black striped card back
            }

            sf::Sprite sprite(cardTexture);
            sprite.setTextureRect(sf::IntRect({col * cellW, row * cellH}, {cellW, cellH}));
            sprite.setScale({scaleX, scaleY});
            sprite.setPosition({ seatPos.x + i * UILayout::CARD_SPACING, seatPos.y });

            cardVisuals.emplace_back(player.playerId, card.getHandIndex(), std::move(sprite));

            // Restore isTarget state
            auto it = targetState.find({player.playerId, i});
            if (it != targetState.end()) {
                cardVisuals.back().isTarget = it->second;
            }
        }
    }
}
// ============================================================================
// Layout Helpers
// ============================================================================
sf::Vector2f UIManager::getPlayerSeatPos(int playerId, int totalPlayers) {
    // Spread players evenly along the bottom, host at top center
    float w = window.getSize().x;
    float h = window.getSize().y;

    if (playerId == 0) {
        // Host sits at top
        return { w / 2.f - 100.f, 60.f };
    }

    float spacing = w / (float)(totalPlayers);
    return { spacing * playerId - 30.f, h - 200.f };
}

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