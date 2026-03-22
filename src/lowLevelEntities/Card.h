#ifndef CARD_H
#define CARD_H
#include <string>
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <SFML/Network.hpp>
#include <SFML/Audio.hpp>
#include "GameConfig.h"
enum class Suit{
    Hearts,
    Diamonds,
    Clubs,
    Spades
};
enum class Rank{
    Ace = 1,
    Two,
    Three,
    Four,
    Five,
    Six,
    Seven,
    Eight,
    Nine,
    Ten,
    Jack,
    Queen,
    King
};
enum class Color {
    Red,
    Black
};
class Card {
    private:
        int id=-1;
        Suit suit;
        Rank rank;
        Color color;
        bool faceUp;
        int ownerId=-1;
        int handIndex=-1;
        int rankBonus=0;
    public:
        bool operator==(const Card& other) const {
            return rank == other.rank && suit == other.suit;
        }
        Card(Suit s, Rank r, bool faceUp){
            suit = s;
            rank = r;
            this->faceUp = faceUp;
            if (s == Suit::Hearts || s == Suit::Diamonds) {
                color = Color::Red;
            } else {
                color = Color::Black;
            }
        };
        //getters
        int getId() const{return id;}
        int getOwnerId() const;
        int getHandIndex() const;
        Suit getSuit() const;
        Rank getRank() const;
        std::string getRankAsString() const {
            switch (rank) {
                case Rank::Ace: return "A";
                case Rank::Two: return "2";
                case Rank::Three: return "3";
                case Rank::Four: return "4";
                case Rank::Five: return "5";
                case Rank::Six: return "6";
                case Rank::Seven: return "7";
                case Rank::Eight: return "8";
                case Rank::Nine: return "9";
                case Rank::Ten: return "10";
                case Rank::Jack: return "J";
                case Rank::Queen: return "Q";
                case Rank::King: return "K";
                default: return "?";
            }
        }
        std::string getSuitAsString() const {
            switch (suit) {
                case Suit::Hearts: return "H";
                case Suit::Diamonds: return "D";
                case Suit::Clubs: return "C";
                case Suit::Spades: return "S";
                default: return "?";
            }
        }   
        Color getColor() const;
        bool isFaceUp() const;
        int getRankBonus() const { return rankBonus; }
        void setRankBonus(int bonus) { rankBonus = bonus; }
        void resetRankBonus() { rankBonus = 0; }
        //setters
        void setId(int _id);
        void flip();
        void setOwnerId(int id);
        void setHandIndex(int id);
};

#endif