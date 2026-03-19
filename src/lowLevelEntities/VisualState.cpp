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
        sprite.setPosition(pos);

        CardVisual cv(sprite);
        cv.cardId = card->getId();
        cv.cardIndex = card->getHandIndex();
        cv.ownerId = card->getOwnerId();
        cv.location = location;
        cv.highlighted = false;
        cv.isTarget = false;
        cv.faceUp = showFace;
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
    // Lightweight sync: update CardVisual metadata from GameState
    // Does NOT touch sprite position — animations/PresentationLayer own that

    sf::Vector2u texSize = cardTexture.getSize();
    int cellW = (int)texSize.x / 15;
    int cellH = (int)texSize.y / 4;

    auto allInfo = gs.getAllPlayerInfo();

    // Build lookup: cardId → {ownerId, handIndex, faceUp}
    struct CardMeta { int ownerId; int handIndex; bool faceUp; };
    std::unordered_map<int, CardMeta> desired;
    for (const auto& info : allInfo) {
        for (int i = 0; i < (int)info.cardsInHand.size(); i++) {
            const Card& c = info.cardsInHand[i];
            desired[c.getId()] = {info.playerId, i, c.isFaceUp()};
        }
    }

    // Update each existing CardVisual
    int changedOwner = 0, changedIndex = 0, changedFace = 0, changedLoc = 0;
    for (auto& cv : cardVisuals) {
        auto it = desired.find(cv.cardId);
        if (it != desired.end()) {
            // Card is in a player's hand
            if (cv.ownerId != it->second.ownerId) { changedOwner++; }
            if (cv.cardIndex != it->second.handIndex) { changedIndex++; }
            if (cv.location != CardLocation::HAND) { changedLoc++; }
            cv.ownerId = it->second.ownerId;
            cv.cardIndex = it->second.handIndex;
            cv.location = CardLocation::HAND;

            // Update face texture if faceUp state changed
            bool showFace = cheatOn || it->second.faceUp;
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
            cv.location = CardLocation::DECK;
        }
    }
    if (changedOwner || changedIndex || changedFace || changedLoc) {
        std::cout << "[Reconcile] owner=" << changedOwner
                  << " index=" << changedIndex
                  << " face=" << changedFace
                  << " loc=" << changedLoc << std::endl;
    }
}
