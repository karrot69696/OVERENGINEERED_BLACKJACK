#include <iostream>
#include "gameEngine/Game.h"

int main() {
    std::cout << "Time to cook!" << std::endl;
    Game newGame= Game();
    newGame.SetupGame();
    newGame.RunGame();
    return 0;
}
// int main() {
//     sf::RenderWindow window(sf::VideoMode({800,600}),"Title");
//     while (window.isOpen()){
//         while (std::optional event = window.pollEvent()){
        
//         }

//         window.clear(sf::Color(127,127,127));
//         window.display();
//     }
//     return 0;
// }