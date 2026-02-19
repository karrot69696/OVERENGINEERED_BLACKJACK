#ifndef CARD_H
#define CARD_H
#include <string>
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
        Suit suit;
        Rank rank;
        Color color;
        bool faceUp;
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
        void flip();
};

#endif