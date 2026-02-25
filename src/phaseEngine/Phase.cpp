#include "Phase.h"
#include "../gameEngine/RoundManager.h"
#include "../gameEngine/UIManager.h"

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
