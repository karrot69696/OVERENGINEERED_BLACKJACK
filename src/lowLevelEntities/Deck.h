#ifndef DECK_H
#define DECK_H

#include <vector>
#include <algorithm>
#include <random>
#include "Card.h"
#include <iostream>

class Deck {
    private:
        std::vector<Card> cards;
    public:
        Deck();
        void shuffle();
        Card draw();
        void retrieveCard(const Card& card){
            cards.push_back(card);
        }
        bool isEmpty();
        void printDeck();
        void sortDeck();
        void clearCards(){
            cards.clear();
        }
        int getSize(){
            return cards.size();
        }
        void flipAllCardsFaceDown();
};

#endif