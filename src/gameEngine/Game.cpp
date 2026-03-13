#include "Game.h"

// ============================================================================
// Game Implementation
// ============================================================================
Game::Game(sf::RenderWindow& window)
    : 
        window(window),
        gameState(), deck(), players(), skillDeck(),
        visualState(this->window, this->gameState),
        uiManager(this->window, this->gameState,this->visualState, this->visualState.getCardVisuals()),
        animationManager(this->window,this->gameState, this->visualState)
{
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
    for (int i=0;i<allCards.size();i++){
        Card* card = allCards[i].get();
        card->setId(i); //every card has a unique ID
        deck.addCard(card);
    }

    visualState.buildCardVisuals(deck, players);
            
}

void Game::SetupGame(){

    //skillDeck.shuffle();
    
    //get inputs
    int numPlayers = 3;
    int numBots = 2;
    // std::cout << "Enter number of players: ";
    // std::cin >> numPlayers;
    // std::cout << "Enter number of bots: ";
    // std::cin >> numBots; 
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
    RoundManager roundManager(
        this->players, 
        this->deck,
        this->gameState,
        this->visualState, 
        this->uiManager, 
        this->animationManager
    );
    roundManager.createSkills();
    roundManager.updateGameState(PhaseName::GAME_START_PHASE, 0);
    roundManager.changePhase(PhaseName::GAME_START_PHASE);
    sf::Clock clock;

    while (window.isOpen()){

        float deltaTime = clock.restart().asSeconds();

        while (std::optional event = this->window.pollEvent()){
            eventHandler(event);
        }

        roundManager.update();
        animationManager.update(deltaTime);

        window.clear();

        uiManager.render();
        animationManager.render();
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



