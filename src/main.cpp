#include <iostream>
#include "gameEngine/Game.h"
int main() {
    std::cout << "Time to cook!" << std::endl;
    Game newGame= Game();
    newGame.SetupGame();
    newGame.RunGame();
    return 0;
}