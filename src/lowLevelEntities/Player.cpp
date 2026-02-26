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
    for (auto& card : cardsInHand){
        card.setOwnerId(-1); //reset ownerId before returning to deck
        deck.retrieveCard(card);
    }
    cardsInHand.clear();
}

//return cards inside card pointers inside a vector
void Player::returnCards(Deck& deck, std::vector<Card*>& cards) {
    std::cout<< "Card(s) returned: ";
    for (Card* card : cards){
        std::cout<< card->getRankAsString() << " of " << card->getSuitAsString() << std::endl;
    }

    for (Card* card : cards) {
        card->setOwnerId(-1); //reset ownerId before returning to deck
        deck.retrieveCard(*card); //dereference Card* card

        auto it = std::find(cardsInHand.begin(), cardsInHand.end(), *card);
        if (it != cardsInHand.end()) {
            cardsInHand.erase(it);
        }
    }
    printCardsInHand();
}



void Player::flipAllCardsFaceUp(){
    for (auto& card : cardsInHand){
        if (!card.isFaceUp()){
            card.flip();
        }
    }
}

// ============================================================================
// Player Decision Logic
// ============================================================================
PlayerAction Player::playerTurnLogic(GameState& state){
    std::cout << "[playerTurnLogic] Player " << (isBot ? "(bot) " : "") << id << " thinking...Hand value: " << calculateHandValue() << std::endl;
    if (isBot){
       return botMode(state);
    }
    else {
        return manualMode(state);
    }
}

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
    else {
        return manualMode(state);
    }  
}

PlayerAction Player::manualMode(GameState& state){
    std::cout << "[manualMode]" << std::endl;
    displayManualMenu();
    char choice = getValidChoice();
    switch (choice){
        case '1':
            return PlayerAction::HIT;
        break;

        case '2':
            return PlayerAction::STAND;
        break;

        case '3':
            return PlayerAction::SKILL_REQUEST;
        break;

        case '4':
            displayGameState(state);
            return PlayerAction::IDLE;
        break;

        default: return PlayerAction::IDLE;
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
// Player Skill Targeting Logic
// ============================================================================
PlayerTargeting Player::targetAndConfirm(GameState& gameState){
    switch (skillName){
        case SkillName::DELIVERANCE:
            return skillTarget_Deliverance(gameState);
        break;
        case SkillName::NEURALGAMBIT:
            return skillTarget_NeuralGambit(gameState);
        default:
            std::cout<<" targetAndConfirm : can't find skill "<<std::endl;
            return PlayerTargeting{};
    }
}

PlayerTargeting Player::skillTarget_Deliverance(GameState& gameState){
    std::cout << "Skill: "<< skillNameToString() 
                <<" - Uses: "<< gameState.getPlayerInfo(id).skillUses << std::endl;
    std::cout << "Your cards:" << std::endl;

    for (auto& card: cardsInHand){
        std::cout << card.getRankAsString() << " of " << card.getSuitAsString() << std::endl;
    }
    std::cout <<"Please type id of a card: ";
    int cardId;
    std::cin >> cardId;
    while (cardId >= cardsInHand.size()){
        std::cout << "Wrong card id, try again: ";
        std::cin >> cardId; 
        std::cout<<std::endl;
    }

    PlayerTargeting target;
    target.targetCards.push_back(cardsInHand[cardId]);
    target.targetPlayerIds.push_back(id);

    return target;
}

PlayerTargeting Player::skillTarget_NeuralGambit(GameState& gameState){
    std::cout << "Skill: "<< skillNameToString() 
                <<" - Uses: "<< gameState.getPlayerInfo(id).skillUses << std::endl;
    std::cout << "Your cards:" << std::endl;

    for (auto& card: cardsInHand){
        std::cout << card.getRankAsString() << " of " << card.getSuitAsString() << std::endl;
    }

    std::cout <<"Please type id of a card: ";
    int cardId;
    std::cin >> cardId;
    while (cardId >= cardsInHand.size()){
        std::cout << "Wrong card id, try again: ";
        std::cin >> cardId; 
        std::cout<<std::endl;
    }
    
}
// ============================================================================
// Helper functions
// ============================================================================

int Player::calculateHandValue() const{
    int handValue = 0;
    int aceCount = 0;

    // First pass: count all cards except aces
    for (auto& card  : cardsInHand){
        Rank cardRank=card.getRank();
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
    for (auto& card : cardsInHand){
        std::cout<< card.getRankAsString() << " of " << card.getSuitAsString() << std::endl;
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