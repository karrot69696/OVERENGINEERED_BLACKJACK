#include <iostream>
#include <string>
#include "gameEngine/Game.h"
#include "networking/NetMessage.h"

// Simple menu button
struct MenuButton {
    sf::RectangleShape shape;
    sf::Text label;
    bool hovered = false;

    MenuButton(sf::Font& font, const std::string& text, sf::Vector2f pos, sf::Vector2f size)
        : label(font, text, 12) {
        shape.setPosition(pos);
        shape.setSize(size);
        shape.setFillColor(sf::Color(60, 60, 80));
        shape.setOutlineThickness(2.f);
        shape.setOutlineColor(sf::Color::White);

        sf::FloatRect bounds = label.getLocalBounds();
        label.setPosition({
            pos.x + size.x / 2.f - bounds.size.x / 2.f,
            pos.y + size.y / 2.f - bounds.size.y / 2.f - 4.f
        });
    }

    bool contains(sf::Vector2f mousePos) {
        return shape.getGlobalBounds().contains(mousePos);
    }

    void draw(sf::RenderWindow& window) {
        shape.setFillColor(hovered ? sf::Color(90, 90, 120) : sf::Color(60, 60, 80));
        window.draw(shape);
        window.draw(label);
    }
};

int main() {
    sf::RenderWindow window(sf::VideoMode({GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT}), "CrazyJack",
        sf::Style::Titlebar | sf::Style::Close);
    window.setFramerateLimit(60);

    sf::Font font;
    if (!font.openFromFile("../assets/fonts/PixeloidSans.ttf")) {
        std::cerr << "Failed to load menu font" << std::endl;
        return 1;
    }

    // Title
    sf::Text title(font, "CrazyJack", 36);
    title.setFillColor(sf::Color(245, 224, 32));
    sf::FloatRect titleBounds = title.getLocalBounds();

    title.setPosition({
        window.getSize().x / 2.f - titleBounds.size.x / 2.f,
        40.f
    });

    // Subtitle
    sf::Text subtitle(font, "Over-engineered Blackjack", 14);
    subtitle.setFillColor(sf::Color(180, 180, 180));
    sf::FloatRect subBounds = subtitle.getLocalBounds();
    subtitle.setPosition({
        window.getSize().x / 2.f - subBounds.size.x / 2.f,
        90.f
    });

    // Buttons — fit the small window
    sf::Vector2f btnSize = {200.f, 35.f};
    float btnX = window.getSize().x / 2.f - btnSize.x / 2.f;
    float btnStartY = window.getSize().y * 0.55f;
    float btnGap = 45.f;

    MenuButton btnLocal(font, "Local Game", {btnX, btnStartY}, btnSize);
    MenuButton btnHost(font, "Host Online Game", {btnX, btnStartY + btnGap}, btnSize);
    MenuButton btnJoin(font, "Join Online Game", {btnX, btnStartY + btnGap * 2}, btnSize);

    // Connection info text
    sf::Text infoText(font, "IP: 127.0.0.1  Port: " + std::to_string(NET_DEFAULT_PORT), 10);
    infoText.setFillColor(sf::Color(150, 150, 150));
    sf::FloatRect infoBounds = infoText.getLocalBounds();
    infoText.setPosition({
        window.getSize().x / 2.f - infoBounds.size.x / 2.f,
        btnStartY - infoBounds.size.y - 10.f
    });

    NetworkMode mode = NetworkMode::LOCAL;
    bool menuDone = false;

    // Menu loop
    while (window.isOpen() && !menuDone) {
        while (std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
                return 0;
            }
            if (const auto* mouseMoved = event->getIf<sf::Event::MouseMoved>()) {
                //sf::Vector2f mp = {(float)mouseMoved->position.x, (float)mouseMoved->position.y};
                sf::Vector2f mp = window.mapPixelToCoords({mouseMoved->position.x, mouseMoved->position.y});
                btnLocal.hovered = btnLocal.contains(mp);
                btnHost.hovered = btnHost.contains(mp);
                btnJoin.hovered = btnJoin.contains(mp);
            }
            if (const auto* mouseBtn = event->getIf<sf::Event::MouseButtonPressed>()) {
                
                if (mouseBtn->button == sf::Mouse::Button::Left) {
                    sf::Vector2f mp = window.mapPixelToCoords({mouseBtn->position.x, mouseBtn->position.y});
                    if (btnLocal.contains(mp)) { mode = NetworkMode::LOCAL; menuDone = true; }
                    if (btnHost.contains(mp))  { mode = NetworkMode::HOST;  menuDone = true; }
                    if (btnJoin.contains(mp))  { mode = NetworkMode::CLIENT; menuDone = true; }
                }
            }
        }

        window.clear(sf::Color(20, 30, 20));
        window.draw(title);
        window.draw(subtitle);
        btnLocal.draw(window);
        btnHost.draw(window);
        btnJoin.draw(window);
        window.draw(infoText);
        window.display();
    }

    if (!window.isOpen()) return 0;

    // Game setup
    std::string serverIP = "127.0.0.1";
    uint16_t port = NET_DEFAULT_PORT;

    Game newGame(window, mode);

    if (mode == NetworkMode::HOST) {
        if (!newGame.startServer(port)) {
            std::cerr << "Failed to start server!" << std::endl;
            return 1;
        }

        // ── Host lobby: wait for players, then start ──
        int connectedCount = 0;
        bool lobbyDone = false;

        newGame.getNetworkManager().onClientConnected = [&](int playerId) {
            connectedCount++;
            std::cout << "[Lobby] Player " << playerId << " joined! ("
                      << connectedCount << " connected)" << std::endl;
        };
        newGame.getNetworkManager().onClientDisconnected = [&](int playerId) {
            connectedCount--;
            std::cout << "[Lobby] Player " << playerId << " left! ("
                      << connectedCount << " connected)" << std::endl;
        };

        // Lobby UI elements
        sf::Text lobbyTitle(font, "Host Lobby", 36);
        lobbyTitle.setFillColor(sf::Color(245, 224, 32));
        sf::FloatRect ltBounds = lobbyTitle.getLocalBounds();
        lobbyTitle.setPosition({
            window.getSize().x / 2.f - ltBounds.size.x / 2.f,
            40.f
        });

        sf::Text portText(font, "Port: " + std::to_string(port), 14);
        portText.setFillColor(sf::Color(180, 180, 180));
        sf::FloatRect ptBounds = portText.getLocalBounds();
        portText.setPosition({
            window.getSize().x / 2.f - ptBounds.size.x / 2.f,
            90.f
        });

        sf::Text waitingText(font, "Waiting for players...", 14);
        waitingText.setFillColor(sf::Color(200, 200, 200));

        sf::Text playerCountText(font, "Players connected: 0", 14);
        playerCountText.setFillColor(sf::Color(150, 220, 150));

        MenuButton btnStart(font, "Start Game", {btnX, window.getSize().y * 0.55f + btnGap * 2}, btnSize);

        while (window.isOpen() && !lobbyDone) {
            while (std::optional event = window.pollEvent()) {
                if (event->is<sf::Event::Closed>()) {
                    window.close();
                    return 0;
                }
                if (const auto* mouseMoved = event->getIf<sf::Event::MouseMoved>()) {
                    sf::Vector2f mp = window.mapPixelToCoords({mouseMoved->position.x, mouseMoved->position.y});
                    btnStart.hovered = (connectedCount > 0) && btnStart.contains(mp);
                }
                if (const auto* mouseBtn = event->getIf<sf::Event::MouseButtonPressed>()) {
                    if (mouseBtn->button == sf::Mouse::Button::Left && connectedCount > 0) {
                        sf::Vector2f mp = window.mapPixelToCoords({mouseBtn->position.x, mouseBtn->position.y});
                        if (btnStart.contains(mp)) {
                            lobbyDone = true;
                        }
                    }
                }
            }

            // Poll network to accept incoming connections
            newGame.getNetworkManager().poll();

            // Update dynamic text
            playerCountText.setString("Players connected: " + std::to_string(connectedCount));
            sf::FloatRect pcBounds = playerCountText.getLocalBounds();
            playerCountText.setPosition({
                window.getSize().x / 2.f - pcBounds.size.x / 2.f,
                window.getSize().y * 0.55f + btnGap
            });

            if (connectedCount == 0) {
                waitingText.setString("Waiting for players...");
                waitingText.setFillColor(sf::Color(200, 200, 200));
            } else {
                waitingText.setString("Ready to start!");
                waitingText.setFillColor(sf::Color(100, 255, 100));
            }
            sf::FloatRect wtBounds = waitingText.getLocalBounds();
            waitingText.setPosition({
                window.getSize().x / 2.f - wtBounds.size.x / 2.f,
                window.getSize().y * 0.55f
            });

            // Render
            window.clear(sf::Color(20, 30, 20));
            window.draw(lobbyTitle);
            window.draw(portText);
            window.draw(waitingText);
            window.draw(playerCountText);

            if (connectedCount > 0) {
                btnStart.draw(window);
            } else {
                // Draw dimmed button
                btnStart.shape.setFillColor(sf::Color(40, 40, 50));
                btnStart.label.setFillColor(sf::Color(100, 100, 100));
                window.draw(btnStart.shape);
                window.draw(btnStart.label);
                btnStart.label.setFillColor(sf::Color::White);
            }

            window.display();
        }

        if (!window.isOpen()) return 0;

        // Tell all clients the game is starting
        newGame.getNetworkManager().broadcastGameStart();

        // Setup: 1 local human (host) + connectedCount remote players + 1 bot
        newGame.SetupGame(1, connectedCount, 1);

    } else if (mode == NetworkMode::CLIENT) {
        if (!newGame.connectToServer(serverIP, port)) {
            std::cerr << "Failed to connect to server!" << std::endl;
            return 1;
        }

        // ── Client lobby: wait for host to start the game ──
        sf::Text clientWaitTitle(font, "Connected!", 36);
        clientWaitTitle.setFillColor(sf::Color(245, 224, 32));
        sf::FloatRect cwBounds = clientWaitTitle.getLocalBounds();
        clientWaitTitle.setPosition({
            window.getSize().x / 2.f - cwBounds.size.x / 2.f,
            40.f
        });

        sf::Text clientWaitSub(font, "Waiting for host to start...", 14);
        clientWaitSub.setFillColor(sf::Color(200, 200, 200));
        sf::FloatRect csBounds = clientWaitSub.getLocalBounds();
        clientWaitSub.setPosition({
            window.getSize().x / 2.f - csBounds.size.x / 2.f,
            90.f
        });

        while (window.isOpen() && !newGame.getNetworkManager().hasGameStarted()) {
            while (std::optional event = window.pollEvent()) {
                if (event->is<sf::Event::Closed>()) {
                    window.close();
                    return 0;
                }
            }
            newGame.getNetworkManager().poll();

            window.clear(sf::Color(20, 30, 20));
            window.draw(clientWaitTitle);
            window.draw(clientWaitSub);
            window.display();
        }

        if (!window.isOpen()) return 0;

        newGame.SetupGame(1, 2);
    } else {
        newGame.SetupGame();
    }

    newGame.RunGame();
    return 0;
}
