#include <iostream>
#include "gameEngine/Game.h"

int main() {
    std::cout << "Time to cook!" << std::endl;
    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    sf::RenderWindow window(desktop, "Game", sf::State::Fullscreen);
    window.setFramerateLimit(60);
    Game newGame(window);
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