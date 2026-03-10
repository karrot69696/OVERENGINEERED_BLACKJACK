#include "Game.h"

// ============================================================================
// Game Implementation
// ============================================================================
Game::Game(sf::RenderWindow& window)
    : 
        window(window),
        gameState(),
        visualState(gameState, this->deck, this->players),
        uiManager(window, gameState, visualState.getCardVisuals()),
        animationManager(window, gameState, visualState)
{}

void Game::dealInitialCards(int numCards){
    for (int i = 0; i < numCards; i++){
        for (auto& player: players){
            player.addCardToHand(deck.draw());
        }
    }
}

void Game::SetupGame(){
    //initialize card container
    for (int s = 0; s < 4; s++) {
        for ( int r = 1; r <= 13; r++) {
            this->allCards.push_back(std::make_unique<Card>(
                static_cast<Suit>(s), 
                static_cast<Rank>(r), false
            ));
        }
    }
    //initialize deck 
    for (auto& card : this->allCards) {
        deck.addCard(card.get());
    }
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
        std::cout << "Invalid input. Using defaults (2 bots, 1 players)." << std::endl;
        numPlayers = 3;
        numBots = 2;  
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
    

}   

void Game::RunGame(){
    std::cout << "\n=== GAME START ===\n" << std::endl;
    SkillManager skillManager;
    RoundManager roundManager(players, deck, skillManager, this->gameState, this->uiManager, this->animationManager);
    roundManager.createSkills();
    roundManager.updateGameState(PhaseName::BLACKJACK_CHECK_PHASE, 0);
    roundManager.changePhase(PhaseName::BLACKJACK_CHECK_PHASE);
    sf::Clock clock;

    //deal initial cards to players
    dealInitialCards(initialCardCount);
    while (window.isOpen()){

        float deltaTime = clock.restart().asSeconds();

        while (std::optional event = this->window.pollEvent()){
            eventHandler(event);
        }

        roundManager.update();
        animationManager.update(deltaTime);

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



