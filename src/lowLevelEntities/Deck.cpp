#include "Deck.h"

Deck::Deck(){
    for (int s = 0; s < 4; s++) {
        for ( int r = 1; r <= 13; r++) {
            cards.emplace_back(
                static_cast<Suit>(s), 
                static_cast<Rank>(r), false
            );
        }
    }
};

Card Deck::draw() {
    if (cards.empty()) {
        throw std::out_of_range("No cards left in the deck");
    }

    Card dealtCard = cards.back();

    cards.pop_back();
    std::cout   << "[deck.draw()] card drawn: " << dealtCard.getRankAsString() 
                << " of " << dealtCard.getSuitAsString() << std::endl;
    return dealtCard;
}

void Deck::shuffle() {
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(cards.begin(), cards.end(), g);
}

bool Deck::isEmpty(){
    return cards.empty();
}
void Deck::printDeck(){
    std::cout << "Current Deck:" << std::endl;
    for ( int i = 0; i < cards.size(); i++){
        std::cout << cards[i].getRankAsString() << " of " << cards[i].getSuitAsString() 
        << std::endl;
    }
}
void Deck::sortDeck(){
    std::sort(cards.begin(), cards.end(),
        [](const Card&a, const Card& b ){
            if (a.getSuit() != b.getSuit()){
                return a.getSuitAsString() < b.getSuitAsString();
            }
            return a.getRankAsString() < b.getRankAsString();
    });
}

void Deck::flipAllCardsFaceDown(){
    for (auto&card : cards){
        if(card.isFaceUp())
        card.flip();
    }
}