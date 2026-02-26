#include "Phase.h"
#include "../gameEngine/Game.h"

Player& Phase::getCurrentPlayer(){
    //grabs current player    
    int currentPlayerIndex = roundManager.getGameState().getCurrentPlayerId();
    std::vector<Player>& players = roundManager.getPlayers();
    Player& currentPlayer = players[currentPlayerIndex];
    return currentPlayer;
}
int Phase::getCurrentPlayerId(){
    return roundManager.getGameState().getCurrentPlayerId();
}
void Phase::incrementCurrentPlayerId(){
    roundManager.getGameState().incrementCurrentPlayerId(roundManager.getPlayers().size());
}

bool Phase::turnHandler(Player& player, Player& opponent){
    
    if(player._isBot()){
       player.setPendingAction(player.botMode(roundManager.getGameState()));
    }

    //if action is taken, resolve it
    switch (player.getPendingAction()){
        case PlayerAction::HIT:
            player.addCardToHand(roundManager.getDeck().draw());
            //the currentID of the gameState = player.getId() when normal hit phase, 
            //but it has to be equal to opponent.getId() when host hit phase
            roundManager.updateGameState(
                player.getHost() ? PhaseName::HOST_HIT_PHASE : PhaseName::PLAYER_HIT_PHASE,
                player.getHost() ? opponent.getId() : player.getId()
            );
            player.setPendingAction(PlayerAction::IDLE);
        break;

        case PlayerAction::SKILL_REQUEST:
            if(skillManager.getSkillUses(player.getSkillName()) > 0){

                roundManager.skillHandler(player);

                roundManager.updateGameState(
                    player.getHost() ? PhaseName::HOST_HIT_PHASE : PhaseName::PLAYER_HIT_PHASE,
                    player.getHost() ? opponent.getId() : player.getId()
                );
            }
            else{
                std::cout<< "[turnHandler] SKILL OUT OF USES" << std::endl;
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
