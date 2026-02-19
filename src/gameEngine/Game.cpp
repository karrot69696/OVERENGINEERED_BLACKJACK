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
    players.front().giveHost();
    std::cout << "Player " << players.front().getId() << " is the host." << std::endl;
    //deal initial cards to players
    dealInitialCards(initialCardCount);

}   

void Game::RunGame(){
    std::cout << "\n=== GAME START ===\n" << std::endl;
    //create skills
    SkillManager skillManager;
    GameState gameState;
    RoundManager roundManager(players, deck, skillManager, gameState);
    roundManager.createSkills();

    for (int round = 0; round < players.size(); round++) {
        roundManager.playRound(round);
        displayPoints();
    }
    
    std::cout << "\n=== GAME OVER ===\n" << std::endl;
    displayPoints();
    
    // Cleanup
    players.clear();
    deck.clearCards();
    skillDeck.clearCards();
}

// ============================================================================
// RoundManager Implementation
// ============================================================================

    // ========================================================================
    // Main round loop
    // ========================================================================
void RoundManager::playRound(int round){
    std::cout << "[playRound] Round " << round + 1 << " begins!" << std::endl;

    //if handleBlackjackPhase returns false -> host has blackjack -> end round immediately
    Player& hostPlayer = getHostPlayer();
    if (!blackJackHandler()) {  
       updateGameState(Phase::ROUND_END,0);
        return;
    }
    std::cout<<std::endl;

    //TURN: HIT, STAND, SKILL
    for (auto& currentPlayer : players){
        updateGameState(Phase::PLAYER_HIT_PHASE,currentPlayer.getId());
        if (currentPlayer.getHost() == 0 && currentPlayer.getBlackJacked() == 0){

            std::cout << "[playRound] Player " << currentPlayer.getId() 
            << (currentPlayer.isBot ? " (BOT)" : "") << "'s turn" << std::endl;
            //choosing between hit, stand or skill, loop until busted or stand
            turnHandler(currentPlayer, currentPlayer);
            std::cout << std::endl;
        }
    }

    //HOST FIGHTS EACH PLAYER, BEFORE EVERY FIGHT HOST GETS A TURN (HIT, STAND, SKILL)
    for (auto& currentPlayer : players){
        if (currentPlayer.getHost() == 0 && currentPlayer.getBlackJacked() == 0){
            battleHandler(hostPlayer,currentPlayer);
        }
    }

    //PREPARE FOR NEXT ROUND
    updateGameState(Phase::ROUND_END,gameState.getCurrentPlayerId());
    if (round < players.size() - 1) {
            roundEndHandler();
    }
}
    // ========================================================================
    // Phase handlers
    // ========================================================================
bool RoundManager::blackJackHandler(){
    for (auto& currentPlayer : players){

        updateGameState(Phase::BLACKJACK_CHECK_PHASE,currentPlayer.getId());
        if (currentPlayer.calculateHandValue() == 21 && currentPlayer.getHandSize() == 2){
            if (currentPlayer.getHost() == 1){
                std::cout << "Host player has Black Jack! Host player wins the round!" << std::endl;
                currentPlayer.gainPoint(players.size()-1);
                return false;
            }
            else {
                std::cout << "Player " << currentPlayer.getId() << " has Black Jack! +2 points" << std::endl;
                currentPlayer.flipAllCardsFaceUp();
                currentPlayer.gainPoint(2);
                currentPlayer.blackJackSet();
            }
        }
    }
    return true;
    std::cout<<std::endl;
}

void RoundManager::turnHandler(Player& player, Player& opponent){
    bool turnOver = false;
    PlayerTargeting targets;
    while (!turnOver){
        PlayerAction action;
        if (player.getHost()){
            action =  player.hostTurnLogic(gameState, opponent, player.getLoss()/player.getBattleCount());
        }
        else {
            action = player.playerTurnLogic(gameState);
        }
        switch (action){

            case PlayerAction::HIT:
                player.addCardToHand(deck.draw()); 
                std::cout   << "[turnHandler] Player "<<player.getId()
                            <<" hits - hand value:"<<player.calculateHandValue() 
                            <<std::endl;
                updateGameState(Phase::PLAYER_HIT_PHASE,player.getId());
            break;

            case PlayerAction::SKILL_REQUEST:

                std::cout   
                    << "[turnHandler] Player "<<player.getId()
                    << " requested skill: "
                    << player.skillNameToString() 
                << std::endl;

                if(skillManager.getSkillUses(player.getSkillName()) > 0){
                    skillHandler(player);
                    updateGameState(Phase::PLAYER_HIT_PHASE,player.getId());
                }
                else{
                    std::cout<< "[turnHandler] SKILL OUT OF USES" << std::endl;
                }

            break;

            case PlayerAction::STAND:
                std::cout   << "[turnHandler] Player "<<player.getId()
                            <<" stands - hand value:"<<player.calculateHandValue() 
                            <<std::endl;
                turnOver=1;      
            break;

            case PlayerAction::IDLE:
                std::cout << "[turnHandler] Idling..." << std::endl;
            break;
        }
    }
}

void RoundManager::battleHandler(Player& host, Player& opponent){

    int opponentHandValue = opponent.calculateHandValue();
    int hostHandValue = host.calculateHandValue() ;

    std::cout << "[resolveBattle] HOST [" << hostHandValue
                << "] vs Player " << opponent.getId() 
                <<" [" << opponentHandValue << "]" << std::endl;


    //prepare for battle, update game state, turn handler
    host.gainBattleCount();
    opponent.gainBattleCount();
    opponent.flipAllCardsFaceUp();
    updateGameState(Phase::HOST_HIT_PHASE,host.getId());
    turnHandler(host,opponent);
    std::cout << std::endl;
    

    //start battle, reprint hand value after host's turn since manipulation might
    //have happenned here
    updateGameState(Phase::BATTLE_PHASE,opponent.getId());
    host.flipAllCardsFaceUp();
    opponentHandValue = opponent.calculateHandValue();
    hostHandValue = host.calculateHandValue() ;
    std::cout << "[resolveBattle] HOST [" << hostHandValue
                << "] vs Player " << opponent.getId() <<" [" << opponentHandValue <<"]"<< std::endl;
    
    //determine outcome
    if (hostHandValue > GameConfig::BLACKJACK_VALUE){
        std::cout << "[resolveBattle] Host BUSTS" << std::endl;
        host.gainLoss();
        opponent.gainPoint();
    }
    else if (opponentHandValue > GameConfig::BLACKJACK_VALUE){
        std::cout << "[resolveBattle] Player " << opponent.getId() << " BUSTS!" << std::endl;
        host.gainPoint();
        opponent.gainLoss();
    }
    else if (hostHandValue > opponentHandValue){
        std::cout << "[resolveBattle] Host WINS " << std::endl;
        host.gainPoint();
        opponent.gainLoss();
    }
    else if (hostHandValue < opponentHandValue){
        std::cout << "[resolveBattle] Player " << opponent.getId() << " WINS" << std::endl;
        opponent.gainPoint();
        host.gainLoss();
    }
    else {
        host.gainLoss();
        std::cout << "[resolveBattle] TIE " << opponent.getId() << std::endl;
    }
    std::cout<<std::endl;
}

void RoundManager::skillHandler(Player& player){
    PlayerTargeting targets = player.targetAndConfirm(gameState);
    // struct PlayerTargeting{
    //     std::vector<int> targetPLayerIds;
    //     std::vector<Card*> targetCards;
    // };

    //grab ACTUAL players from the given ids.
    std::vector<Player*> actualTargets;

    for (int id : targets.targetPLayerIds) {
        for (auto& p : players) {
            if (p.getId() == id) {
                actualTargets.push_back(&p); //*pointerToObject = &object
            }
        }
    }
    // struct SkillContext {
    //     Player& user;
    //     std::vector<Player*> targetPlayers;
    //     std::vector<Card*> targetCards;
    //     Deck& deck;
    //     GameState& state;        // read/write if needed
    // };

    SkillContext context{
        player,
        actualTargets,
        targets.targetCards,
        deck,
        gameState
    };

    skillManager.processSkill(context);
}

void RoundManager::roundEndHandler(){
    Player& hostPlayer = getHostPlayer();
    //new round
    std::cout << "[roundEndHandler] starting new round..." << std::endl;

    //give host to next player
    players[hostPlayer.getId() + 1].giveHost();

    std::cout << "[roundEndHandler] Player " 
    << players[hostPlayer.getId() + 1].getId() 
    << " is the new host." << std::endl;

    hostPlayer.removeHost();

    //change all cards back to normal  //TO DO


    //reset skill uses
    skillManager.resetSkillUses(players);

    //All players return cards to deck + reset blackjack
    for (auto& player : players){
        player.returnCards(deck);
        player.blackJackSet(false);
    }
    deck.flipAllCardsFaceDown();
    deck.sortDeck();
    deck.printDeck();

    //redeal cards
    deck.shuffle();
    dealCardsToPlayers(2);


    std::cout<<std::endl;
}

    // ========================================================================
    // Helper functions
    // ========================================================================
void RoundManager::updateGameState(Phase phase, int playerId){
    std::cout<< "[updateGameState] - "<< phaseToString() 
                << " Player Id: "<< playerId << std::endl;
                
    gameState.updatePhase(phase, playerId);
    gameState.updateDeckCount(deck.getSize());
    //update playerInfo
    std::vector<PlayerInfo> allPlayerInfos;
    for (auto& player : players){
        //struct PlayerInfo {
        //     int playerId;
        //     vector<Card> cardsInHand;
        //     SkillName skill;
        //     int skillUses;
        // };
        std::vector<Card> playerHand;
        for (int i = 0; i < player.getHandSize(); i++){
            playerHand.push_back(player.getCardInHand(i));
        }
        
        PlayerInfo playerInfo{  
            player.getId(),
            playerHand,
            player.getSkillName(),
            skillManager.getSkillUses(player.getSkillName()),
            player.getPoint()
        }; 

        allPlayerInfos.push_back(playerInfo);
    }
    gameState.updateAllPlayerInfo(allPlayerInfos);
}

Player& RoundManager::getHostPlayer() {
    auto it = std::find_if(
        players.begin(),
        players.end(),
        [](const Player& p) { return p.getHost() == 1; }
    );

    if (it == players.end()) {
        throw std::runtime_error("Host player not found");
    }
    return *it;
}

void RoundManager::dealCardsToPlayers(int numCards){
    for (int i = 0; i < numCards; i++){
        for (auto& player : players){
            player.addCardToHand(deck.draw());
        }
    }
}

void RoundManager::createSkills(){
    skillManager.createSkills(players);
}

std::string RoundManager::phaseToString(){
        switch(gameState.getPhase()){   
            case Phase::BATTLE_PHASE:
                return "BATTLE PHASE";
            break;
            case Phase::BLACKJACK_CHECK_PHASE:
                return "BLACKJACK CHECK PHASE";
            break;
            case Phase::ROUND_END:
                return "ROUND END";
            break;
            case Phase::HOST_HIT_PHASE:
                return "HOST HIT PHASE";
            break;
            case Phase::PLAYER_HIT_PHASE:
                return "PLAYER HIT PHASE ";
            break;
            default:
                return "UNKNOWN PHASE";          
        }
    }