#include "Game.h"

// ============================================================================
// Game Implementation
// ============================================================================
void Game::dealInitialCards(int numCards){
    for (int i = 0; i < numCards; i++){
        for (auto& player: players){
            player.addCardToHand(deck.draw());
        }
    }
}

void Game::displayPoints(){
    std::cout << "Points tally:" << std::endl;
    for (auto& player : players){
        std::cout << "Player " << player.getId() << ": " << player.getPoint() << " points." << std::endl;
    }
}

void Game::SetupGame(){

    //initialize deck and skill deck
    deck = Deck();
    deck.shuffle();
    skillDeck = SkillDeck();
    //skillDeck.shuffle();
    
    //get inputs
    int numPlayers;
    int numBots;
    std::cout << "Enter number of players: ";
    std::cin >> numPlayers;
    std::cout << "Enter number of bots: ";
    std::cin >> numBots; 
    // Validate input
    if (numPlayers <= 1 || numBots < 0 ) {
        std::cout << "Invalid input. Using defaults (2 bots, 2 players)." << std::endl;
        numPlayers = 2;  
    }
    else{
        numPlayers = (numPlayers>maxNumPlayer)? maxNumPlayer : numPlayers;
        numBots = (numBots>numPlayers)? numPlayers: numBots;
    }
    int numHumman = numPlayers - numBots; 
    //create players
    for (int i = 0; i <= numHumman-1 ; i++){
        SkillName newSkill = skillDeck.drawSkill();
        std::cout<< "Player " << i << " created." << std::endl;
        players.emplace_back( i ,newSkill, 0, 0);
    }
    //create bots
    for (int i = numHumman; i <= numPlayers-1; i++){
        std::cout<< "Player (bot) " << i << " created." << std::endl;
        SkillName newSkill = skillDeck.drawSkill();
        players.emplace_back( i ,newSkill, 1, 0);
    }

    //choose host
    players.front().setHost();
    std::cout << "Player " << players.front().getId() << " is the host." << std::endl;
    //deal initial cards to players
    dealInitialCards(initialCardCount);

}   

void Game::RunGame(){
    std::cout << "\n=== GAME START ===\n" << std::endl;
    SkillManager skillManager;
    RoundManager roundManager(players, deck, skillManager, this->gameState, this->uiManager);
    roundManager.createSkills();
    roundManager.updateGameState(PhaseName::BLACKJACK_CHECK_PHASE, gameState.getCurrentPlayerId());
    int round = 0;
    // for (int round = 0; round < players.size(); round++) {
    //     roundManager.playRound(round);
    //     displayPoints();
    // }
    while (window.isOpen()){

        while (std::optional event = this->window.pollEvent()){
            eventHandler(event);
        }

        if(!roundManager.playRound(round)){
            std::cout << "\n=== GAME OVER ===\n" << std::endl;
            return;
        }
        window.clear();
        uiManager.render();
        window.display();
        
    }
            
    // Cleanup
    players.clear();
    deck.clearCards();
    skillDeck.clearCards();
    
}
void Game::eventHandler(const std::optional<sf::Event>& event){

    if ( event->is<sf::Event::Closed>()){
        window.close();
    }
    else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()){
        if (keyPressed->scancode == sf::Keyboard::Scancode::Escape){
            window.close();
        }
    }
    uiManager.handleEvent(event);
}



