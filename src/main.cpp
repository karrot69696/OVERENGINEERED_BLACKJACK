#include <iostream>
#include <string>
#include "gameEngine/Game.h"
#include "networking/NetMessage.h"

int main() {
    std::cout << "Time to cook!" << std::endl;
    std::cout << "\n=== CrazyJack ===\n" << std::endl;
    std::cout << "1. Local Game" << std::endl;
    std::cout << "2. Host Online Game" << std::endl;
    std::cout << "3. Join Online Game" << std::endl;
    std::cout << "Choice: ";

    int choice = 1;
    std::cin >> choice;

    NetworkMode mode = NetworkMode::LOCAL;
    std::string serverIP = "127.0.0.1";
    uint16_t port = NET_DEFAULT_PORT;

    if (choice == 2) {
        mode = NetworkMode::HOST;
        std::cout << "Port (default " << NET_DEFAULT_PORT << "): ";
        std::string portStr;
        std::cin.ignore();
        std::getline(std::cin, portStr);
        if (!portStr.empty()) port = static_cast<uint16_t>(std::stoi(portStr));
    } else if (choice == 3) {
        mode = NetworkMode::CLIENT;
        std::cout << "Server IP: " << "127.0.0.1" << std::endl;
        //std::cin >> serverIP;
        std::cout << "Port (default " << NET_DEFAULT_PORT << "): ";
        std::string portStr;
        std::cin.ignore();
        std::getline(std::cin, portStr);
        if (!portStr.empty()) port = static_cast<uint16_t>(std::stoi(portStr));
    }

    sf::RenderWindow window(sf::VideoMode({GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT}), "CrazyJack");
    window.setFramerateLimit(60);

    Game newGame(window, mode);

    if (mode == NetworkMode::HOST) {
        if (!newGame.startServer(port)) {
            std::cerr << "Failed to start server!" << std::endl;
            return 1;
        }
        // Start with host as human + 2 bots (remote players join as additional humans)
        newGame.SetupGame(1, 2);
    } else if (mode == NetworkMode::CLIENT) {
        if (!newGame.connectToServer(serverIP, port)) {
            std::cerr << "Failed to connect to server!" << std::endl;
            return 1;
        }
        // Client: local setup is placeholder, server state will overwrite
        newGame.SetupGame(1, 2);
    } else {
        newGame.SetupGame();
    }

    newGame.RunGame();
    return 0;
}
