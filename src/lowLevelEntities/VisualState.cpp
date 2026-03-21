#include "VisualState.h"
#include <unordered_map>
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
// VisualState
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

VisualState::VisualState(sf::RenderWindow& window, GameState& gameState)
    : window(window), gameState(gameState) {

    std::filesystem::path fontPath =            "../assets/fonts/PixeloidSans.ttf";
    std::filesystem::path cardTexturePath =     "../assets/images/CuteCards.png";

    if (!font.openFromFile(fontPath)) {
        std::cerr << "[UIManager] Failed to load font from assets/fonts/PixeloidSans.ttf" << std::endl;
    }

    if (!cardTexture.loadFromFile(cardTexturePath)) {
        std::cerr << "[UIManager] Failed to load card texture from assets/images/CuteCards.png" << std::endl;
    }
    
}

void VisualState::buildCardVisuals(Deck& deck, std::vector<Player>& players){
    //coordinates for sprite creation
    sf::Vector2u texSize = cardTexture.getSize();
    int cellW = (int)texSize.x / 15;
    int cellH = (int)texSize.y / 4;
    float scaleX = UILayout::CARD_SIZE.x / cellW;
    float scaleY = UILayout::CARD_SIZE.y / cellH;

    //card coordinates on screen


    float spacing = cellW * scaleX * 0.004f;   // overlap spacing
    float cardWidth = cellW * scaleX;

    int totalCards = deck.getCards().size();

    float startX = 50.f;  // left side
    auto winSize = window.getSize();
    float windowH = static_cast<float>(winSize.y);
    float y = windowH / 2.f;  // middle vertically

    int i = 0 ;
        //build cardVisuals based on cards in deck
    for (auto* card : deck.getCards()) {

        bool showFace = cheatOn || card->isFaceUp() || card->getOwnerId() == localPlayerId;

        int col, row;

        if (showFace) {
            col = cardSpriteCol(card->getRank());
            row = cardSpriteRow(card->getSuit());
        } else {
            col = 14;
            row = 2;
        }

        sf::Sprite sprite(cardTexture);
        sprite.setTextureRect(sf::IntRect({col * cellW, row * cellH}, {cellW, cellH}));
        sprite.setScale({scaleX, scaleY});

        sprite.setPosition({startX + i * 0.03f, y + i * 0.03f});

        CardVisual cardVisual(sprite);
        cardVisual.cardId=card->getId();
        cardVisual.cardIndex = card->getHandIndex();
        cardVisual.ownerId = card->getOwnerId();
        cardVisual.location = CardLocation::DECK;
        cardVisual.highlighted = false;
        cardVisual.isTarget = false;
        cardVisual.faceUp = showFace;
        cardVisual.rankBonus=0;
        cardVisuals.emplace_back(cardVisual);

        i++;
    }
}

void VisualState::rebuildFromState(Deck& deck, std::vector<Player>& players) {
    cardVisuals.clear();

    sf::Vector2u texSize = cardTexture.getSize();
    int cellW = (int)texSize.x / 15;
    int cellH = (int)texSize.y / 4;
    float scaleX = UILayout::CARD_SIZE.x / cellW;
    float scaleY = UILayout::CARD_SIZE.y / cellH;

    auto winSize = window.getSize();
    float startX = static_cast<float>(winSize.x) * UILayout::DECK_X_RATIO;
    float windowH = static_cast<float>(winSize.y);
    float deckY = windowH / 2.f;

    int totalPlayers = (int)players.size();

    // Helper lambda to create a CardVisual from a Card*
    auto makeVisual = [&](Card* card, CardLocation location, sf::Vector2f pos) {
        bool showFace = cheatOn || card->isFaceUp() || card->getOwnerId() == localPlayerId;
        int col, row, rankBonus = 0;
        if (showFace) {
            col = cardSpriteCol(card->getRank());
            row = cardSpriteRow(card->getSuit());
            rankBonus = card->getRankBonus();
        } else {
            col = 14;
            row = 2;
        }

        sf::Sprite sprite(cardTexture);
        sprite.setTextureRect(sf::IntRect({col * cellW, row * cellH}, {cellW, cellH}));
        sprite.setScale({scaleX, scaleY});
        sprite.setPosition(pos);

        CardVisual cv(sprite);
        cv.cardId = card->getId();
        cv.cardIndex = card->getHandIndex();
        cv.ownerId = card->getOwnerId();
        cv.location = location;
        cv.highlighted = false;
        cv.isTarget = false;
        cv.faceUp = showFace;
        cv.rankBonus = rankBonus;
        cardVisuals.emplace_back(cv);
    };

    // 1. Deck cards — stacked at deck position
    int i = 0;
    for (auto* card : deck.getCards()) {
        makeVisual(card, CardLocation::DECK, {startX + i * 0.03f, deckY + i * 0.03f});
        i++;
    }

    // 2. Player hand cards — positioned at player seats
    for (auto& player : players) {
        sf::Vector2f seatPos = getPlayerSeatPos(player.getId(), totalPlayers);
        for (int c = 0; c < player.getHandSize(); c++) {
            Card* card = player.getCardInHand(c);
            float cardX = seatPos.x + c * UILayout::CARD_SPACING;
            float cardY = seatPos.y;
            makeVisual(card, CardLocation::HAND, {cardX, cardY});
        }
    }
}

void VisualState::reconcile(GameState& gs) {
    // Block reconcile while PresentationLayer is processing events
    // Prevents two writers (reconcile + event handlers) from racing on CardVisual metadata
    if (reconcileBlocked) return;

    // Lightweight sync: update CardVisual metadata from GameState
    // Does NOT touch sprite position — animations/PresentationLayer own that

    sf::Vector2u texSize = cardTexture.getSize();
    int cellW = (int)texSize.x / 15;
    int cellH = (int)texSize.y / 4;

    auto allInfo = gs.getAllPlayerInfo();

    // Build lookup: cardId → {ownerId, handIndex, faceUp}
    struct CardMeta { int ownerId; int handIndex; bool faceUp; int rankBonus;};
    std::unordered_map<int, CardMeta> desired;
    for (const auto& info : allInfo) {
        for (int i = 0; i < (int)info.cardsInHand.size(); i++) {
            const Card& c = info.cardsInHand[i];
            desired[c.getId()] = {info.playerId, i, c.isFaceUp(), c.getRankBonus()};
        }
    }

    // Update each existing CardVisual
    int changedOwner = 0, changedIndex = 0, changedFace = 0, changedLoc = 0, changedRank = 0;
    for (auto& cv : cardVisuals) {
        if (cv.isPinned()) continue;
        auto it = desired.find(cv.cardId);
        if (it != desired.end()) {
            // Card is in a player's hand
            if (cv.ownerId != it->second.ownerId) { changedOwner++; }
            if (cv.cardIndex != it->second.handIndex) { changedIndex++; }
            if (cv.location != CardLocation::HAND) { changedLoc++; }
            if (cv.rankBonus != it->second.rankBonus) { changedRank++; }
            cv.ownerId = it->second.ownerId;
            cv.cardIndex = it->second.handIndex;
            cv.location = CardLocation::HAND;
            cv.rankBonus = it->second.rankBonus;
            // Update face texture if faceUp state changed
            bool showFace = cheatOn || it->second.faceUp || it->second.ownerId == localPlayerId;
            if (cv.faceUp != showFace) {
                changedFace++;
                cv.faceUp = showFace;
                // We need suit/rank to pick the right texture rect — find from allInfo
                const Card& infoCard = allInfo[0].cardsInHand[0]; // placeholder
                for (const auto& info : allInfo) {
                    for (const Card& c : info.cardsInHand) {
                        if (c.getId() == cv.cardId) {
                            int col, row;
                            if (showFace) {
                                col = cardSpriteCol(c.getRank());
                                row = cardSpriteRow(c.getSuit());
                            } else {
                                col = 14;
                                row = 2;
                            }
                            cv.cardSprite.setTextureRect(sf::IntRect(
                                {col * cellW, row * cellH}, {cellW, cellH}));
                            break;
                        }
                    }
                }
            }
        } else {
            // Card not in any player's hand → it's in the deck
            if (cv.location != CardLocation::DECK) { changedLoc++; }
            cv.ownerId = -1;
            cv.cardIndex = -1;
            cv.rankBonus = 0;
            cv.location = CardLocation::DECK;
        }
    }
    (void)changedOwner; (void)changedIndex; (void)changedFace; (void)changedLoc; (void)changedRank;
}

void VisualState::flipCardVisualFaceUp(int cardId) {
    sf::Vector2u texSize = cardTexture.getSize();
    int cellW = (int)texSize.x / 15;
    int cellH = (int)texSize.y / 4;

    CardVisual& cv = getCardVisual(cardId);
    if (cv.faceUp) return; // already face-up

    // Look up rank/suit from gameState
    Rank rank = gameState.getCardRank(cv.ownerId, cv.cardIndex);
    Suit suit = gameState.getCardSuit(cv.ownerId, cv.cardIndex);
    int col = cardSpriteCol(rank);
    int row = cardSpriteRow(suit);
    cv.cardSprite.setTextureRect(sf::IntRect({col * cellW, row * cellH}, {cellW, cellH}));
    cv.faceUp = true;
}

void VisualState::flipCardVisualFaceDown(int cardId) {
    sf::Vector2u texSize = cardTexture.getSize();
    int cellW = (int)texSize.x / 15;
    int cellH = (int)texSize.y / 4;

    CardVisual& cv = getCardVisual(cardId);
    if (!cv.faceUp) return;

    cv.cardSprite.setTextureRect(sf::IntRect({14 * cellW, 2 * cellH}, {cellW, cellH}));
    cv.faceUp = false;
}

void VisualState::enforceVisibility() {
    sf::Vector2u texSize = cardTexture.getSize();
    int cellW = (int)texSize.x / 15;
    int cellH = (int)texSize.y / 4;

    // Build lookup from game logic: cardId → {logicFaceUp, ownerId, rank, suit}
    struct FaceInfo { bool logicFaceUp; int ownerId; Rank rank; Suit suit; };
    std::unordered_map<int, FaceInfo> logic;
    auto allInfo = gameState.getAllPlayerInfo();
    for (const auto& info : allInfo) {
        for (const Card& c : info.cardsInHand) {
            logic[c.getId()] = {c.isFaceUp(), info.playerId, c.getRank(), c.getSuit()};
        }
    }

    for (auto& cv : cardVisuals) {
        if (cv.isPinned()) continue;

        auto it = logic.find(cv.cardId);
        bool shouldBeUp;
        if (it != logic.end()) {
            // Card is in a player's hand
            shouldBeUp = cheatOn || it->second.logicFaceUp || it->second.ownerId == localPlayerId;
        } else {
            // Card is in the deck — face-down (unless cheat)
            shouldBeUp = cheatOn;
        }

        if (shouldBeUp && it != logic.end()) {
            // Always verify correct face texture (guards against stale cardIndex flips)
            sf::IntRect expected({cardSpriteCol(it->second.rank) * cellW,
                                  cardSpriteRow(it->second.suit) * cellH}, {cellW, cellH});
            if (!cv.faceUp || cv.cardSprite.getTextureRect() != expected) {
                cv.cardSprite.setTextureRect(expected);
                cv.faceUp = true;
            }
        } else {
            if (cv.faceUp) {
                cv.cardSprite.setTextureRect(sf::IntRect({14 * cellW, 2 * cellH}, {cellW, cellH}));
                cv.faceUp = false;
            }
        }
    }
}
