#include "UIManager.h"
#include "Log.h"
#include <map>
#include <sstream>
// ============================================================================
// Button
// ============================================================================
Button::Button(sf::Font& font, const std::string& text, sf::Vector2f pos, sf::Vector2f size)
    : label(font, text, (unsigned int)(16 * UILayout::S()))
{
    float S = UILayout::S();
    shape.setPosition(pos);
    shape.setSize(size);
    shape.setFillColor(UILayout::BUTTON_NORMAL);
    shape.setOutlineThickness(1.f * S);
    shape.setOutlineColor(sf::Color::White);

    label = sf::Text(font, text, (unsigned int)(16 * S));
    // Center label in button
    sf::FloatRect bounds = label.getLocalBounds();
    label.setPosition({
        pos.x + size.x / 2.f - bounds.size.x / 2.f,
        pos.y + size.y / 2.f - bounds.size.y / 2.f - 4.f * S
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
    float S = UILayout::S();

    // Build action buttons (hidden by default)
    float btnY = window.getSize().y - 60.f * S;
    float btnStartX = window.getSize().x / 2.f - 190.f * S;

    actionButtons.emplace_back(font, "Hit",   sf::Vector2f{btnStartX,              btnY}, UILayout::BUTTON_SIZE());
    actionButtons.emplace_back(font, "Stand", sf::Vector2f{btnStartX + 130.f * S,  btnY}, UILayout::BUTTON_SIZE());
    actionButtons.emplace_back(font, "Skill", sf::Vector2f{btnStartX + 260.f * S,  btnY}, UILayout::BUTTON_SIZE());

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

    std::filesystem::path tableTexturePath = "assets/images/crazyJackBG.png";
    if (!tableTexture.loadFromFile(tableTexturePath)) {
        std::cerr << "[UIManager] Failed to load tableTexture from assets/images/crazyJackBG.png" << std::endl;
    }
    if (!cardRankPatch.loadFromFile("assets/images/cardBoosted.png")) {
        std::cerr << "[UIManager] Failed to load cardRankPatch from assets/images/cardBoosted.png" << std::endl;
    }
    cardRankPatchSprite = std::make_unique<sf::Sprite>(cardRankPatch);
    clearInput();

    std::filesystem::path playerIconPath = "assets/images/playerIcon.png";
    if (!playerIcon.loadFromFile(playerIconPath)) {
        std::cerr << "[UIManager] Failed to load playerIcon from assets/images/playerIcon.png" << std::endl;
    }
    playerIcon2.loadFromFile("assets/images/playerIcon2.png");
    // Player icon sprite definitions are stored — playerVisuals built dynamically
    // in ensurePlayerVisuals() based on actual player count from GameState.
}

void UIManager::ensurePlayerVisuals(int count) {
    if ((int)playerVisuals.size() == count) return;
    playerVisuals.clear();

    // 4 unique character crops — cycle for >4 players
    struct IconDef { sf::Texture* tex; sf::IntRect rect; };
    sf::Vector2i cropSize = { 195, 195 };
    IconDef icons[] = {
        { &playerIcon,  sf::IntRect({772, 194}, cropSize) },
        { &playerIcon,  sf::IntRect({62, 212},  cropSize) },
        { &playerIcon,  sf::IntRect({512, 224}, cropSize) },
        { &playerIcon2, sf::IntRect({817, 255}, {167, 167}) },
    };
    int numIcons = 4;

    for (int i = 0; i < count; i++) {
        auto& def = icons[i % numIcons];
        sf::Sprite sprite(*def.tex);
        sprite.setTextureRect(def.rect);
        PlayerVisual pv(sprite);
        pv.playerId = i;
        playerVisuals.push_back(pv);
    }
}

// ============================================================================
// Public Interface
// ============================================================================
void UIManager::requestActionInput(int playerId, float duration) {
    if(!showActionMenu) std::cout << "[UIManager] Request Action Input Menu" << std::endl;
    activePlayerId = playerId;
    showActionMenu = true;
    showTargetingOverlay_Deliverance = false;
    pendingTargeting = {};
    inputTimerClock.restart();
    inputTimerDuration = duration;
}

void UIManager::requestTargetInput(int playerId, float duration) {
    std::cout << "[UIManager] Request Target Input Menu" << std::endl;
    activePlayerId = playerId;
    showActionMenu = false;
    pendingTargeting = {};
    inputTimerClock.restart();
    inputTimerDuration = duration;
    // Choose overlay based on player's skill
    auto players = gameState.getAllPlayerInfo();
    for (auto& p : players) {
        if (p.playerId == playerId) {
            if (p.skill == SkillName::NEURALGAMBIT) {
                showTargetingOverlay_NeuralGambit = true;
                ngStep = NgStep::PICK_PLAYER;
                ngTargetPlayerIds={};
                ngTargetCardIds = {};
                for (auto& playerVisual : playerVisuals){
                    playerVisual.isTarget = false;
                }
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
    showChronoPrompt = false;
    showPlayerPickOverlay = false;
    playerPickAllowedIds = {};
    pickCardAllowedIds = {};
    // Note: showDeckPeek is NOT cleared — it's non-blocking and persists through phase transitions
    // Unpin any NG cards before clearing IDs (prevent pin leaks)
    for (int cid : ngTargetCardIds) {
        visualState.getCardVisual(cid).unpin();
    }
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
    inputTimerClock.restart();
    inputTimerDuration = GameConfig::TARGET_PROMPT_DURATION;
}

void UIManager::requestBoostPickInput(int card1Id, int card2Id) {
    std::cout << "[UIManager] requestBoostPickInput: cards " << card1Id << " and " << card2Id << std::endl;
    ngTargetCardIds = {card1Id, card2Id};
    ngStep = NgStep::PICK_BOOST_CARD;
    showTargetingOverlay_NeuralGambit = true;
    inputTimerClock.restart();
    inputTimerDuration = GameConfig::TARGET_PROMPT_DURATION;
    // Pin + flip so enforceVisibility() doesn't override during pick
    visualState.flipCardVisualFaceUp(card1Id);
    visualState.flipCardVisualFaceUp(card2Id);
    visualState.getCardVisual(card1Id).pin();
    visualState.getCardVisual(card2Id).pin();
}

void UIManager::requestReactivePrompt(const std::string& skillName, std::string& extraInfo, float timerDuration) {
    std::cout << "[UIManager] Reactive prompt: " << skillName << " (" << timerDuration << "s)" << std::endl;
    showReactivePrompt = true;
    reactivePromptSkillName = skillName;
    reactivePromptExtraInfo = extraInfo;
    reactivePromptDuration = timerDuration;
    reactivePromptClock.restart();
}

void UIManager::hideReactivePrompt() {
    showReactivePrompt = false;
}

void UIManager::requestChronoPrompt(bool hasSnapshot, float timerDuration) {
    std::cout << "[UIManager] Chrono prompt (hasSnapshot=" << hasSnapshot << ", " << timerDuration << "s)" << std::endl;
    showChronoPrompt = true;
    chronoHasSnapshot = hasSnapshot;
    chronoPromptDuration = timerDuration;
    chronoPromptClock.restart();
}

// ============================================================================
// Event Handling
// ============================================================================
void UIManager::handleEvent(const std::optional<sf::Event>& event) {
    if (!event.has_value()) return;

    // F3 toggles player tooltip, F4 cycles game log, F6 toggles debug tooltip
    if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
        if (keyPressed->scancode == sf::Keyboard::Scancode::F3) {
            showHoverTooltip = !showHoverTooltip;
            std::cout << "[UIManager] Player tooltip: " << (showHoverTooltip ? "ON" : "OFF") << std::endl;
        }
        if (keyPressed->scancode == sf::Keyboard::Scancode::F4) {
            gameLogMode = (gameLogMode + 1) % 3;
            const char* modes[] = {"OFF", "RECENT", "FULL"};
            std::cout << "[UIManager] Game log: " << modes[gameLogMode] << std::endl;
        }
        if (keyPressed->scancode == sf::Keyboard::Scancode::F6) {
            showDebugTooltip = !showDebugTooltip;
            std::cout << "[UIManager] Debug tooltip: " << (showDebugTooltip ? "ON" : "OFF") << std::endl;
        }
    }

    // Track mouse position for hover effects (map pixel → world coords for resized window)
    if (const auto* mouseMoved = event->getIf<sf::Event::MouseMoved>()) {
        mousePos = window.mapPixelToCoords({mouseMoved->position.x, mouseMoved->position.y});
    }

    // Double-click on player sprite toggles skill description
    if (const auto* mouseBtn = event->getIf<sf::Event::MouseButtonPressed>()) {
        if (mouseBtn->button == sf::Mouse::Button::Left) {
            sf::Vector2f clickPos = window.mapPixelToCoords({mouseBtn->position.x, mouseBtn->position.y});
            for (auto& pv : playerVisuals) {
                if (pv.playerSprite.getGlobalBounds().contains(clickPos)) {
                    if (lastClickedPlayerId == pv.playerId && doubleClickClock.getElapsedTime().asMilliseconds() < 400) {
                        skillDescPlayerId = (skillDescPlayerId == pv.playerId) ? -1 : pv.playerId;
                        lastClickedPlayerId = -1; // reset so triple-click doesn't re-toggle
                    } else {
                        lastClickedPlayerId = pv.playerId;
                        doubleClickClock.restart();
                    }
                    break;
                }
            }
        }
    }

    if (const auto* mouseBtn = event->getIf<sf::Event::MouseButtonPressed>()) {
        if (mouseBtn->button != sf::Mouse::Button::Left) return;
        sf::Vector2f mousePos = window.mapPixelToCoords({mouseBtn->position.x, mouseBtn->position.y});
        std::cout << "[UIManager] Mouse click at (" << mousePos.x << ", " << mousePos.y << ")\n";

        // Player pick overlay clicks (Destiny Deflect redirect target)
        if (showPlayerPickOverlay) {
            for (auto& playerVisual : playerVisuals) {
                if (playerVisual.isClicked(mousePos)) {
                    auto it = std::find(playerPickAllowedIds.begin(), playerPickAllowedIds.end(), playerVisual.playerId);
                    if (it != playerPickAllowedIds.end()) {
                        PlayerTargeting target;
                        target.targetPlayerIds.push_back(playerVisual.playerId);
                        pendingTargeting = target;
                        showPlayerPickOverlay = false;
                        playerPickAllowedIds = {};
                        std::cout << "[UIManager] Player pick: selected P" << playerVisual.playerId << std::endl;
                        confirmTargeting();
                        return;
                    }
                }
            }
            return; // block other clicks while player pick is showing
        }

        // Deck peek OK button click (non-blocking — check but don't block)
        if (showDeckPeek) {
            float S = UILayout::S();
            float panelW = 200.f * S, panelH = 190.f * S;
            float panelX = window.getSize().x - panelW - 10.f * S;
            float panelY = 10.f * S;
            float okW = 80.f * S, okH = 25.f * S;
            float okX = panelX + panelW / 2.f - okW / 2.f;
            float okY = panelY + panelH - 30.f * S + 4.f * S + 2.f * S;
            sf::FloatRect okBounds({okX, okY}, {okW, okH});
            if (okBounds.contains(mousePos)) {
                dismissDeckPeek();
                std::cout << "[UIManager] Deck peek dismissed via OK" << std::endl;
                return;
            }
            // Don't return here — deck peek is non-blocking, other clicks pass through
        }

        // Reactive prompt clicks (Yes/No)
        if (showReactivePrompt) {
            float S = UILayout::S();
            float centerX = window.getSize().x / 2.f;
            float centerY = window.getSize().y / 2.f;
            sf::FloatRect yesBounds({centerX - 90.f * S, centerY + 10.f * S}, {70.f * S, 30.f * S});
            sf::FloatRect noBounds({centerX + 20.f * S, centerY + 10.f * S}, {70.f * S, 30.f * S});

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

        // Chronosphere choice prompt clicks ([Snapshot]/[Rewind])
        if (showChronoPrompt) {
            float S = UILayout::S();
            float centerX = window.getSize().x / 2.f;
            float centerY = window.getSize().y / 2.f;
            sf::FloatRect snapshotBounds({centerX - 90.f * S, centerY + 10.f * S}, {70.f * S, 30.f * S});
            sf::FloatRect rewindBounds({centerX + 20.f * S, centerY + 10.f * S}, {70.f * S, 30.f * S});

            if (snapshotBounds.contains(mousePos)) {
                showChronoPrompt = false;
                std::cout << "[UIManager] Chrono prompt: SNAPSHOT" << std::endl;
                if (onChronoChoice) onChronoChoice(ChronoChoice::SNAPSHOT);
                return;
            }
            if (chronoHasSnapshot && rewindBounds.contains(mousePos)) {
                showChronoPrompt = false;
                std::cout << "[UIManager] Chrono prompt: REWIND" << std::endl;
                if (onChronoChoice) onChronoChoice(ChronoChoice::REWIND);
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
                        // Unpin revealed cards — enforceVisibility() takes over
                        for (int cid : ngTargetCardIds) {
                            visualState.getCardVisual(cid).unpin();
                        }
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
            float S = UILayout::S();
            sf::Vector2f confirmPos = {
                window.getSize().x - 150.f * S,
                window.getSize().y - 60.f * S
            };
            sf::FloatRect confirmBounds(confirmPos, UILayout::BUTTON_SIZE());
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
    renderTable();
    renderCards();
    renderHUD();
    renderPlayerVisuals();
    if (showActionMenu)                       renderActionMenu();
    if (showTargetingOverlay_Deliverance)     renderTargetingOverlay_Deliverance();
    if (showTargetingOverlay_NeuralGambit)    renderTargetingOverlay_NeuralGambit();
    if (showPickCardOverlay)                  renderPickCardOverlay();
    if (showPlayerPickOverlay)                renderPlayerPickOverlay();
    if (showReactivePrompt)                   renderReactivePrompt();
    if (showChronoPrompt)                     renderChronoPrompt();
    if (showDeckPeek)                         renderDeckPeekOverlay();
    if (showHoverTooltip)                     renderPlayerTooltip();
    if (showDebugTooltip)                     renderHoverTooltip();
    if (gameLogMode > 0)                      renderGameLog();
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
    auto info = gameState.getAllPlayerInfo();
    std::unordered_map<int, int> boostedMap;

    for (auto &p : info) {
        for (auto &c : p.cardsInHand) {
            if (c.getRankBonus()) {
                boostedMap[c.getId()] = c.getRankBonus();
            }
        }
    }
    for (auto& cv : cardVisuals) {
        window.draw(cv.cardSprite);
        if (boostedMap.empty()) continue;
        if (!cv.faceUp) continue;
        auto it = boostedMap.find(cv.cardId);
        if (it == boostedMap.end()) continue;
        cv.rankBonus = it->second;
        // Position patch sprite centered on the card without changing the card's origin
        auto cardBounds = cv.cardSprite.getGlobalBounds();
        float cardCenterX = cardBounds.position.x + cardBounds.size.x / 2.f;
        float cardCenterY = cardBounds.position.y + cardBounds.size.y / 2.f;

        auto patchLocal = cardRankPatchSprite->getLocalBounds();
        cardRankPatchSprite->setOrigin({patchLocal.size.x / 2.f, patchLocal.size.y / 2.f});
        cardRankPatchSprite->setPosition({cardCenterX, cardCenterY});
        cardRankPatchSprite->setScale(cv.cardSprite.getScale());
        window.draw(*cardRankPatchSprite);

        int cardRank = static_cast<int>(gameState.getCardRank(cv.ownerId, cv.cardIndex));
        sf::Text boostedRankText(font, std::to_string(cardRank + cv.rankBonus), 30);
        boostedRankText.setScale(cv.cardSprite.getScale());
        boostedRankText.setFillColor(sf::Color::White);
        auto textBounds = boostedRankText.getLocalBounds();
        boostedRankText.setOrigin({textBounds.size.x / 2.f, textBounds.size.y / 2.f});
        boostedRankText.setPosition({cardCenterX, cardCenterY});
        window.draw(boostedRankText);
        
    }
}

void UIManager::renderHUD() {
    float S = UILayout::S();
    // PhaseName label top-left
    sf::Text phaseText(font, gameState.phaseToString(gameState.getPhaseName()), (unsigned int)(16 * S));
    phaseText.setFillColor(getPhaseNameColor());
    phaseText.setPosition({10.f * S, 10.f * S});
    window.draw(phaseText);
}

void UIManager::renderActionMenu() {
    // Auto-stand on timeout
    if (inputTimerDuration > 0.f && inputTimerClock.getElapsedTime().asSeconds() >= inputTimerDuration) {
        showActionMenu = false;
        std::cout << "[UIManager] Action prompt timed out, auto-standing" << std::endl;
        if (onActionChosen) onActionChosen(PlayerAction::STAND);
        return;
    }

    float S = UILayout::S();
    // Dark strip at bottom
    sf::RectangleShape strip({ (float)window.getSize().x, 70.f * S });
    strip.setFillColor(UILayout::HUD_BG);
    strip.setPosition({ 0.f, (float)window.getSize().y - 70.f * S });
    window.draw(strip);

    sf::Text prompt(font, "Player " + std::to_string(activePlayerId) + " - choose action:", (unsigned int)(16 * S));
    prompt.setFillColor(sf::Color::Yellow);
    prompt.setPosition({ 10.f * S, (float)window.getSize().y - 65.f * S });
    window.draw(prompt);

    for (auto& btn : actionButtons) btn.draw(window);
    renderInputTimerBar();
}

void UIManager::renderPlayerVisuals(){
    float S = UILayout::S();
    //update visuals with info from gameState
    auto players = gameState.getAllPlayerInfo();
    if (players.empty()) return;
    ensurePlayerVisuals((int)players.size());
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

        // Check if this player visual is borrowed by AnimationManager
        bool isBorrowed = false;
        for (int borrowedId : borrowedPlayerVisualIds){
            if(playerVisual.playerId == borrowedId) isBorrowed = true;
        }

        // Draw player icon — skip position/scale/draw if borrowed (AnimationManager controls it)
        if (!isBorrowed) {
            float iconS = GameConfig::PLAYER_ICON_SCALE * S;
            sf::Vector2f iconScale = {iconS, iconS};
            playerVisual.playerSprite.setPosition({
                playerVisual.seatPostion.x - 56.f * S,
                playerVisual.seatPostion.y - 26.f * S
            });
            playerVisual.playerSprite.setScale(iconScale);
            window.draw(playerVisual.playerSprite);
        }

        // Host label above player icon
        if (playerVisual.isHost) {
            sf::Text hostLabel(font, "HOST", (unsigned int)(16 * S));
            hostLabel.setFillColor(sf::Color(255, 200, 50));
            hostLabel.setStyle(sf::Text::Bold);
            hostLabel.setPosition({ playerVisual.seatPostion.x - 56.f * S, playerVisual.seatPostion.y - 50.f * S });
            window.draw(hostLabel);
        }

        std::string label = "P" + std::to_string(playerVisual.playerId)
                          + "  " + std::to_string(playerVisual.points) + "pts"
                          + "  [" + gameState.skillNameToString(playerVisual.skillName)
                          + " x" + std::to_string(playerVisual.skillUses) + "]";

        sf::Text playerLabel(font, label, (unsigned int)(9 * S));
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

        playerLabel.setPosition({ playerVisual.seatPostion.x, playerVisual.seatPostion.y - 20.f * S });
        window.draw(playerLabel);

        // Turn indicator arrow
        if (isCurrentTurn) {
            sf::Text arrow(font, ">", (unsigned int)(16 * S));
            arrow.setFillColor(sf::Color(100, 255, 100));
            arrow.setStyle(sf::Text::Bold);
            arrow.setPosition({ playerVisual.seatPostion.x - 14.f * S, playerVisual.seatPostion.y - 22.f * S });
            window.draw(arrow);
        }
    }
}

void UIManager::renderTargetingOverlay_Deliverance() {
    float S = UILayout::S();
    // Auto-pick random card on timeout
    if (inputTimerDuration > 0.f && inputTimerClock.getElapsedTime().asSeconds() >= inputTimerDuration) {
        for (auto& cv : cardVisuals) {
            if (cv.ownerId == activePlayerId) {
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
                break;
            }
        }
        showTargetingOverlay_Deliverance = false;
        std::cout << "[UIManager] Deliverance targeting timed out, auto-picking" << std::endl;
        confirmTargeting();
        return;
    }

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
                    std::sin(elapsed * 40.f) * 3.f * S +
                    std::sin(elapsed * 80.f) * 3.f * S;

                float shakeOffsetY =
                    std::cos(elapsed * 40.f) * 3.f * S +
                    std::sin(elapsed * 80.f) * 3.f * S;
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
            highlight.setOutlineThickness(hovered ? 5.f * S : 3.f * S);
            highlight.setOutlineColor(cv.isTarget ? UILayout::CARD_TARGET :
                                      hovered ? sf::Color(255, 240, 100) : UILayout::CARD_HIGHLIGHT);
            window.draw(highlight);
        }
    }

    // Confirm button
    Button confirmBtn(font, "Confirm",
        { window.getSize().x - 150.f * S, (float)window.getSize().y - 60.f * S },
        UILayout::BUTTON_SIZE());
    confirmBtn.shape.setFillColor(sf::Color(50, 150, 50));
    confirmBtn.draw(window);

    sf::Text prompt(font, "Click a card, then Confirm", (unsigned int)(16 * S));
    prompt.setFillColor(sf::Color::Yellow);
    prompt.setPosition({ 10.f * S, (float)window.getSize().y - 65.f * S });
    window.draw(prompt);
    renderInputTimerBar();
}
void UIManager::renderTargetingOverlay_NeuralGambit() {
    float S = UILayout::S();
    auto players = gameState.getAllPlayerInfo();
    int totalPlayers = (int)players.size();

    // Auto-pick on timeout (PICK_PLAYER and PICK_BOOST_CARD steps only)
    if (inputTimerDuration > 0.f && inputTimerClock.getElapsedTime().asSeconds() >= inputTimerDuration) {
        if (ngStep == NgStep::PICK_PLAYER) {
            // Auto-pick 2 random distinct players
            ngTargetPlayerIds.clear();
            std::vector<int> candidates;
            for (auto& p : players) candidates.push_back(p.playerId);
            while ((int)ngTargetPlayerIds.size() < 2 && !candidates.empty()) {
                int idx = rand() % (int)candidates.size();
                ngTargetPlayerIds.push_back(candidates[idx]);
                candidates.erase(candidates.begin() + idx);
            }
            std::sort(ngTargetPlayerIds.begin(), ngTargetPlayerIds.end());
            PlayerTargeting partial;
            partial.targetPlayerIds = ngTargetPlayerIds;
            if (onTargetChosen) onTargetChosen(partial);
            ngStep = NgStep::WAITING_FOR_PICKS;
            std::cout << "[UIManager] NeuralGambit PICK_PLAYER timed out, auto-picking" << std::endl;
            return;
        }
        else if (ngStep == NgStep::PICK_BOOST_CARD) {
            // Auto-pick one of the two revealed cards
            int chosenId = ngTargetCardIds[rand() % 2];
            for (auto& cv : cardVisuals) {
                if (cv.cardId == chosenId) {
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
                    break;
                }
            }
            for (int cid : ngTargetCardIds) visualState.getCardVisual(cid).unpin();
            showTargetingOverlay_NeuralGambit = false;
            ngTargetPlayerIds = {};
            std::cout << "[UIManager] NeuralGambit PICK_BOOST timed out, auto-picking" << std::endl;
            confirmTargeting();
            return;
        }
    }

    // Bottom strip prompt
    sf::RectangleShape strip({ (float)window.getSize().x, 70.f * S });
    strip.setFillColor(UILayout::HUD_BG);
    strip.setPosition({ 0.f, (float)window.getSize().y - 70.f * S });
    window.draw(strip);

    // Step-specific prompt text and visuals
    if (ngStep == NgStep::PICK_PLAYER) {
        hoveredPlayerId = -1;
        sf::Text prompt(font, "NEURAL GAMBIT: Choose a target player", (unsigned int)(16 * S));

        prompt.setFillColor(sf::Color::Cyan);
        prompt.setPosition({ 10.f * S, (float)window.getSize().y - 65.f * S });
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
                    std::sin(elapsed * 40.f) * 3.f * S +
                    std::sin(elapsed * 80.f) * 3.f * S;

                float shakeOffsetY =
                    std::cos(elapsed * 40.f) * 3.f * S +
                    std::sin(elapsed * 80.f) * 3.f * S;
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
            highlight.setOutlineThickness(hovered ? 5.f * S : 3.f * S);
            highlight.setOutlineColor(playerVisual.isTarget ? UILayout::CARD_TARGET :
                                      hovered ? sf::Color(255, 240, 100) : UILayout::CARD_HIGHLIGHT);
            window.draw(highlight);
        }
    }
    else if (ngStep == NgStep::WAITING_FOR_PICKS) {
        sf::Text prompt(font, "NEURAL GAMBIT: Waiting for players to reveal cards...", (unsigned int)(16 * S));
        prompt.setFillColor(sf::Color(150, 220, 255));
        prompt.setPosition({ 10.f * S, (float)window.getSize().y - 65.f * S });
        window.draw(prompt);

        // Highlight targeted players with shake; others get hover scale
        for (auto& playerVisual : playerVisuals) {
            bool isTarget = std::find(ngTargetPlayerIds.begin(), ngTargetPlayerIds.end(), playerVisual.playerId) != ngTargetPlayerIds.end();
            auto playerBounds = playerVisual.playerSprite.getGlobalBounds();
            bool hovered = playerBounds.contains(mousePos);

            if (isTarget) {
                float elapsed = shakeClock.getElapsedTime().asSeconds();
                float shakeOffsetX = std::sin(elapsed * 40.f) * 3.f * S + std::sin(elapsed * 80.f) * 3.f * S;
                float shakeOffsetY = std::cos(elapsed * 40.f) * 3.f * S + std::sin(elapsed * 80.f) * 3.f * S;
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
            highlight.setOutlineThickness(isTarget ? 4.f * S : (hovered ? 3.f * S : 0.f));
            highlight.setOutlineColor(isTarget ? UILayout::CARD_TARGET : sf::Color(255, 240, 100));
            window.draw(highlight);
        }
    }
    else if (ngStep == NgStep::PICK_BOOST_CARD) {
        sf::Text prompt(font, "NEURAL GAMBIT: Choose which card gets boosted", (unsigned int)(16 * S));
        prompt.setFillColor(sf::Color::Cyan);
        prompt.setPosition({ 10.f * S, (float)window.getSize().y - 65.f * S });
        window.draw(prompt);


        // Highlight both revealed cards
        for (auto& cv : cardVisuals) {

            if (cv.cardId != ngTargetCardIds[0] && cv.cardId != ngTargetCardIds[1]) continue;

            auto cardBounds = cv.cardSprite.getGlobalBounds();
            bool hovered = cardBounds.contains(mousePos);

            if (hovered) {
                sf::Vector2f origScale = cv.cardSprite.getScale();
                cv.cardSprite.setScale(origScale * 1.15f);
                cardBounds = cv.cardSprite.getGlobalBounds();
                window.draw(cv.cardSprite);
                cv.cardSprite.setScale(origScale);
            }
            auto bounds = cv.cardSprite.getGlobalBounds();
            hovered = bounds.contains(mousePos);
            sf::RectangleShape highlight({ bounds.size.x, bounds.size.y });
            highlight.setPosition({ bounds.position.x, bounds.position.y });
            highlight.setFillColor(sf::Color(255, 215, 0, hovered ? 60 : 30));
            highlight.setOutlineThickness(hovered ? 6.f * S : 3.f * S);
            highlight.setOutlineColor(sf::Color(255, 215, 0));
            window.draw(highlight);
        }
    }
    // Timer bar (skip during WAITING_FOR_PICKS — server-driven, no local timeout)
    if (ngStep != NgStep::WAITING_FOR_PICKS) renderInputTimerBar();
}

// ============================================================================
// Layout Helpers
// ============================================================================

void UIManager::renderPickCardOverlay() {
    float S = UILayout::S();
    // Auto-pick random allowed card on timeout
    if (inputTimerDuration > 0.f && inputTimerClock.getElapsedTime().asSeconds() >= inputTimerDuration
        && !pickCardAllowedIds.empty()) {
        int randomId = pickCardAllowedIds[rand() % (int)pickCardAllowedIds.size()];
        for (auto& cv : cardVisuals) {
            if (cv.cardId == randomId) {
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
                break;
            }
        }
        showPickCardOverlay = false;
        pickCardAllowedIds = {};
        std::cout << "[UIManager] Pick-card timed out, auto-picking" << std::endl;
        confirmTargeting();
        return;
    }

    // Bottom strip
    sf::RectangleShape strip({ (float)window.getSize().x, 70.f * S });
    strip.setFillColor(UILayout::HUD_BG);
    strip.setPosition({ 0.f, (float)window.getSize().y - 70.f * S });
    window.draw(strip);

    sf::Text prompt(font, "NEURAL GAMBIT: Choose a card to reveal", (unsigned int)(16 * S));
    prompt.setFillColor(sf::Color::Cyan);
    prompt.setPosition({ 10.f * S, (float)window.getSize().y - 65.f * S });
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
            float shakeOffsetX = std::sin(elapsed * 40.f) * 3.f * S + std::sin(elapsed * 80.f) * 3.f * S;
            float shakeOffsetY = std::cos(elapsed * 40.f) * 3.f * S + std::sin(elapsed * 80.f) * 3.f * S;
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
            float shakeOffsetX = std::sin(elapsed * 40.f) * 3.f * S + std::sin(elapsed * 80.f) * 3.f * S;
            float shakeOffsetY = std::cos(elapsed * 40.f) * 3.f * S + std::sin(elapsed * 80.f) * 3.f * S;
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
        highlight.setOutlineThickness(hovered ? 5.f * S : 3.f * S);
        highlight.setOutlineColor(cv.isTarget ? UILayout::CARD_TARGET :
                                  hovered ? sf::Color(255, 240, 100) : UILayout::CARD_HIGHLIGHT);
        window.draw(highlight);
    }
    renderInputTimerBar();
}

void UIManager::renderInputTimerBar() {
    if (inputTimerDuration <= 0.f) return;
    float S = UILayout::S();
    float elapsed = inputTimerClock.getElapsedTime().asSeconds();
    float progress = std::max(0.f, 1.f - (elapsed / inputTimerDuration));

    float barW = 300.f * S;
    float barH = 6.f * S;
    float centerX = window.getSize().x / 2.f;
    float barY = (float)window.getSize().y - 80.f * S;

    sf::RectangleShape barBg({barW, barH});
    barBg.setFillColor(sf::Color(60, 60, 60));
    barBg.setPosition({centerX - barW / 2.f, barY});
    window.draw(barBg);

    sf::RectangleShape barFill({barW * progress, barH});
    barFill.setFillColor(progress > 0.3f ? sf::Color(100, 255, 100) : sf::Color(255, 80, 80));
    barFill.setPosition({centerX - barW / 2.f, barY});
    window.draw(barFill);
}

void UIManager::renderReactivePrompt() {
    float S = UILayout::S();
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
    float boxW = window.getSize().x * 0.65f, boxH = window.getSize().y * 0.65f;
    float boxTop = centerY - boxH / 2.f;

    // Semi-transparent background box
    sf::RectangleShape bg({boxW, boxH});
    bg.setFillColor(sf::Color(20, 20, 40, 220));
    bg.setOutlineThickness(2.f * S);
    bg.setOutlineColor(sf::Color(255, 200, 50));
    bg.setPosition({centerX - boxW / 2.f, boxTop});
    window.draw(bg);

    // Skill name text
    sf::Text skillText(font, reactivePromptSkillName, (unsigned int)(14 * S));
    skillText.setFillColor(sf::Color(255, 200, 50));
    sf::FloatRect stBounds = skillText.getLocalBounds();
    float skillTextY = boxTop + boxH * 0.25f;
    skillText.setPosition({centerX - stBounds.size.x / 2.f, skillTextY});
    window.draw(skillText);

    // Extra info text
    sf::Text extraInfo(font, reactivePromptExtraInfo, (unsigned int)(15 * S));
    extraInfo.setFillColor(sf::Color(255, 200, 50));
    sf::FloatRect eiBounds = extraInfo.getLocalBounds();
    float extraInfoY = skillTextY + stBounds.size.y + 10.f * S;
    extraInfo.setPosition({centerX - eiBounds.size.x / 2.f, extraInfoY});
    window.draw(extraInfo);

    // Timer bar
    float barW = boxW - 20.f * S;
    float barH = 6.f * S;
    float progress = 1.f - (elapsed / reactivePromptDuration);
    float barY = extraInfoY + eiBounds.size.y + 12.f * S;
    sf::RectangleShape barBg({barW, barH});
    barBg.setFillColor(sf::Color(60, 60, 60));
    barBg.setPosition({centerX - barW / 2.f, barY});
    window.draw(barBg);

    sf::RectangleShape barFill({barW * progress, barH});
    barFill.setFillColor(progress > 0.3f ? sf::Color(100, 255, 100) : sf::Color(255, 80, 80));
    barFill.setPosition({centerX - barW / 2.f, barY});
    window.draw(barFill);

    // Yes/No buttons — positions must match click handler hitboxes
    float btnY = centerY + 10.f * S;

    sf::RectangleShape yesBtn({70.f * S, 30.f * S});
    yesBtn.setFillColor(sf::Color(50, 150, 50));
    yesBtn.setOutlineThickness(1.f * S);
    yesBtn.setOutlineColor(sf::Color::White);
    yesBtn.setPosition({centerX - 90.f * S, btnY});
    window.draw(yesBtn);

    sf::Text yesText(font, "Let's roll", (unsigned int)(11 * S));
    yesText.setFillColor(sf::Color::White);
    sf::FloatRect ytBounds = yesText.getLocalBounds();
    yesText.setPosition({centerX - 90.f * S + 35.f * S - ytBounds.size.x / 2.f, btnY + 4.f * S});
    window.draw(yesText);

    sf::RectangleShape noBtn({70.f * S, 30.f * S});
    noBtn.setFillColor(sf::Color(150, 50, 50));
    noBtn.setOutlineThickness(1.f * S);
    noBtn.setOutlineColor(sf::Color::White);
    noBtn.setPosition({centerX + 20.f * S, btnY});
    window.draw(noBtn);

    sf::Text noText(font, "Nah i'm good", (unsigned int)(11 * S));
    noText.setFillColor(sf::Color::White);
    sf::FloatRect ntBounds = noText.getLocalBounds();
    noText.setPosition({centerX + 20.f * S + 35.f * S - ntBounds.size.x / 2.f, btnY + 4.f * S});
    window.draw(noText);
}

// ============================================================================
// Chronosphere Choice Prompt
// ============================================================================
void UIManager::renderChronoPrompt() {
    float S = UILayout::S();
    float elapsed = chronoPromptClock.getElapsedTime().asSeconds();

    // Auto-decline on timeout
    if (elapsed >= chronoPromptDuration) {
        showChronoPrompt = false;
        std::cout << "[UIManager] Chrono prompt timed out" << std::endl;
        // Timeout = no choice (decline)
        if (onChronoChoice) onChronoChoice(ChronoChoice::SNAPSHOT); // default to snapshot
        return;
    }

    float centerX = window.getSize().x / 2.f;
    float centerY = window.getSize().y / 2.f;
    float boxW = window.getSize().x * 0.65f, boxH = window.getSize().y * 0.65f;
    float boxTop = centerY - boxH / 2.f;

    // Semi-transparent background box
    sf::RectangleShape bg({boxW, boxH});
    bg.setFillColor(sf::Color(20, 20, 40, 220));
    bg.setOutlineThickness(2.f * S);
    bg.setOutlineColor(sf::Color(100, 200, 255));
    bg.setPosition({centerX - boxW / 2.f, boxTop});
    window.draw(bg);

    // Title text
    sf::Text titleText(font, "CHRONOSPHERE", (unsigned int)(14 * S));
    titleText.setFillColor(sf::Color(100, 200, 255));
    sf::FloatRect ttBounds = titleText.getLocalBounds();
    float titleY = boxTop + boxH * 0.25f;
    titleText.setPosition({centerX - ttBounds.size.x / 2.f, titleY});
    window.draw(titleText);

    // Info text
    std::string info = chronoHasSnapshot ? "Choose your action:" : "Snapshot your hand?";
    sf::Text infoText(font, info, (unsigned int)(10 * S));
    infoText.setFillColor(sf::Color(100, 200, 255));
    sf::FloatRect itBounds = infoText.getLocalBounds();
    float infoY = titleY + ttBounds.size.y + 10.f * S;
    infoText.setPosition({centerX - itBounds.size.x / 2.f, infoY});
    window.draw(infoText);

    // Timer bar
    float barW = boxW - 20.f * S;
    float barH = 6.f * S;
    float progress = 1.f - (elapsed / chronoPromptDuration);
    float barY = infoY + itBounds.size.y + 12.f * S;
    sf::RectangleShape barBg({barW, barH});
    barBg.setFillColor(sf::Color(60, 60, 60));
    barBg.setPosition({centerX - barW / 2.f, barY});
    window.draw(barBg);

    sf::RectangleShape barFill({barW * progress, barH});
    barFill.setFillColor(progress > 0.3f ? sf::Color(100, 200, 255) : sf::Color(255, 80, 80));
    barFill.setPosition({centerX - barW / 2.f, barY});
    window.draw(barFill);

    // Snapshot button (always available)
    float btnY = centerY + 10.f * S;
    sf::RectangleShape snapBtn({70.f * S, 30.f * S});
    snapBtn.setFillColor(sf::Color(50, 120, 180));
    snapBtn.setOutlineThickness(1.f * S);
    snapBtn.setOutlineColor(sf::Color::White);
    snapBtn.setPosition({centerX - 90.f * S, btnY});
    window.draw(snapBtn);

    sf::Text snapText(font, "Snapshot", (unsigned int)(11 * S));
    snapText.setFillColor(sf::Color::White);
    sf::FloatRect stBounds = snapText.getLocalBounds();
    snapText.setPosition({centerX - 90.f * S + 35.f * S - stBounds.size.x / 2.f, btnY + 6.f * S});
    window.draw(snapText);

    // Rewind button (grayed out if no snapshot)
    sf::RectangleShape rewBtn({70.f * S, 30.f * S});
    rewBtn.setFillColor(chronoHasSnapshot ? sf::Color(180, 120, 50) : sf::Color(80, 80, 80));
    rewBtn.setOutlineThickness(1.f * S);
    rewBtn.setOutlineColor(chronoHasSnapshot ? sf::Color::White : sf::Color(120, 120, 120));
    rewBtn.setPosition({centerX + 20.f * S, btnY});
    window.draw(rewBtn);

    sf::Text rewText(font, "Rewind", (unsigned int)(11 * S));
    rewText.setFillColor(chronoHasSnapshot ? sf::Color::White : sf::Color(120, 120, 120));
    sf::FloatRect rtBounds = rewText.getLocalBounds();
    rewText.setPosition({centerX + 20.f * S + 35.f * S - rtBounds.size.x / 2.f, btnY + 6.f * S});
    window.draw(rewText);
}

// ============================================================================
// Player-facing Hover Tooltip
// ============================================================================
static void drawTooltipBox(sf::RenderWindow& window, sf::Font& font,
                           const std::vector<std::string>& lines,
                           sf::Vector2f anchor, sf::Color bgColor = sf::Color(10, 10, 10, 210)) {
    if (lines.empty()) return;
    float S = UILayout::S();
    const unsigned int fontSize = (unsigned int)(10 * S);
    const float padding = 4.f * S;
    const float lineHeight = fontSize + 2.f * S;
    float maxWidth = 0.f;
    for (auto& line : lines) {
        sf::Text measure(font, line, fontSize);
        float w = measure.getLocalBounds().size.x;
        if (w > maxWidth) maxWidth = w;
    }
    float tooltipW = maxWidth + padding * 2.f;
    float tooltipH = lineHeight * lines.size() + padding * 2.f;
    float tx = anchor.x + 12.f * S;
    float ty = anchor.y + 12.f * S;
    float winW = static_cast<float>(window.getSize().x);
    float winH = static_cast<float>(window.getSize().y);
    if (tx + tooltipW > winW) tx = anchor.x - tooltipW - 4.f * S;
    if (ty + tooltipH > winH) ty = anchor.y - tooltipH - 4.f * S;
    if (tx < 0.f) tx = 0.f;
    if (ty < 0.f) ty = 0.f;

    sf::RectangleShape bg({tooltipW, tooltipH});
    bg.setFillColor(bgColor);
    bg.setOutlineThickness(1.f * S);
    bg.setOutlineColor(sf::Color(180, 180, 180, 200));
    bg.setPosition({tx, ty});
    window.draw(bg);
    for (int i = 0; i < (int)lines.size(); i++) {
        sf::Text text(font, lines[i], fontSize);
        text.setFillColor(sf::Color(220, 220, 220));
        text.setPosition({tx + padding, ty + padding + i * lineHeight});
        window.draw(text);
    }
}

void UIManager::renderPlayerTooltip() {
    int localId = visualState.getLocalPlayerId();
    std::vector<std::string> lines;

    // Card tooltip: only for cards that are faceUp or belong to the local player
    for (auto& cv : cardVisuals) {
        if (cv.location != CardLocation::HAND) continue;
        if (!cv.cardSprite.getGlobalBounds().contains(mousePos)) continue;

        bool canSee = cv.faceUp || cv.ownerId == localId;
        if (!canSee) break;

        if (cv.ownerId >= 0 && cv.cardIndex >= 0) {
            PlayerInfo info = gameState.getPlayerInfo(cv.ownerId);
            if (cv.cardIndex < (int)info.cardsInHand.size()) {
                const Card& c = info.cardsInHand[cv.cardIndex];
                lines.push_back("#" + std::to_string(cv.cardId));
                lines.push_back(c.getRankAsString() + " of " + c.getSuitAsString());
                if (c.getRankBonus() != 0)
                    lines.push_back("Rank bonus: " + std::to_string(c.getRankBonus()));
            }
        }
        break;
    }

    // Player tooltip: show handValue only if all their cards are logically faceUp
    // (or it's the local player — they always know their own hand)
    if (lines.empty()) {
        for (auto& pv : playerVisuals) {
            if (!pv.playerSprite.getGlobalBounds().contains(mousePos)) continue;

            PlayerInfo info = gameState.getPlayerInfo(pv.playerId);
            lines.push_back(gameState.skillNameToString(info.skill));
            lines.push_back("Uses: " + std::to_string(info.skillUses));
            lines.push_back("Points: " + std::to_string(info.points));
            if (pv.playerId == localId) {
                lines.push_back("Hand: " + std::to_string(info.handValue));
            } else {
                bool allFaceUp = true;
                for (auto& card : info.cardsInHand) {
                    if (!card.isFaceUp()) { allFaceUp = false; break; }
                }
                if (allFaceUp && !info.cardsInHand.empty())
                    lines.push_back("Hand: " + std::to_string(info.handValue));
            }

            // Skill description toggled by double-click — split on \n
            if (skillDescPlayerId == pv.playerId) {
                std::string desc = gameState.skillDescriptionToString(info.skill);
                if (!desc.empty()) {
                    std::istringstream stream(desc);
                    std::string line;
                    while (std::getline(stream, line)) lines.push_back(line);
                }
            }
            break;
        }
    }

    if (!lines.empty()) drawTooltipBox(window, font, lines, mousePos);
}

// ============================================================================
// Debug Hover Tooltip
// ============================================================================
static std::string cardLocationStr(CardLocation loc) {
    switch (loc) {
        case CardLocation::HAND:         return "HAND";
        case CardLocation::DECK:         return "DECK";
        case CardLocation::DISCARD_PILE: return "DISCARD";
        default:                         return "?";
    }
}

void UIManager::renderHoverTooltip() {
    float S = UILayout::S();
    std::vector<std::string> lines;

    // Cards first (overlap player areas, higher priority)
    for (auto& cv : cardVisuals) {
        if (cv.location != CardLocation::HAND) continue;
        if (!cv.cardSprite.getGlobalBounds().contains(mousePos)) continue;

        lines.push_back("cardId: " + std::to_string(cv.cardId));
        lines.push_back("ownerId: " + std::to_string(cv.ownerId));
        lines.push_back("cardIndex: " + std::to_string(cv.cardIndex));
        lines.push_back("faceUp: " + std::string(cv.faceUp ? "true" : "false"));
        lines.push_back("pinCount: " + std::to_string(cv.pinCount));
        lines.push_back("location: " + cardLocationStr(cv.location));

        // CardVisual's own rankBonus
        lines.push_back("cv.rankBonus: " + std::to_string(cv.rankBonus));

        // Game-logic card info from GameState
        if (cv.ownerId >= 0 && cv.cardIndex >= 0) {
            PlayerInfo info = gameState.getPlayerInfo(cv.ownerId);
            if (cv.cardIndex < (int)info.cardsInHand.size()) {
                const Card& c = info.cardsInHand[cv.cardIndex];
                lines.push_back("gs.cardId: " + std::to_string(c.getId()));
                lines.push_back("rank: " + c.getRankAsString());
                lines.push_back("suit: " + c.getSuitAsString());
                lines.push_back("gs.rankBonus: " + std::to_string(c.getRankBonus()));
                lines.push_back("logic faceUp: " + std::string(c.isFaceUp() ? "true" : "false"));
            }
        }
        if (cv.highlighted) lines.push_back("highlighted: true");
        if (cv.isTarget)    lines.push_back("isTarget: true");
        break;
    }

    // Player areas
    if (lines.empty()) {
        for (auto& pv : playerVisuals) {
            if (!pv.playerSprite.getGlobalBounds().contains(mousePos)) continue;

            PlayerInfo info = gameState.getPlayerInfo(pv.playerId);
            lines.push_back("playerId: " + std::to_string(info.playerId));
            lines.push_back("handValue: " + std::to_string(info.handValue));
            lines.push_back("cards: " + std::to_string(info.cardsInHand.size()));
            lines.push_back("skill: " + gameState.skillNameToString(info.skill));
            lines.push_back("skillUses: " + std::to_string(info.skillUses));
            lines.push_back("points: " + std::to_string(info.points));
            if (info.isBot)    lines.push_back("isBot: true");
            if (info.isHost)   lines.push_back("isHost: true");
            if (info.isRemote) lines.push_back("isRemote: true");
            break;
        }
    }

    if (lines.empty()) return;

    // Measure and render
    const unsigned int fontSize = (unsigned int)(10 * S);
    const float padding = 4.f * S;
    const float lineHeight = fontSize + 2.f * S;
    float maxWidth = 0.f;

    for (auto& line : lines) {
        sf::Text measure(font, line, fontSize);
        float w = measure.getLocalBounds().size.x;
        if (w > maxWidth) maxWidth = w;
    }

    float tooltipW = maxWidth + padding * 2.f;
    float tooltipH = lineHeight * lines.size() + padding * 2.f;

    // Position near cursor, clamp to window
    float tx = mousePos.x + 12.f * S;
    float ty = mousePos.y + 12.f * S;
    float winW = static_cast<float>(window.getSize().x);
    float winH = static_cast<float>(window.getSize().y);

    if (tx + tooltipW > winW) tx = mousePos.x - tooltipW - 4.f * S;
    if (ty + tooltipH > winH) ty = mousePos.y - tooltipH - 4.f * S;
    if (tx < 0.f) tx = 0.f;
    if (ty < 0.f) ty = 0.f;

    // Background
    sf::RectangleShape bg({tooltipW, tooltipH});
    bg.setFillColor(sf::Color(10, 10, 10, 210));
    bg.setOutlineThickness(1.f * S);
    bg.setOutlineColor(sf::Color(180, 180, 180, 200));
    bg.setPosition({tx, ty});
    window.draw(bg);

    // Text lines
    for (int i = 0; i < (int)lines.size(); i++) {
        sf::Text text(font, lines[i], fontSize);
        text.setFillColor(sf::Color(220, 220, 220));
        text.setPosition({tx + padding, ty + padding + i * lineHeight});
        window.draw(text);
    }
}

// ============================================================================
// In-Game Log Overlay (Minecraft-style)
// ============================================================================
void UIManager::renderGameLog() {
    float S = UILayout::S();
    const auto& entries = TimestampBuf::getEntries();
    if (entries.empty()) return;

    const unsigned int fontSize = (unsigned int)(9 * S);
    const float lineHeight = fontSize + 3.f * S;
    const float padding = 3.f * S;
    const float marginBottom = 8.f * S;
    const float marginLeft = 4.f * S;
    const float fadeDuration = 2.f;   // seconds to fade out
    const float visibleDuration = 6.f; // seconds before fading starts
    const int maxRecentLines = 15;
    const int maxFullLines = 40;

    float now = TimestampBuf::currentTime();
    float winH = static_cast<float>(window.getSize().y);
    float winW = static_cast<float>(window.getSize().x);

    // Collect visible lines
    struct VisibleLine { std::string text; float alpha; };
    std::vector<VisibleLine> visible;

    bool fullMode = (gameLogMode == 2);
    int maxLines = fullMode ? maxFullLines : maxRecentLines;

    // Walk backwards from most recent
    int startIdx = std::max(0, (int)entries.size() - maxLines);
    for (int i = startIdx; i < (int)entries.size(); i++) {
        const LogEntry& e = entries[i];
        float age = now - e.timestamp;

        float alpha = 1.f;
        if (!fullMode) {
            if (age > visibleDuration + fadeDuration) continue; // fully faded
            if (age > visibleDuration) {
                alpha = 1.f - (age - visibleDuration) / fadeDuration;
            }
        }
        visible.push_back({e.text, alpha});
    }

    if (visible.empty()) return;

    // Render from bottom up
    float bottomY = winH - marginBottom;

    // Truncate long lines to fit screen width
    float maxTextWidth = winW * 0.65f;

    for (int i = (int)visible.size() - 1; i >= 0; i--) {
        float y = bottomY - ((int)visible.size() - i) * lineHeight;
        if (y < 0.f) break;  // off-screen

        uint8_t a = static_cast<uint8_t>(visible[i].alpha * 220.f);

        // Background strip
        sf::RectangleShape bg({maxTextWidth + padding * 2.f, lineHeight});
        bg.setFillColor(sf::Color(0, 0, 0, static_cast<uint8_t>(visible[i].alpha * 120.f)));
        bg.setPosition({marginLeft, y});
        window.draw(bg);

        // Text
        std::string display = visible[i].text;
        sf::Text text(font, display, fontSize);
        // Truncate if too wide
        if (text.getLocalBounds().size.x > maxTextWidth) {
            while (display.size() > 3 && text.getLocalBounds().size.x > maxTextWidth) {
                display.pop_back();
                text.setString(display + "...");
            }
        }
        text.setFillColor(sf::Color(200, 200, 200, a));
        text.setPosition({marginLeft + padding, y + 1.f * S});
        window.draw(text);
    }
}

// ============================================================================
// Destiny Deflect: Player pick overlay + Deck peek overlay
// ============================================================================
void UIManager::requestPlayerPick(const std::vector<int>& allowedPlayerIds) {
    std::cout << "[UIManager] Player pick requested, " << allowedPlayerIds.size() << " targets" << std::endl;
    showPlayerPickOverlay = true;
    playerPickAllowedIds = allowedPlayerIds;
    pendingTargeting = {};
    inputTimerClock.restart();
    inputTimerDuration = GameConfig::TARGET_PROMPT_DURATION;
}

void UIManager::showDeckPeekOverlay(const std::vector<PeekCardInfo>& cards, float duration) {
    std::cout << "[UIManager] Deck peek overlay: " << cards.size() << " cards, " << duration << "s" << std::endl;
    showDeckPeek = true;
    deckPeekCards = cards;
    deckPeekDuration = duration;
    deckPeekClock.restart();
}

void UIManager::dismissDeckPeek() {
    showDeckPeek = false;
    deckPeekCards.clear();
}

void UIManager::renderPlayerPickOverlay() {
    float S = UILayout::S();
    float elapsed = inputTimerClock.getElapsedTime().asSeconds();

    // Auto-pick on timeout: pick player with highest hand value (deterministic)
    if (elapsed >= inputTimerDuration) {
        showPlayerPickOverlay = false;
        int bestId = -1;
        int bestValue = -1;
        auto allInfo = gameState.getAllPlayerInfo();
        for (auto& info : allInfo) {
            auto it = std::find(playerPickAllowedIds.begin(), playerPickAllowedIds.end(), info.playerId);
            if (it != playerPickAllowedIds.end()) {
                if (info.handValue > bestValue || (info.handValue == bestValue && (bestId == -1 || info.playerId < bestId))) {
                    bestValue = info.handValue;
                    bestId = info.playerId;
                }
            }
        }
        if (bestId != -1) {
            PlayerTargeting target;
            target.targetPlayerIds.push_back(bestId);
            pendingTargeting = target;
            std::cout << "[UIManager] Player pick timed out, auto-picked P" << bestId << std::endl;
            confirmTargeting();
        }
        playerPickAllowedIds = {};
        return;
    }

    float centerX = window.getSize().x / 2.f;
    float boxW = window.getSize().x * 0.6f, boxH = 60.f * S;
    float boxY = window.getSize().y - 90.f * S;

    // Background strip at bottom
    sf::RectangleShape bg({boxW, boxH});
    bg.setFillColor(sf::Color(20, 40, 60, 220));
    bg.setOutlineThickness(2.f * S);
    bg.setOutlineColor(sf::Color(50, 200, 255));
    bg.setPosition({centerX - boxW / 2.f, boxY});
    window.draw(bg);

    // Title text
    sf::Text title(font, "Choose your victim", (unsigned int)(12 * S));
    title.setFillColor(sf::Color(50, 200, 255));
    sf::FloatRect tBounds = title.getLocalBounds();
    title.setPosition({centerX - tBounds.size.x / 2.f, boxY + 5.f * S});
    window.draw(title);

    // Timer bar
    float barW = boxW - 20.f * S;
    float barH = 5.f * S;
    float progress = 1.f - (elapsed / inputTimerDuration);
    float barY2 = boxY + boxH - 12.f * S;
    sf::RectangleShape barBg({barW, barH});
    barBg.setFillColor(sf::Color(60, 60, 60));
    barBg.setPosition({centerX - barW / 2.f, barY2});
    window.draw(barBg);
    sf::RectangleShape barFill({barW * progress, barH});
    barFill.setFillColor(progress > 0.3f ? sf::Color(100, 255, 100) : sf::Color(255, 80, 80));
    barFill.setPosition({centerX - barW / 2.f, barY2});
    window.draw(barFill);

    // Highlight eligible players
    for (auto& pv : playerVisuals) {
        auto it = std::find(playerPickAllowedIds.begin(), playerPickAllowedIds.end(), pv.playerId);
        if (it != playerPickAllowedIds.end()) {
            sf::FloatRect pvBounds = pv.playerSprite.getGlobalBounds();
            sf::RectangleShape highlight({pvBounds.size.x + 8.f * S, pvBounds.size.y + 8.f * S});
            highlight.setFillColor(sf::Color(50, 200, 255, 60));
            highlight.setOutlineThickness(2.f * S);
            highlight.setOutlineColor(sf::Color(50, 200, 255));
            highlight.setPosition({pvBounds.position.x - 4.f * S, pvBounds.position.y - 4.f * S});
            window.draw(highlight);
        }
    }
}

void UIManager::renderDeckPeekOverlay() {
    float S = UILayout::S();
    float elapsed = deckPeekClock.getElapsedTime().asSeconds();

    // Auto-dismiss on timeout
    if (elapsed >= deckPeekDuration) {
        dismissDeckPeek();
        return;
    }

    // Panel in top-right corner
    float panelW = 200.f * S, panelH = 190.f * S;
    float panelX = window.getSize().x - panelW - 10.f * S;
    float panelY = 10.f * S;

    sf::RectangleShape bg({panelW, panelH});
    bg.setFillColor(sf::Color(15, 15, 30, 230));
    bg.setOutlineThickness(2.f * S);
    bg.setOutlineColor(sf::Color(150, 100, 255));
    bg.setPosition({panelX, panelY});
    window.draw(bg);

    // Title
    sf::Text title(font, "Threads of fate exposed", (unsigned int)(11 * S));
    title.setFillColor(sf::Color(150, 100, 255));
    sf::FloatRect tBounds = title.getLocalBounds();
    title.setPosition({panelX + panelW / 2.f - tBounds.size.x / 2.f, panelY + 5.f * S});
    window.draw(title);

    // Draw 3 card sprites from the spritesheet
    if (cardVisuals.empty() || deckPeekCards.empty()) return;
    sf::Vector2u texSize = cardVisuals[0].cardSprite.getTexture().getSize();
    int cellW = (int)texSize.x / 15;
    int cellH = (int)texSize.y / 4;
    float cardScale = 0.4f * S;
    float scaledW = cellW * cardScale;
    float cardStartX = panelX + (panelW - scaledW * deckPeekCards.size() - 5.f * S * ((int)deckPeekCards.size() - 1)) / 2.f;
    float cardY = panelY + 25.f * S;

    for (int i = 0; i < (int)deckPeekCards.size(); i++) {
        auto& info = deckPeekCards[i];
        int col = static_cast<int>(info.rank) - 1;
        int row;
        switch (info.suit) {
            case Suit::Spades:   row = 0; break;
            case Suit::Diamonds: row = 1; break;
            case Suit::Clubs:    row = 2; break;
            case Suit::Hearts:   row = 3; break;
            default:             row = 0; break;
        }

        sf::Sprite cardSprite(cardVisuals[0].cardSprite.getTexture());
        cardSprite.setTextureRect(sf::IntRect({col * cellW, row * cellH}, {cellW, cellH}));
        cardSprite.setScale({cardScale, cardScale});
        cardSprite.setPosition({cardStartX + i * (scaledW + 5.f * S), cardY});
        window.draw(cardSprite);
    }

    // Timer bar
    float progress = 1.f - (elapsed / deckPeekDuration);
    float barW = panelW - 20.f * S;
    float barH = 4.f * S;
    float barY = panelY + panelH - 35.f * S;
    sf::RectangleShape barBg({barW, barH});
    barBg.setFillColor(sf::Color(60, 60, 60));
    barBg.setPosition({panelX + 10.f * S, barY});
    window.draw(barBg);
    sf::RectangleShape barFill({barW * progress, barH});
    barFill.setFillColor(sf::Color(150, 100, 255, 200));
    barFill.setPosition({panelX + 10.f * S, barY});
    window.draw(barFill);

    // OK button
    float okW = 80.f * S, okH = 25.f * S;
    float okX = panelX + panelW / 2.f - okW / 2.f;
    float okY = panelY + panelH - 30.f * S + barH + 2.f * S;
    sf::RectangleShape okBtn({okW, okH});
    okBtn.setFillColor(sf::Color(80, 60, 140));
    okBtn.setOutlineThickness(1.f * S);
    okBtn.setOutlineColor(sf::Color(150, 100, 255));
    okBtn.setPosition({okX, okY});
    window.draw(okBtn);

    sf::Text okText(font, "Sweet!", (unsigned int)(10 * S));
    okText.setFillColor(sf::Color::White);
    sf::FloatRect okBounds = okText.getLocalBounds();
    okText.setPosition({okX + okW / 2.f - okBounds.size.x / 2.f, okY + okH / 2.f - okBounds.size.y / 2.f - 2.f * S});
    window.draw(okText);
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