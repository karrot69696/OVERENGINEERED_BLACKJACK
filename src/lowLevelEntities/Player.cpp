#include "Player.h"
// ============================================================================
// Card Manipulation
// ============================================================================

void Player::Stand() {
    std::cout << " >> Stand! Final hand value: " 
              << calculateHandValue() << std::endl;
}\
//return all cards
void Player::returnCards(Deck& deck){
    std::cout<< "Card(s) returned: ";
    for (Card* card : cardsInHand){
        std::cout<< card->getRankAsString() << " of " << card->getSuitAsString() << std::endl;
    }
    for (auto* card : cardsInHand){
        card->setOwnerId(-1);
        card->setHandIndex(-1); //reset ownerId before returning to deck
        deck.addCard(card);
    }
    cardsInHand.clear();
}

//return cards inside card pointers inside a vector
void Player::returnCards(Deck& deck, const std::vector<Card*>& cards) {

    std::cout << "Card(s) returned:\n";

    for (Card* card : cards){
        std::cout << card->getRankAsString()
                  << " of "
                  << card->getSuitAsString()
                  << std::endl;
    }

    for (Card* card : cards) {

        card->setOwnerId(-1);
        card->setHandIndex(-1);

        deck.addCard(card);

        auto it = std::find(cardsInHand.begin(), cardsInHand.end(), card);

        if (it != cardsInHand.end()) {
            cardsInHand.erase(it);
        }
    }

    for (int i = 0; i < (int)cardsInHand.size(); i++){
        cardsInHand[i]->setHandIndex(i);
    }

    printCardsInHand();
}

void Player::addCardToHand(Card* card){
    if (card == nullptr) {
        std::cout << "[Player] Card is null" << std::endl;
        return;
    }
    card->setOwnerId(this->id);
    card->setHandIndex((int)cardsInHand.size());
    cardsInHand.push_back(card);
    
    std::cout << "[Player] Card owned by Player "<< card->getOwnerId() 
    << " at index "<<card->getHandIndex()
    <<std::endl;
}

void Player::flipAllCardsFaceUp(){
    for (auto* card : cardsInHand){
        if (!card->isFaceUp()){
            card->flip();
        }
    }
}

// ============================================================================
// Player Decision Logic
// ============================================================================

PlayerAction Player::hostTurnLogic(GameState& state, Player& opponent, int hostLossCount){
    std::cout << "Host player " << id <<(isBot? "(bot)" : "") << " thinking...Hand value: " << calculateHandValue() << std::endl;
    if (isBot){
        //hit continuously until qualified to stand
        while (calculateHandValue() < GameConfig::DEALER_STAND_VALUE){  
            std::cout << "Host (bot) " << id << " decides to hit." << std::endl;
            return PlayerAction::HIT;
        }
        //hard decision making
        while (shouldHostHit(opponent, hostLossCount)){
            std::cout << "Host (bot)" << id << "decides to hit." << std::endl;
            return PlayerAction::HIT;
        }
        return PlayerAction::STAND;
    }
}

PlayerAction Player::botMode(GameState& state){
    //hit continuously until qualified to stand
    while (calculateHandValue() < GameConfig::DEALER_STAND_VALUE){  
       return PlayerAction::HIT;
    }
    //randomly decide to hit based on burst risk
    std::cout << "Evaluating risk and gacha..." << std::endl;
    while (calculateHandValue() < GameConfig::BLACKJACK_VALUE && shouldTakeRisk(burstRiskCalculation()) ){
        return PlayerAction::HIT;
    }
    return PlayerAction::STAND;
}
// ============================================================================
// Helper functions
// ============================================================================

int Player::calculateHandValue() const{
    int handValue = 0;
    int aceCount = 0;

    // First pass: count all cards except aces
    for (auto* card  : cardsInHand){
        Rank cardRank = card->getRank();
        if (cardRank == Rank::Jack || cardRank == Rank::Queen
            || cardRank == Rank::King){
               handValue += GameConfig::FACE_CARD_VALUE; 
        }
        else if (cardRank == Rank::Ace){
            aceCount++;
        }
        else handValue +=  static_cast<int>(cardRank);
    }

    // Second pass: handle aces optimally
    for (int i = 0; i < aceCount; i++) {
        if (handValue + GameConfig::ACE_HIGH_VALUE <= GameConfig::BLACKJACK_VALUE) {
            handValue += GameConfig::ACE_HIGH_VALUE;
        } else {
            handValue += GameConfig::ACE_LOW_VALUE;
        }
    }
    return handValue;
}

double Player::burstRiskCalculation(){
    double safeRange = GameConfig::BLACKJACK_VALUE - calculateHandValue();
    double burstRisk = 1 - (safeRange/  GameConfig::CARD_RANKS);
    std::cout << "Burst risk: " << burstRisk * 100 << std::endl;
    return std::clamp(burstRisk, 0.0, 1.0);
}

bool Player::shouldTakeRisk(double risk) const {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0, 100);
    double rng=dis(gen);
    std::cout << "RNG:" << rng << std::endl;
    if (rng >= risk * 100){
        return true;
    }
    return false;
}

bool Player::shouldHostHit(Player& opponent, double lossRate){
    int myValue = calculateHandValue();
    double burstPressure =  burstRiskCalculation();

    //Consider burst risk first
    if (!shouldTakeRisk(burstPressure)){
        return 0;
    }

    //If their initial value is higher than us , the closer they are to 21 the more pressure
    double opponentHandValue = opponent.calculateHandValue();
    double opponentRisk = 0;

    //if opponent is stronger, starts calculating pressure
    if (opponentHandValue > myValue && opponentHandValue <= GameConfig::BLACKJACK_VALUE){
            double oppStrength= GameConfig::BLACKJACK_VALUE - opponentHandValue;
            double maxStrength = GameConfig::BLACKJACK_VALUE - GameConfig::DEALER_STAND_VALUE;
            opponentRisk = 1.0 - oppStrength / maxStrength;
            opponentRisk = std::clamp(opponentRisk, 0.0, 1.0);
    }

    //pressure, more loss = more aggressive = lower the risk
    double totalRisk = opponentRisk * GameConfig::OPPONENT_PRESSURE_WEIGHT
                        - lossRate * GameConfig::LOSING_PRESSURE_WEIGHT ;

    // Debug output
    std::cout << "| Burst: " << static_cast<int>(burstPressure * 100) << "% "
              << "| Opponent: " << static_cast<int>(opponentRisk * 100) << "% "
              << "| Loss Rate: " << static_cast<int>(lossRate * 100) << "% "
              << "| Total: " << static_cast<int>(totalRisk * 100) << "%" << std::endl;
    
    //Make decision
    return shouldTakeRisk(totalRisk);
}

// ============================================================================
// Print functions
// ============================================================================
char Player::getValidChoice() const {
    char choice;
    std::cin >> choice;
    
    while (choice != '1' && choice != '2' && choice != '3' && choice != '4'
                         && choice != '4') {
        std::cout << "Invalid choice...";
        std::cin >> choice;
    }

    return choice;
}

void Player::displayManualMenu() const {
    std::cout << "\n  ____________________________" << std::endl;
    std::cout << "  | Your turn - Choose action  |" << std::endl;
    std::cout << "  |____________________________|" << std::endl;
    std::cout << "  | 1. Hit                     |" << std::endl;
    std::cout << "  | 2. Stand                   |" << std::endl;
    std::cout << "  | 3. Use Skill               |" << std::endl;
    std::cout << "  | 4. Check game state        |" << std::endl;
    std::cout << "  |____________________________|" << std::endl;
    std::cout << "  Choice: ";
}

void Player::displayGameState(GameState& state) const {
    state.printAllInfo();
}

void Player::printCardsInHand(){
    std::cout<<"Player " << id <<"'s hand: " << std::endl;
    for (auto* card : cardsInHand){
        std::cout<< card->getRankAsString() << " of " << card->getSuitAsString() 
        << " index: "<< card->getHandIndex()<< std::endl;
    }
}

std::string Player::skillNameToString(){
    switch (skillName){
        case SkillName::DELIVERANCE:
            return "DELIVERANCE";
        break;
        default:
            return "[skillNameToString] Skill name not found";
    }
} 