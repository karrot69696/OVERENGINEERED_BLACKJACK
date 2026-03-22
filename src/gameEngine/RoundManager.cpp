#include "RoundManager.h"
#include "Game.h"
    RoundManager::RoundManager(
        std::vector<Player>& _players,
        Deck& _deck,
        GameState& _gameState, VisualState& _visualState,
        UIManager& _uiManager,
        EventQueue& _eventQueue,
        NetworkManager* _networkManager)
        :
        players(_players),
        deck(_deck), skillManager(),
        gameState(_gameState), visualState(_visualState),
        uiManager(_uiManager),
        eventQueue(_eventQueue),
        networkManager(_networkManager) {}
// ========================================================================
// Helper functions
// ========================================================================
void RoundManager::updateGameState(PhaseName phase, int playerId){
    // std::cout<< "[updateGameState] - "<< phaseToString() 
    //             << " Player Id: "<< playerId << std::endl;
                
    gameState.setPhaseName(phase, playerId);
    // Snapshot deck cards into GameState
    std::vector<Card> deckSnapshot;
    for (auto* card : deck.getCards()) {
        deckSnapshot.push_back(*card);
    }
    gameState.setDeckCards(deckSnapshot);
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
            playerHand.push_back(*player.getCardInHand(i));
        }
        
        PlayerInfo playerInfo{
            player.getId(),
            playerHand,
            player.getSkillName(),
            skillManager.getSkillUses(player.getSkillName()),
            player.getPoint(),
            player.calculateHandValue(),
            player._isBot(),
            (bool)player.getHost(),
            player.isRemote
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

Player& RoundManager::getPlayerById(int id){
    for (auto& player : players){
        if (player.getId() == id)
            return player;
    }
    throw std::runtime_error("Player with ID " + std::to_string(id) + " not found.");
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
// PHASE SYSTEM
// ========================================================================

void RoundManager::update(){
    
    //check for game end | invalid currentPhase
    if(gameState.getPhaseName() == PhaseName::GAME_OVER){
        return;
    }
    else if (!currentPhase){
        std::cout << "[RoundManager] No current phase set. Cannot update." << std::endl;
        return;
    }

    //game loop
    auto next = currentPhase->onUpdate();
    
    if (next)
        changePhase(*next);
}

void RoundManager::changePhase(PhaseName newPhase){
    //call onExit of CURRENT phase before changing
    if (currentPhase)
        currentPhase->onExit();

    // Flush stale remote actions/targets from the previous turn
    if (networkManager)
        networkManager->clearAllRemoteInputs();

    //change phase
    currentPhase = createPhase(newPhase);

    //call onEnter of NEW phase after changing
    if (currentPhase)
        currentPhase->onEnter();
}

std::unique_ptr<Phase> RoundManager::createPhase(const PhaseName name){
    switch (name){
        case PhaseName::GAME_START_PHASE:
            return std::make_unique<GameStartPhase>(this->uiManager, this->eventQueue, *this, this->skillManager, this->gameState, this->visualState);
        case PhaseName::BLACKJACK_CHECK_PHASE:
            return std::make_unique<BlackJackCheckPhase>(this->uiManager, this->eventQueue, *this, this->skillManager, this->gameState, this->visualState);
        case PhaseName::PLAYER_HIT_PHASE:
            return std::make_unique<PlayerHitPhase>(this->uiManager, this->eventQueue, *this, this->skillManager, this->gameState, this->visualState);
        case PhaseName::HOST_HIT_PHASE:
            return std::make_unique<HostHitPhase>(this->uiManager, this->eventQueue, *this, this->skillManager, this->gameState, this->visualState);
        case PhaseName::BATTLE_PHASE:
            return std::make_unique<BattlePhase>(this->uiManager, this->eventQueue, *this, this->skillManager, this->gameState, this->visualState);
        case PhaseName::ROUND_END:
            return std::make_unique<RoundEndPhase>(this->uiManager, this->eventQueue, *this, this->skillManager, this->gameState, this->visualState);
        case PhaseName::GAME_OVER:
            return nullptr;
    }
    return nullptr;
}