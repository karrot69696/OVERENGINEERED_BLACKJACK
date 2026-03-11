#include "VisualState.h"
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

        bool showFace = cheatOn || card->isFaceUp();

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

        cardVisuals.emplace_back(cardVisual);

        i++;
    }
}
