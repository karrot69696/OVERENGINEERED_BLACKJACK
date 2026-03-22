#ifndef DECK_H
#define DECK_H

#include <vector>
#include <algorithm>
#include <random>
#include "Card.h"
#include <iostream>

class Deck {
    private:
        std::vector<Card*> cards;
    public:
        Deck(){};
        void shuffle();
        Card* draw();
        void addCard(Card* card){
            cards.push_back(card);
        }
        void addCardToBottom(Card* card){
            cards.insert(cards.begin(), card);
        }
        bool isEmpty();
        void printDeck();
        void sortDeck();
        void clearCards(){
            cards.clear();
        }
        int getSize(){
            return (int)cards.size();
        }
        std::vector<Card*> getCards() {return cards;}
        void flipAllCardsFaceDown();
};

#endif