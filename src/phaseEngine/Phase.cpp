#include "Phase.h"
#include "../gameEngine/Game.h"

Phase::Phase(UIManager& uiManager,
             RoundManager& roundManager,
             SkillManager& skillManager,
             GameState& gameState)
    : uiManager(uiManager),
      roundManager(roundManager),
      skillManager(skillManager),
      gameState(gameState),
      deck(roundManager.getDeck()),
      players(roundManager.getPlayers())
{}

Player& Phase::getCurrentPlayer(){
    //grabs current player    
    int currentPlayerIndex = gameState.getCurrentPlayerId();
    Player& currentPlayer = players[currentPlayerIndex];
    return currentPlayer;
}
int Phase::getCurrentPlayerId(){
    return gameState.getCurrentPlayerId();
}
void Phase::incrementCurrentPlayerId(){
    gameState.incrementCurrentPlayerId(players.size());
}

bool Phase::turnHandler(Player& player, Player& opponent){
    
    if(player._isBot()){
       player.setPendingAction(player.botMode(gameState));
    }

    //if action is taken, resolve it
    switch (player.getPendingAction()){
        case PlayerAction::HIT:
            player.addCardToHand(deck.draw());
            //the currentID of the gameState = player.getId() when normal hit phase, 
            //but it has to be equal to opponent.getId() when host hit phase
            roundManager.updateGameState(
                player.getHost() ? PhaseName::HOST_HIT_PHASE : PhaseName::PLAYER_HIT_PHASE,
                player.getHost() ? opponent.getId() : player.getId()
            );
            player.setPendingAction(PlayerAction::IDLE);
        break;

        case PlayerAction::SKILL_REQUEST:
        
            //wait until target exists, then process skill in skillHandler
            if(gameState.pendingTarget.targetPlayerIds.size() > 0 || 
            gameState.pendingTarget.targetCards.size() > 0){
                std::cout << "[Phase][turnHandler] Processing skill for player " << player.getId() << std::endl;
                skillHandler(player);

                roundManager.updateGameState(
                    player.getHost() ? PhaseName::HOST_HIT_PHASE : PhaseName::PLAYER_HIT_PHASE,
                    player.getHost() ? opponent.getId() : player.getId()
                );
            }
        break;

        case PlayerAction::STAND:
            roundManager.updateGameState(
                player.getHost() ? PhaseName::HOST_HIT_PHASE : PhaseName::PLAYER_HIT_PHASE,
                player.getHost() ? opponent.getId() : player.getId()
            );
            player.setPendingAction(PlayerAction::IDLE);
            return false;   
        break;

        case PlayerAction::IDLE:
        break;
    }
    
    return true;
}

void Phase::skillHandler(Player& player){
    
    //PlayerTargeting targets = player.targetAndConfirm(gameState);
    // struct PlayerTargeting{
    //     std::vector<int> targetPlayerIds;
    //     std::vector<Card*> targetCards;
    // };

    //grab ACTUAL players and cards from the given ids.
    std::vector<Player*> actualTargets;
    std::vector<Card*> actualTargetCards;

    for (int id : gameState.pendingTarget.targetPlayerIds) {
        for (auto& p : players) {
            if (p.getId() == id) {
                actualTargets.push_back(&p); //*pointerToObject = &object
            }
        }
    }

    for (Card card : gameState.pendingTarget.targetCards) {
        for (auto& p : players) {
            for (int i = 0; i < p.getHandSize(); i++) {
                Card* c = p.getCardAddr(i);
                if (c->getSuit() == card.getSuit() && c->getRank() == card.getRank()) {
                    std::cout << "[skillHandler] Found target card: " << c->getRankAsString() << c->getSuitAsString() << " in player " << p.getId() << "'s hand." << std::endl;
                    actualTargetCards.push_back(c); 
                }
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
        actualTargetCards,
        deck,
        gameState
    };

    if(!skillManager.processSkill(context)){
        std::cout << "[skillHandler] Skill processing failed for player " << player.getId() << std::endl;
    }

    gameState.pendingTarget = {}; //reset pending target after processing skill
    uiManager.requestActionInput(player.getId());
    player.setPendingAction(PlayerAction::IDLE);
}