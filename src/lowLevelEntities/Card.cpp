#include "Card.h"
Suit Card::getSuit() const {
    return suit;
}
Rank Card::getRank() const {
    return rank;
}
Color Card::getColor() const {
    return color;
}
bool Card::isFaceUp() const {
    return faceUp;
}
void Card::flip() {
    faceUp = !faceUp;
}