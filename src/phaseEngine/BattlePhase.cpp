#include "BattlePhase.h"
#include "../gameEngine/Game.h"

void BattlePhase::onEnter() {
    std::cout << "\n=== ENTERING BATTLE PHASE ===\n" << std::endl;

    Player& currentPlayer = getCurrentPlayer();
    std::string battleText =
    "HOST " + std::to_string(roundManager.getHostPlayer().getId()) +
    " VS PLAYER " + std::to_string(currentPlayer.getId());
    eventQueue.push({
        GameEventType::PHASE_ANNOUNCED,
        PhaseAnnouncedEvent{
            battleText,
            AnimConfig::PHASE_TEXT_DURATION
        }});
    roundManager.updateGameState(PhaseName::BATTLE_PHASE,currentPlayer.getId());
}


std::optional<PhaseName> BattlePhase::onUpdate(){
    //get host player and current player
    Player& host = roundManager.getHostPlayer();
    Player& opponent = getCurrentPlayer();

    if (host.getId() == opponent.getId()) {
        throw std::runtime_error("Host player cannot battle themselves.");
    }
    //calculate hand values
    int opponentHandValue = opponent.calculateHandValue();
    int hostHandValue = host.calculateHandValue() ;

    std::cout << "[BattlePhase] HOST [" << hostHandValue
                << "] vs Player " << opponent.getId() 
                <<" [" << opponentHandValue << "]" << std::endl;
    //start battle, reprint hand value after host's turn since manipulation might
    //have happenned here
    std::vector<int> revealIds;
    for (int i = 0; i < host.getHandSize(); i++) {
        Card* c = host.getCardInHand(i);
        if (!c->isFaceUp()) revealIds.push_back(c->getId());
    }
    for (int i = 0; i < opponent.getHandSize(); i++) {
        Card* c = opponent.getCardInHand(i);
        if (!c->isFaceUp()) revealIds.push_back(c->getId());
    }
    host.flipAllCardsFaceUp();
    opponent.flipAllCardsFaceUp();
    roundManager.updateGameState(PhaseName::BATTLE_PHASE, opponent.getId());
    if (!revealIds.empty()) {
        eventQueue.push({GameEventType::CARDS_REVEALED, CardsRevealedEvent{revealIds}});
        eventQueue.push({GameEventType::REQUEST_ACTION_INPUT,
                        RequestActionInputEvent{host.getId()}});
    }
    opponentHandValue = opponent.calculateHandValue();
    hostHandValue = host.calculateHandValue() ;
    std::cout << "[BattlePhase] HOST [" << hostHandValue
                << "] vs Player " << opponent.getId() <<" [" << opponentHandValue <<"]"<< std::endl;

    //helper: award win to winner, loss to loser, emit events
    auto resolveWin = [&](Player& winner, Player& loser, const std::string& msg){
        std::cout << "[BattlePhase] " << msg << std::endl;
        winner.gainPoint(GameConfig::POINTS_GAIN_WON);
        loser.gainLoss();
        eventQueue.push({GameEventType::POINT_CHANGED, PointChangedEvent{
            winner.getId(), "+"+ std::to_string(GameConfig::POINTS_GAIN_WON)+ "POINT"
        }});
        eventQueue.push({GameEventType::SHOCK_EFFECT, ShockEffectEvent{winner.getId(), loser.getId(), 1.2f}});
        eventQueue.push({GameEventType::EXPLOSION_EFFECT, ExplosionEffectEvent{loser.getId(), 3.0f, 1.2f}});
    };

    //determine outcome
    bool hostBust = hostHandValue > GameConfig::BLACKJACK_VALUE;
    bool oppBust  = opponentHandValue > GameConfig::BLACKJACK_VALUE;

    if (hostBust) {
        resolveWin(opponent, host, "Host BUSTS");
    } else if (oppBust) {
        resolveWin(host, opponent, "Player " + std::to_string(opponent.getId()) + " BUSTS!");
    } else if (hostHandValue > opponentHandValue) {
        resolveWin(host, opponent, "Host WINS");
    } else if (hostHandValue < opponentHandValue) {
        resolveWin(opponent, host, "Player " + std::to_string(opponent.getId()) + " WINS");
    } else {
        std::cout << "[BattlePhase] TIE " << opponent.getId() << std::endl;
        host.gainPoint(GameConfig::POINTS_GAIN_TIE);
        opponent.gainPoint(GameConfig::POINTS_GAIN_TIE);
        eventQueue.push({GameEventType::POINT_CHANGED, PointChangedEvent{opponent.getId(), "+"+std::to_string(GameConfig::POINTS_GAIN_TIE)}});
        eventQueue.push({GameEventType::POINT_CHANGED, PointChangedEvent{host.getId(), "+"+std::to_string(GameConfig::POINTS_GAIN_TIE)}});
    }
    host.gainBattleCount();
    opponent.gainBattleCount();


    //after battle, indexing next player
    incrementCurrentPlayerId();

    if (allPlayersProcessed()){
        std::cout<< "[BattlePhase] All battles resolved for round " << round << "." << std::endl;
        std::cout << "Moving to ROUND_END phase." << std::endl;
        roundManager.updateGameState(PhaseName::ROUND_END, 0);
        return PhaseName::ROUND_END; 
    }

    //go back to host hit after battle
    roundManager.updateGameState(PhaseName::HOST_HIT_PHASE, getCurrentPlayer().getId());
    return PhaseName::HOST_HIT_PHASE;
}


void BattlePhase::onExit() {
    std::cout << "\n=== EXITING BATTLE PHASE ===\n" << std::endl;
}