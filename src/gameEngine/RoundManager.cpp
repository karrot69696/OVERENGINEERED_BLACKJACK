#include "RoundManager.h"
#include "Game.h"


// ========================================================================
// OLD MAIN ROUND LOOP
// ========================================================================
bool RoundManager::playRound(int& round){

    //if handleBlackjackPhaseName returns false -> host has blackjack -> end round immediately
    switch (this->gameState.getPhaseName()){
        case PhaseName::BLACKJACK_CHECK_PHASE:
            //BLACKJACK CHECK FOR ALL PLAYERS
            if (!blackJackHandler()) {
                if(players[gameState.getCurrentPlayerId()].getHost() == 1 && players[gameState.getCurrentPlayerId()].getBlackJacked() == 1){
                    std::cout << "[playRound] Host has blackjack. Ending round immediately." << std::endl;
                }
                else {
                    uiManager.onActionChosen = [&](PlayerAction chosenAction){

                        std::cout << "[turnHandler] Player " << players[gameState.getCurrentPlayerId()].getId() << " chose action: " 
                                    << (chosenAction == PlayerAction::HIT ? 
                                        "HIT" : chosenAction == PlayerAction::STAND ? "STAND" : "SKILL_REQUEST") 
                                    << std::endl;

                        players[gameState.getCurrentPlayerId()].setPendingAction(chosenAction);
                    };
                    std::cout << "[playRound] EXITING BLACKJACK CHECK PHASE" << std::endl;
                }  
            }
        break;
        case PhaseName::PLAYER_HIT_PHASE:
            //TURN: HIT, STAND, SKILL
            if(!playerHitHandler()){
                //if playerHitHandler returns false -> all players have finished their turns -> move to host phase
                std::cout << "[playRound] EXITING PLAYER HIT PHASE" << std::endl;
                
                //set callback for host action input during host hit phase
                uiManager.onActionChosen = [&](PlayerAction chosenAction){

                        std::cout << "[turnHandler] Host " << getHostPlayer().getId() << " chose action: " 
                                    << (chosenAction == PlayerAction::HIT ? 
                                        "HIT" : chosenAction == PlayerAction::STAND ? "STAND" : "SKILL_REQUEST") 
                                    << std::endl;

                        getHostPlayer().setPendingAction(chosenAction);
                };
            }
        break;
        case PhaseName::HOST_HIT_PHASE:
            //HOST FIGHTS EACH PLAYER, BEFORE EVERY FIGHT HOST GETS A TURN (HIT, STAND, SKILL)
            if(!hostHandler()){
                //if hostHandler returns false -> host stands -> move to battle phase
                std::cout << "[playRound] EXITING HOST HIT PHASE" << std::endl;
            }
        break;
        case PhaseName::BATTLE_PHASE:
            //HOST BATTLES CURRENT PLAYER
            if( !battleHandler(getHostPlayer(), players[gameState.getCurrentPlayerId()])
                ){
                //if battleHandler returns true -> host wins -> end round immediately
                std::cout << "[playRound] EXITING BATTLE PHASE" << std::endl;
            }
        break;
        case PhaseName::ROUND_END:
            //ROUND END, CALCULATE LOSSES, PREPARE FOR NEXT ROUND
            updateGameState(PhaseName::ROUND_END,gameState.getCurrentPlayerId());
            round++;
            if (round == players.size()){
                std::cout << "[playRound] NO MORE ROUNDS, GAME OVER!" << std::endl;
                return false;
            }
            roundEndHandler();
        break;
    }
    return true;
}

// ========================================================================
// OLD PHASE SYSTEM
// ========================================================================

bool RoundManager::blackJackHandler(){

    //checks current player    
    Player& currentPlayer = players[gameState.getCurrentPlayerId()];

    updateGameState(PhaseName::BLACKJACK_CHECK_PHASE,currentPlayer.getId());

    if (currentPlayer.calculateHandValue() == 21 && currentPlayer.getHandSize() == 2){
        if (currentPlayer.getHost() == 1){
            std::cout << "Host player has Black Jack! Host player wins the round!" << std::endl;
            currentPlayer.gainPoint(players.size()-1);
            updateGameState(PhaseName::ROUND_END, currentPlayer.getId());
            return false;
        }
        else {
            std::cout << "Player " << currentPlayer.getId() << " has Black Jack! +2 points" << std::endl;
            currentPlayer.flipAllCardsFaceUp();
            currentPlayer.gainPoint(2);
            currentPlayer.blackJackSet();
        }
    }

    //increment player id for next check
    this->gameState.incrementCurrentPlayerId(players.size());

    //if all players have been checked, reset to first player and move to next phase
    if (gameState.getCurrentPlayerId() == -1){
        std::cout << "[blackJackHandler] All players checked for blackjack. Moving to PLAYER_HIT_PHASE." << std::endl;
        uiManager.onActionChosen = [&](PlayerAction chosenAction){

            std::cout << "[turnHandler] Player " << currentPlayer.getId() << " chose action: " 
                        << (chosenAction == PlayerAction::HIT ? 
                            "HIT" : chosenAction == PlayerAction::STAND ? "STAND" : "SKILL_REQUEST") 
                        << std::endl;

            currentPlayer.setPendingAction(chosenAction);
        };

        uiManager.requestActionInput(currentPlayer.getId());
        updateGameState(PhaseName::PLAYER_HIT_PHASE, 0);
        return false;
    }
    return true;
}

bool RoundManager::playerHitHandler(){
    
    //get current player
    int currentIndex = gameState.getCurrentPlayerId();

    Player& currentPlayer = players[currentIndex];

    //update player info in game state
    updateGameState(PhaseName::PLAYER_HIT_PHASE,currentPlayer.getId());

    // skip player that is not host or blackjacked
    if (currentPlayer.getHost() == 1 || currentPlayer.getBlackJacked() == 1){
        
        gameState.incrementCurrentPlayerId(players.size());
        //if all players have been checked, reset to first player and move to next phase
        if (gameState.getCurrentPlayerId() == -1){
            updateGameState(PhaseName::HOST_HIT_PHASE, 0);
            return false;
        }
    }
    // if turnHandler returns false -> player stands -> move to next player
    else if(!turnHandler(currentPlayer, currentPlayer)){
        std::cout << "[playerHitHandler] Player " << currentPlayer.getId() << " has finished their turn." << std::endl;
        
        //indexing next player
        gameState.incrementCurrentPlayerId(players.size());

        //if all players have been checked, reset to first player and move to next phase
        if (gameState.getCurrentPlayerId() == -1){
            std::cout << "[playerHitHandler] Moving to HOST_HIT_PHASE\n";
            updateGameState(PhaseName::HOST_HIT_PHASE, 0);
            return false;
        }
    }
    return true;

}

bool RoundManager::hostHandler(){

    //get host player and current player
    Player& hostPlayer = getHostPlayer();
    Player& currentPlayer = players[gameState.getCurrentPlayerId()];

    //update player info in game state
    updateGameState(PhaseName::HOST_HIT_PHASE,currentPlayer.getId());

    //prepare for battle, update game state, turn handler
    currentPlayer.flipAllCardsFaceUp();

    // handle player that is not host or blackjacked
    if (currentPlayer.getHost() == 1 || currentPlayer.getBlackJacked() == 1){
        gameState.incrementCurrentPlayerId(players.size());
        //if all players have been checked, reset to first player and move to next phase
        if (gameState.getCurrentPlayerId() == -1){
            updateGameState(PhaseName::ROUND_END, 0);
            return false;
        }
    }
    else if(!turnHandler(hostPlayer, currentPlayer)){
        updateGameState(PhaseName::BATTLE_PHASE, currentPlayer.getId());
        return false;
    }
    return true;

}

bool RoundManager::turnHandler(Player& player, Player& opponent){
    //polling action from player
    // if (player.getHost()){
    //     action =  player.hostTurnLogic(gameState, opponent, player.getLoss()/player.getBattleCount());
    // }
    // else {
    //     action = player.playerTurnLogic(gameState);
    // }


    if(player._isBot()){
       player.setPendingAction(player.botMode(gameState));
    }

    //if action is taken, resolve it
    switch (player.consumePendingAction()){
        case PlayerAction::HIT:
            player.addCardToHand(deck.draw());
            //the currentID of the gameState = player.getId() when normal hit phase, 
            //but it has to be equal to opponent.getId() when host hit phase
            updateGameState(
                player.getHost() ? PhaseName::HOST_HIT_PHASE : PhaseName::PLAYER_HIT_PHASE,
                player.getHost() ? opponent.getId() : player.getId()
            );
        break;

        case PlayerAction::SKILL_REQUEST:
            if(skillManager.getSkillUses(player.getSkillName()) > 0){

                skillHandler(player);

                updateGameState(
                    player.getHost() ? PhaseName::HOST_HIT_PHASE : PhaseName::PLAYER_HIT_PHASE,
                    player.getHost() ? opponent.getId() : player.getId()
                );
            }
            else{
                std::cout<< "[turnHandler] SKILL OUT OF USES" << std::endl;
            }
        break;

        case PlayerAction::STAND:
            updateGameState(
                player.getHost() ? PhaseName::HOST_HIT_PHASE : PhaseName::PLAYER_HIT_PHASE,
                player.getHost() ? opponent.getId() : player.getId()
            );
            return false;   
        break;

        case PlayerAction::IDLE:
        break;
    }
    
    return true;
}

bool RoundManager::battleHandler(Player& host, Player& opponent){

    int opponentHandValue = opponent.calculateHandValue();
    int hostHandValue = host.calculateHandValue() ;

    std::cout << "[battleHandler] HOST [" << hostHandValue
                << "] vs Player " << opponent.getId() 
                <<" [" << opponentHandValue << "]" << std::endl;
    //start battle, reprint hand value after host's turn since manipulation might
    //have happenned here
    updateGameState(PhaseName::BATTLE_PHASE,opponent.getId());
    host.flipAllCardsFaceUp();
    opponentHandValue = opponent.calculateHandValue();
    hostHandValue = host.calculateHandValue() ;
    std::cout << "[battleHandler] HOST [" << hostHandValue
                << "] vs Player " << opponent.getId() <<" [" << opponentHandValue <<"]"<< std::endl;
    
    //determine outcome
    if (hostHandValue > GameConfig::BLACKJACK_VALUE){
        std::cout << "[battleHandler] Host BUSTS" << std::endl;
        host.gainLoss();
        opponent.gainPoint();
    }
    else if (opponentHandValue > GameConfig::BLACKJACK_VALUE){
        std::cout << "[battleHandler] Player " << opponent.getId() << " BUSTS!" << std::endl;
        host.gainPoint();
        opponent.gainLoss();
    }
    else if (hostHandValue > opponentHandValue){
        std::cout << "[battleHandler] Host WINS " << std::endl;
        host.gainPoint();
        opponent.gainLoss();
    }
    else if (hostHandValue < opponentHandValue){
        std::cout << "[battleHandler] Player " << opponent.getId() << " WINS" << std::endl;
        opponent.gainPoint();
        host.gainLoss();
    }
    else {
        host.gainLoss();
        std::cout << "[battleHandler] TIE " << opponent.getId() << std::endl;
    }
    host.gainBattleCount();
    opponent.gainBattleCount();


    //after battle, indexing next player
    gameState.incrementCurrentPlayerId(players.size());

    if (gameState.getCurrentPlayerId() == -1){
        std::cout<< "[battleHandler] All battles resolved for round " << round << "." << std::endl;
        std::cout << "Moving to ROUND_END phase." << std::endl;
        updateGameState(PhaseName::ROUND_END, 0);
        return false; 
    }

    updateGameState(PhaseName::HOST_HIT_PHASE, gameState.getCurrentPlayerId());
    return true;
}

void RoundManager::skillHandler(Player& player){
    
    //PlayerTargeting targets = player.targetAndConfirm(gameState);
    // struct PlayerTargeting{
    //     std::vector<int> targetPLayerIds;
    //     std::vector<Card*> targetCards;
    // };

    
    uiManager.requestTargetInput(player.getId());
    uiManager.onTargetChosen = [&](PlayerTargeting chosenTarget){
        gameState.pendingPlayerTargeting = chosenTarget;
    };
    //grab ACTUAL players from the given ids.
    std::vector<Player*> actualTargets;

    for (int id : gameState.pendingPlayerTargeting.targetPLayerIds) {
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
        gameState.pendingPlayerTargeting.targetCards,
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
    players[hostPlayer.getId() + 1].setHost();

    std::cout << "[roundEndHandler] Player " 
    << players[hostPlayer.getId() + 1].getId() 
    << " is the new host." << std::endl;

    hostPlayer.resetHost();

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
    //deck.printDeck();

    //redeal cards
    deck.shuffle();
    dealCardsToPlayers(2);

    updateGameState(PhaseName::BLACKJACK_CHECK_PHASE, 0);
}

// ========================================================================
// Helper functions
// ========================================================================
void RoundManager::updateGameState(PhaseName phase, int playerId){
    // std::cout<< "[updateGameState] - "<< phaseToString() 
    //             << " Player Id: "<< playerId << std::endl;
                
    gameState.setPhaseName(phase, playerId);
    gameState.setDeckCount(deck.getSize());
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
    gameState.setAllPlayerInfo(allPlayerInfos);
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
        switch(gameState.getPhaseName()){   
            case PhaseName::BATTLE_PHASE:
                return "BATTLE PHASE";
            break;
            case PhaseName::BLACKJACK_CHECK_PHASE:
                return "BLACKJACK CHECK PHASE";
            break;
            case PhaseName::ROUND_END:
                return "ROUND END";
            break;
            case PhaseName::HOST_HIT_PHASE:
                return "HOST HIT PHASE";
            break;
            case PhaseName::PLAYER_HIT_PHASE:
                return "PLAYER HIT PHASE ";
            break;
            default:
                return "UNKNOWN PHASE";          
        }
    }


// ========================================================================
// NEW PHASE SYSTEM
// ========================================================================

Player& RoundManager::getPlayerById(int id){
    for (auto& player : players){
        if (player.getId() == id)
            return player;
    }
    throw std::runtime_error("Player with ID " + std::to_string(id) + " not found.");
}


void RoundManager::update(){

    auto next = currentPhase->onUpdate();

    if (next)
        changePhase(*next);
        
    else if (next == std::nullopt && gameState.getPhaseName() == PhaseName::GAME_OVER){
        std::cout << "Game Over! Thanks for playing!" << std::endl;
    }
}

void RoundManager::changePhase(PhaseName newPhase){
    //call onExit of CURRENT phase before changing
    if (currentPhase)
        currentPhase->onExit();

    //change phase
    currentPhase = createPhase(newPhase);

    //call onEnter of NEW phase after changing
    if (currentPhase)
        currentPhase->onEnter();
}

std::unique_ptr<Phase> RoundManager::createPhase(const PhaseName name){
    if (name == PhaseName::BLACKJACK_CHECK_PHASE)
        return std::make_unique<BlackJackCheckPhase>(uiManager, *this);
    if (name == PhaseName::PLAYER_HIT_PHASE)
        return std::make_unique<PlayerHitPhase>(uiManager, *this);
    if (name == PhaseName::HOST_HIT_PHASE)
        return std::make_unique<HostHitPhase>(uiManager, *this);
    if (name == PhaseName::BATTLE_PHASE)
        return std::make_unique<BattlePhase>(uiManager, *this);
    if (name == PhaseName::ROUND_END)
        return std::make_unique<RoundEndPhase>(uiManager, *this);
    if (name == PhaseName::GAME_OVER){
        return nullptr;
    }

    return nullptr;
}