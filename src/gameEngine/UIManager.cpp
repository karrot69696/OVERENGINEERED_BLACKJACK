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
// PlayerVisual
// ============================================================================

void PlayerVisual::draw(sf::RenderWindow& window) {
    window.draw(playerSprite);
}

bool PlayerVisual::isClicked(sf::Vector2f mousePos) {
    return playerSprite.getGlobalBounds().contains(mousePos);
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
        showActionMenu = false;
        if (onActionChosen) onActionChosen(PlayerAction::HIT);
    };
    actionButtons[1].onClick = [this]() {
        showActionMenu = false;
        if (onActionChosen) onActionChosen(PlayerAction::STAND);
    };
    actionButtons[2].onClick = [this]() {
        showActionMenu = false;
        if (onActionChosen) onActionChosen(PlayerAction::SKILL_REQUEST);
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
    PlayerVisual player0(p0);
    playerVisuals.push_back(player0);
    // Player 1: start x=64 y=224, crop 96x96
    sf::Sprite p1(playerIcon);
    p1.setTextureRect(sf::IntRect({62, 212}, cropSize));
    PlayerVisual player1(p1);
    playerVisuals.push_back(player1);

    // Player 2: start x=512 y=224, crop 96x96
    sf::Sprite p2(playerIcon);
    p2.setTextureRect(sf::IntRect({536, 240}, cropSize));
    PlayerVisual player2(p2);
    playerVisuals.push_back(player2);
}

// ============================================================================
// Public Interface
// ============================================================================
void UIManager::requestActionInput(int playerId) {
    if(!showActionMenu) std::cout << "[UIManager] Request Action Input Menu" << std::endl;
    activePlayerId = playerId;
    showActionMenu = true;
    showTargetingOverlay_Deliverance = false;
    pendingTargeting = {};
}

void UIManager::requestTargetInput(int playerId) {
    std::cout << "[UIManager] Request Target Input Menu" << std::endl;
    activePlayerId = playerId;
    showActionMenu = false;
    pendingTargeting = {};
    // Choose overlay based on player's skill
    auto players = gameState.getAllPlayerInfo();
    for (auto& p : players) {
        if (p.playerId == playerId) {
            if (p.skill == SkillName::NEURALGAMBIT) {
                showTargetingOverlay_NeuralGambit = true;
                ngStep = NgStep::PICK_PLAYER;
                ngTargetPlayerIds={};
                ngTargetCardIds = {};
            } else if (p.skill == SkillName::DELIVERANCE) {
                showTargetingOverlay_Deliverance = true;
            }
            return;
        }
    }
}

void UIManager::clearInput() {
    std::cout << "[UIManager] Clear Input Menu" << std::endl;
    showActionMenu = false;
    showTargetingOverlay_Deliverance = false;
    showTargetingOverlay_NeuralGambit = false;
    showPickCardOverlay = false;
    showReactivePrompt = false;
    pickCardAllowedIds = {};
    ngStep = NgStep::PICK_PLAYER;
    ngTargetPlayerIds = {};
    ngTargetCardIds = {};
    activePlayerId = -1;
    pendingTargeting = {};
}

void UIManager::requestPickCard(const std::vector<int>& allowedCardIds) {
    std::cout << "[UIManager] requestPickCard: " << allowedCardIds.size() << " allowed cards" << std::endl;
    showPickCardOverlay = true;
    pickCardAllowedIds = allowedCardIds;
    pendingTargeting = {};
}

void UIManager::requestBoostPickInput(int card1Id, int card2Id) {
    std::cout << "[UIManager] requestBoostPickInput: cards " << card1Id << " and " << card2Id << std::endl;
    ngTargetCardIds = {card1Id, card2Id};
    ngStep = NgStep::PICK_BOOST_CARD;
    showTargetingOverlay_NeuralGambit = true;
}

void UIManager::requestReactivePrompt(const std::string& skillName, float timerDuration) {
    std::cout << "[UIManager] Reactive prompt: " << skillName << " (" << timerDuration << "s)" << std::endl;
    showReactivePrompt = true;
    reactivePromptSkillName = skillName;
    reactivePromptDuration = timerDuration;
    reactivePromptClock.restart();
}

void UIManager::hideReactivePrompt() {
    showReactivePrompt = false;
}

// ============================================================================
// Event Handling
// ============================================================================
void UIManager::handleEvent(const std::optional<sf::Event>& event) {
    if (!event.has_value()) return;

    // Track mouse position for hover effects (map pixel → world coords for resized window)
    if (const auto* mouseMoved = event->getIf<sf::Event::MouseMoved>()) {
        mousePos = window.mapPixelToCoords({mouseMoved->position.x, mouseMoved->position.y});
    }

    if (const auto* mouseBtn = event->getIf<sf::Event::MouseButtonPressed>()) {
        if (mouseBtn->button != sf::Mouse::Button::Left) return;
        sf::Vector2f mousePos = window.mapPixelToCoords({mouseBtn->position.x, mouseBtn->position.y});
        std::cout << "[UIManager] Mouse click at (" << mousePos.x << ", " << mousePos.y << ")\n";

        // Reactive prompt clicks (Yes/No)
        if (showReactivePrompt) {
            float centerX = window.getSize().x / 2.f;
            float centerY = window.getSize().y / 2.f;
            sf::FloatRect yesBounds({centerX - 90.f, centerY + 10.f}, {70.f, 30.f});
            sf::FloatRect noBounds({centerX + 20.f, centerY + 10.f}, {70.f, 30.f});

            if (yesBounds.contains(mousePos)) {
                showReactivePrompt = false;
                std::cout << "[UIManager] Reactive prompt: YES" << std::endl;
                if (onReactiveResponse) onReactiveResponse(true);
                return;
            }
            if (noBounds.contains(mousePos)) {
                showReactivePrompt = false;
                std::cout << "[UIManager] Reactive prompt: NO" << std::endl;
                if (onReactiveResponse) onReactiveResponse(false);
                return;
            }
            return;  // block other clicks while prompt is visible
        }

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

        // Pick-card overlay: target player or skill user picking a specific card
        if (showPickCardOverlay) {
            for (auto& cv : cardVisuals) {
                if (cv.isClicked(mousePos) &&
                    std::find(pickCardAllowedIds.begin(), pickCardAllowedIds.end(), cv.cardId) != pickCardAllowedIds.end()) {
                    PlayerTargeting target;
                    Card chosenCard(
                        gameState.getCardSuit(cv.ownerId, cv.cardIndex),
                        gameState.getCardRank(cv.ownerId, cv.cardIndex),
                        gameState.isCardFaceUp(cv.ownerId, cv.cardIndex)
                    );
                    chosenCard.setId(cv.cardId);
                    chosenCard.setOwnerId(cv.ownerId);
                    chosenCard.setHandIndex(cv.cardIndex);
                    target.targetCards.push_back(chosenCard);
                    pendingTargeting = target;
                    showPickCardOverlay = false;
                    pickCardAllowedIds = {};
                    std::cout << "[UIManager] Pick-card overlay: picked card " << cv.cardId << std::endl;
                    confirmTargeting();
                    return;
                }
            }
            return;
        }

        // NeuralGambit targeting clicks
        if (showTargetingOverlay_NeuralGambit) {
            if (ngStep == NgStep::PICK_PLAYER) {
                for (auto& playerVisual : playerVisuals) {
                    if (playerVisual.isClicked(mousePos)) {
                        if (playerVisual.isTarget) {
                            playerVisual.isTarget = false;
                            auto it = std::find(ngTargetPlayerIds.begin(), ngTargetPlayerIds.end(), playerVisual.playerId);
                            if (it != ngTargetPlayerIds.end()) ngTargetPlayerIds.erase(it);
                            return;
                        }
                        playerVisual.isTarget = true;
                        ngTargetPlayerIds.push_back(playerVisual.playerId);
                        if ((int)ngTargetPlayerIds.size() < 2) return;
                        // Two players selected: fire partial target (player IDs only) to Phase
                        std::sort(ngTargetPlayerIds.begin(), ngTargetPlayerIds.end());
                        PlayerTargeting partial;
                        partial.targetPlayerIds = ngTargetPlayerIds;
                        if (onTargetChosen) onTargetChosen(partial);
                        ngStep = NgStep::WAITING_FOR_PICKS;
                        std::cout << "[UIManager][NeuralGambit] Sent player selection, waiting for picks" << std::endl;
                        return;
                    }
                }
            }
            else if (ngStep == NgStep::WAITING_FOR_PICKS) {
                // No input needed — server is orchestrating target picks
            }
            else if (ngStep == NgStep::PICK_BOOST_CARD) {
                for (auto& cv : cardVisuals) {
                    if ((cv.cardId == ngTargetCardIds[0] || cv.cardId == ngTargetCardIds[1])
                            && cv.isClicked(mousePos)) {
                        // Send only the chosen boost card — server has the other two
                        PlayerTargeting target;
                        Card chosenCard(
                            gameState.getCardSuit(cv.ownerId, cv.cardIndex),
                            gameState.getCardRank(cv.ownerId, cv.cardIndex),
                            gameState.isCardFaceUp(cv.ownerId, cv.cardIndex)
                        );
                        chosenCard.setId(cv.cardId);
                        chosenCard.setOwnerId(cv.ownerId);
                        chosenCard.setHandIndex(cv.cardIndex);
                        target.targetCards.push_back(chosenCard);
                        pendingTargeting = target;
                        showTargetingOverlay_NeuralGambit = false;
                        ngTargetPlayerIds = {};
                        std::cout << "[UIManager][NeuralGambit] Boost card chosen: " << cv.cardId << std::endl;
                        confirmTargeting();
                        return;
                    }
                }
            }
            return; // don't fall through to Deliverance handling
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
                    chosenCard.setId(cv.cardId);
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
    renderPlayerVisuals();
    if (showActionMenu)                       renderActionMenu();
    if (showTargetingOverlay_Deliverance)     renderTargetingOverlay_Deliverance();
    if (showTargetingOverlay_NeuralGambit)    renderTargetingOverlay_NeuralGambit();
    if (showPickCardOverlay)                  renderPickCardOverlay();
    if (showReactivePrompt)                   renderReactivePrompt();
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

void UIManager::renderPlayerVisuals(){
    //update visuals with info from gameState
    auto players = gameState.getAllPlayerInfo();
    int currentTurnId = gameState.getCurrentPlayerId();
    for (int i=0;i<(int)players.size();i++){
        playerVisuals[i].playerId=players[i].playerId;
        playerVisuals[i].seatPostion = visualState.getPlayerSeatPos(i, (int)players.size());
        playerVisuals[i].skillName = players[i].skill;
        playerVisuals[i].skillUses = players[i].skillUses;
        playerVisuals[i].points = players[i].points;
        playerVisuals[i].isHost = players[i].isHost;
    }
    // Per-player icon + points + seat label
    for (auto& playerVisual : playerVisuals ) {
        bool isCurrentTurn = (playerVisual.playerId == currentTurnId);

        // Draw player icon
        sf::Vector2f iconScale = {0.18f, 0.18f};
        playerVisual.playerSprite.setPosition({
            playerVisual.seatPostion.x - 86.f,
            playerVisual.seatPostion.y - 26.f
        });
        playerVisual.playerSprite.setScale(iconScale);
        window.draw(playerVisual.playerSprite);

        // Host label above player icon
        if (playerVisual.isHost) {
            sf::Text hostLabel(font, "HOST", 18);
            hostLabel.setFillColor(sf::Color(255, 200, 50));
            hostLabel.setStyle(sf::Text::Bold);
            hostLabel.setPosition({ playerVisual.seatPostion.x - 86.f, playerVisual.seatPostion.y - 50.f });
            window.draw(hostLabel);
        }

        std::string label = "P" + std::to_string(playerVisual.playerId)
                          + "  " + std::to_string(playerVisual.points) + "pts"
                          + "  [" + gameState.skillNameToString(playerVisual.skillName)
                          + " x" + std::to_string(playerVisual.skillUses) + "]";

        sf::Text playerLabel(font, label, 11);
        // Highlight current turn player's name
        if (isCurrentTurn) {
            playerLabel.setFillColor(sf::Color(100, 255, 100));
            playerLabel.setStyle(sf::Text::Bold);
        } else {
            playerLabel.setFillColor(sf::Color::White);
        }
        //host gets another name label highlight during host hit phase
        if (gameState.getPhaseName() == PhaseName::HOST_HIT_PHASE && playerVisual.isHost) {
            playerLabel.setFillColor(sf::Color(255, 200, 20));
            playerLabel.setStyle(sf::Text::Bold);
        }

        playerLabel.setPosition({ playerVisual.seatPostion.x, playerVisual.seatPostion.y - 20.f });
        window.draw(playerLabel);

        // Turn indicator arrow
        if (isCurrentTurn) {
            sf::Text arrow(font, ">", 16);
            arrow.setFillColor(sf::Color(100, 255, 100));
            arrow.setStyle(sf::Text::Bold);
            arrow.setPosition({ playerVisual.seatPostion.x - 14.f, playerVisual.seatPostion.y - 22.f });
            window.draw(arrow);
        }
    }
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
void UIManager::renderTargetingOverlay_NeuralGambit() {
    auto players = gameState.getAllPlayerInfo();
    int totalPlayers = (int)players.size();

    // Bottom strip prompt
    sf::RectangleShape strip({ (float)window.getSize().x, 70.f });
    strip.setFillColor(UILayout::HUD_BG);
    strip.setPosition({ 0.f, (float)window.getSize().y - 70.f });
    window.draw(strip);

    // Step-specific prompt text and visuals
    if (ngStep == NgStep::PICK_PLAYER) {
        hoveredPlayerId = -1;
        sf::Text prompt(font, "NEURAL GAMBIT: Choose a target player", 16);

        prompt.setFillColor(sf::Color::Cyan);
        prompt.setPosition({ 10.f, (float)window.getSize().y - 65.f });
        window.draw(prompt);

        // Draw highlights for all players (include self)
        for (auto& playerVisual : playerVisuals) {
            auto playerBounds = playerVisual.playerSprite.getGlobalBounds();
            bool hovered = playerBounds.contains(mousePos);

            if (hovered) hoveredPlayerId = playerVisual.playerId;
            // Hover effect: scale up slightly
            if (playerVisual.isTarget){
                float elapsed = shakeClock.getElapsedTime().asSeconds();
                float shakeOffsetX =
                    std::sin(elapsed * 40.f) * 3.f +
                    std::sin(elapsed * 80.f) * 3.f;

                float shakeOffsetY =
                    std::cos(elapsed * 40.f) * 3.f +
                    std::sin(elapsed * 80.f) * 3.f;
                sf::Vector2f origPos = playerVisual.playerSprite.getPosition();
                playerVisual.playerSprite.setPosition({origPos.x + shakeOffsetX, origPos.y + shakeOffsetY});
                window.draw(playerVisual.playerSprite);
                playerVisual.playerSprite.setPosition(origPos);
            }
            else if (hovered && !playerVisual.isTarget) {
                sf::Vector2f origScale = playerVisual.playerSprite.getScale();
                float hoverScale = 1.15f;
                playerVisual.playerSprite.setScale(origScale * hoverScale);
                playerBounds = playerVisual.playerSprite.getGlobalBounds();
                window.draw(playerVisual.playerSprite);
                playerVisual.playerSprite.setScale(origScale);
            }

            sf::RectangleShape highlight({playerBounds.size.x, playerBounds.size.y});
            highlight.setPosition({playerBounds.position.x, playerBounds.position.y});
            highlight.setFillColor(sf::Color::Transparent);
            highlight.setOutlineThickness(hovered ? 5.f : 3.f);
            highlight.setOutlineColor(playerVisual.isTarget ? UILayout::CARD_TARGET :
                                      hovered ? sf::Color(255, 240, 100) : UILayout::CARD_HIGHLIGHT);
            window.draw(highlight);
        }
    }
    else if (ngStep == NgStep::WAITING_FOR_PICKS) {
        sf::Text prompt(font, "NEURAL GAMBIT: Waiting for players to reveal cards...", 16);
        prompt.setFillColor(sf::Color(150, 220, 255));
        prompt.setPosition({ 10.f, (float)window.getSize().y - 65.f });
        window.draw(prompt);

        // Highlight targeted players with shake; others get hover scale
        for (auto& playerVisual : playerVisuals) {
            bool isTarget = std::find(ngTargetPlayerIds.begin(), ngTargetPlayerIds.end(), playerVisual.playerId) != ngTargetPlayerIds.end();
            auto playerBounds = playerVisual.playerSprite.getGlobalBounds();
            bool hovered = playerBounds.contains(mousePos);

            if (isTarget) {
                float elapsed = shakeClock.getElapsedTime().asSeconds();
                float shakeOffsetX = std::sin(elapsed * 40.f) * 3.f + std::sin(elapsed * 80.f) * 3.f;
                float shakeOffsetY = std::cos(elapsed * 40.f) * 3.f + std::sin(elapsed * 80.f) * 3.f;
                sf::Vector2f origPos = playerVisual.playerSprite.getPosition();
                playerVisual.playerSprite.setPosition({origPos.x + shakeOffsetX, origPos.y + shakeOffsetY});
                window.draw(playerVisual.playerSprite);
                playerVisual.playerSprite.setPosition(origPos);
            } else if (hovered) {
                sf::Vector2f origScale = playerVisual.playerSprite.getScale();
                playerVisual.playerSprite.setScale(origScale * 1.15f);
                playerBounds = playerVisual.playerSprite.getGlobalBounds();
                window.draw(playerVisual.playerSprite);
                playerVisual.playerSprite.setScale(origScale);
            }

            sf::RectangleShape highlight({ playerBounds.size.x, playerBounds.size.y });
            highlight.setPosition({ playerBounds.position.x, playerBounds.position.y });
            highlight.setFillColor(sf::Color::Transparent);
            highlight.setOutlineThickness(isTarget ? 4.f : (hovered ? 3.f : 0.f));
            highlight.setOutlineColor(isTarget ? UILayout::CARD_TARGET : sf::Color(255, 240, 100));
            window.draw(highlight);
        }
    }
    else if (ngStep == NgStep::PICK_BOOST_CARD) {
        sf::Text prompt(font, "NEURAL GAMBIT: Choose which card gets boosted", 16);
        prompt.setFillColor(sf::Color::Cyan);
        prompt.setPosition({ 10.f, (float)window.getSize().y - 65.f });
        window.draw(prompt);

        // Highlight both revealed cards
        for (auto& cv : cardVisuals) {
            if (cv.cardId != ngTargetCardIds[0] && cv.cardId != ngTargetCardIds[1]) continue;
            auto bounds = cv.cardSprite.getGlobalBounds();
            bool hovered = bounds.contains(mousePos);
            sf::RectangleShape highlight({ bounds.size.x, bounds.size.y });
            highlight.setPosition({ bounds.position.x, bounds.position.y });
            highlight.setFillColor(sf::Color(255, 215, 0, hovered ? 60 : 30));
            highlight.setOutlineThickness(hovered ? 6.f : 3.f);
            highlight.setOutlineColor(sf::Color(255, 215, 0));
            window.draw(highlight);
        }
    }
}

// ============================================================================
// Layout Helpers
// ============================================================================

void UIManager::renderPickCardOverlay() {
    // Bottom strip
    sf::RectangleShape strip({ (float)window.getSize().x, 70.f });
    strip.setFillColor(UILayout::HUD_BG);
    strip.setPosition({ 0.f, (float)window.getSize().y - 70.f });
    window.draw(strip);

    sf::Text prompt(font, "NEURAL GAMBIT: Choose a card to reveal", 16);
    prompt.setFillColor(sf::Color::Cyan);
    prompt.setPosition({ 10.f, (float)window.getSize().y - 65.f });
    window.draw(prompt);

    

    // Shake the player whose cards are being picked
    if (!pickCardAllowedIds.empty()) {
        int ownerId = -1;
        for (auto& cv : cardVisuals) {
            if (std::find(pickCardAllowedIds.begin(), pickCardAllowedIds.end(), cv.cardId) != pickCardAllowedIds.end()) {
                ownerId = cv.ownerId;
                break;
            }
        }
        for (auto& pv : playerVisuals) {
            if (pv.playerId != ownerId) continue;
            float elapsed = shakeClock.getElapsedTime().asSeconds();
            float shakeOffsetX = std::sin(elapsed * 40.f) * 3.f + std::sin(elapsed * 80.f) * 3.f;
            float shakeOffsetY = std::cos(elapsed * 40.f) * 3.f + std::sin(elapsed * 80.f) * 3.f;
            sf::Vector2f origPos = pv.playerSprite.getPosition();
            pv.playerSprite.setPosition({origPos.x + shakeOffsetX, origPos.y + shakeOffsetY});
            window.draw(pv.playerSprite);
            pv.playerSprite.setPosition(origPos);
            break;
        }
    }

    // Highlight only allowed cards
    for (auto& cv : cardVisuals) {
        if (std::find(pickCardAllowedIds.begin(), pickCardAllowedIds.end(), cv.cardId) == pickCardAllowedIds.end()) continue;
        auto bounds = cv.cardSprite.getGlobalBounds();
        bool hovered = bounds.contains(mousePos);

        if (cv.isTarget) {
            float elapsed = shakeClock.getElapsedTime().asSeconds();
            float shakeOffsetX = std::sin(elapsed * 40.f) * 3.f + std::sin(elapsed * 80.f) * 3.f;
            float shakeOffsetY = std::cos(elapsed * 40.f) * 3.f + std::sin(elapsed * 80.f) * 3.f;
            sf::Vector2f origPos = cv.cardSprite.getPosition();
            cv.cardSprite.setPosition({origPos.x + shakeOffsetX, origPos.y + shakeOffsetY});
            window.draw(cv.cardSprite);
            cv.cardSprite.setPosition(origPos);
        }
        else if (hovered) {
            sf::Vector2f origScale = cv.cardSprite.getScale();
            cv.cardSprite.setScale(origScale * 1.15f);
            window.draw(cv.cardSprite);
            cv.cardSprite.setScale(origScale);
            bounds = cv.cardSprite.getGlobalBounds();
        }

        sf::RectangleShape highlight({ bounds.size.x, bounds.size.y });
        highlight.setPosition({ bounds.position.x, bounds.position.y });
        highlight.setFillColor(sf::Color::Transparent);
        highlight.setOutlineThickness(hovered ? 5.f : 3.f);
        highlight.setOutlineColor(cv.isTarget ? UILayout::CARD_TARGET :
                                  hovered ? sf::Color(255, 240, 100) : UILayout::CARD_HIGHLIGHT);
        window.draw(highlight);
    }
}

void UIManager::renderReactivePrompt() {
    float elapsed = reactivePromptClock.getElapsedTime().asSeconds();

    // Auto-decline on timeout
    if (elapsed >= reactivePromptDuration) {
        showReactivePrompt = false;
        std::cout << "[UIManager] Reactive prompt timed out" << std::endl;
        if (onReactiveResponse) onReactiveResponse(false);
        return;
    }

    float centerX = window.getSize().x / 2.f;
    float centerY = window.getSize().y / 2.f;
    float boxW = 200.f, boxH = 80.f;

    // Semi-transparent background box
    sf::RectangleShape bg({boxW, boxH});
    bg.setFillColor(sf::Color(20, 20, 40, 220));
    bg.setOutlineThickness(2.f);
    bg.setOutlineColor(sf::Color(255, 200, 50));
    bg.setPosition({centerX - boxW / 2.f, centerY - boxH / 2.f});
    window.draw(bg);

    // Skill name text
    sf::Text skillText(font, reactivePromptSkillName + "?", 14);
    skillText.setFillColor(sf::Color(255, 200, 50));
    sf::FloatRect stBounds = skillText.getLocalBounds();
    skillText.setPosition({centerX - stBounds.size.x / 2.f, centerY - boxH / 2.f + 6.f});
    window.draw(skillText);

    // Timer bar
    float barW = boxW - 20.f;
    float barH = 6.f;
    float progress = 1.f - (elapsed / reactivePromptDuration);
    sf::RectangleShape barBg({barW, barH});
    barBg.setFillColor(sf::Color(60, 60, 60));
    barBg.setPosition({centerX - barW / 2.f, centerY - boxH / 2.f + 28.f});
    window.draw(barBg);

    sf::RectangleShape barFill({barW * progress, barH});
    barFill.setFillColor(progress > 0.3f ? sf::Color(100, 255, 100) : sf::Color(255, 80, 80));
    barFill.setPosition({centerX - barW / 2.f, centerY - boxH / 2.f + 28.f});
    window.draw(barFill);

    // Yes button
    sf::RectangleShape yesBtn({70.f, 30.f});
    yesBtn.setFillColor(sf::Color(50, 150, 50));
    yesBtn.setOutlineThickness(1.f);
    yesBtn.setOutlineColor(sf::Color::White);
    yesBtn.setPosition({centerX - 90.f, centerY + 10.f});
    window.draw(yesBtn);

    sf::Text yesText(font, "Yes", 14);
    yesText.setFillColor(sf::Color::White);
    sf::FloatRect ytBounds = yesText.getLocalBounds();
    yesText.setPosition({centerX - 90.f + 35.f - ytBounds.size.x / 2.f, centerY + 14.f});
    window.draw(yesText);

    // No button
    sf::RectangleShape noBtn({70.f, 30.f});
    noBtn.setFillColor(sf::Color(150, 50, 50));
    noBtn.setOutlineThickness(1.f);
    noBtn.setOutlineColor(sf::Color::White);
    noBtn.setPosition({centerX + 20.f, centerY + 10.f});
    window.draw(noBtn);

    sf::Text noText(font, "No", 14);
    noText.setFillColor(sf::Color::White);
    sf::FloatRect ntBounds = noText.getLocalBounds();
    noText.setPosition({centerX + 20.f + 35.f - ntBounds.size.x / 2.f, centerY + 14.f});
    window.draw(noText);
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