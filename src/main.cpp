#include <iostream>
#include <string>
#include "gameEngine/Game.h"
#include "gameEngine/Log.h"
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
    // Install timestamped logging — every std::cout line gets a [sec.ms] prefix
    TimestampBuf tsBuf(std::cout.rdbuf());
    std::cout.rdbuf(&tsBuf);

    // Start at 90% of desktop size (windowed)
    auto desktop = sf::VideoMode::getDesktopMode();
    unsigned int winW = static_cast<unsigned int>(desktop.size.x * 0.9f);
    unsigned int winH = static_cast<unsigned int>(desktop.size.y * 0.9f);
    sf::RenderWindow window(sf::VideoMode({winW, winH}), "CrazyJack",
        sf::Style::Titlebar | sf::Style::Close);
    window.setFramerateLimit(60);
    GameSettings::instance().recalcScale(winW);

    sf::Font font;
    if (!font.openFromFile("assets/fonts/PixeloidSans.ttf")) {
        std::cerr << "Failed to load menu font" << std::endl;
        return 1;
    }

    // Helper: apply window size from settings
    auto applyWindowSize = [&]() {
        auto& settings = GameSettings::instance();
        auto dt = sf::VideoMode::getDesktopMode();
        if (settings.smallWindow) {
            window.create(sf::VideoMode({GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT}),
                "CrazyJack", sf::Style::Titlebar | sf::Style::Close);
            window.setPosition({
                static_cast<int>((dt.size.x - GameConfig::WINDOW_WIDTH) / 2),
                static_cast<int>((dt.size.y - GameConfig::WINDOW_HEIGHT) / 2)
            });
        } else {
            unsigned int w = static_cast<unsigned int>(dt.size.x * 0.9f);
            unsigned int h = static_cast<unsigned int>(dt.size.y * 0.9f);
            window.create(sf::VideoMode({w, h}), "CrazyJack",
                sf::Style::Titlebar | sf::Style::Close);
            window.setPosition({
                static_cast<int>((dt.size.x - w) / 2),
                static_cast<int>((dt.size.y - h) / 2)
            });
        }
        window.setFramerateLimit(60);
        settings.recalcScale(window.getSize().x);
    };

    NetworkMode mode = NetworkMode::LOCAL;
    bool menuDone = false;

    // Outer loop: returns here after options screen
    while (window.isOpen() && !menuDone) {
        // Build menu layout for current window size
        float cx = window.getSize().x / 2.f;
        float wy = (float)window.getSize().y;

        sf::Text title(font, "CrazyJack", 36);
        title.setFillColor(sf::Color(245, 224, 32));
        sf::FloatRect titleBounds = title.getLocalBounds();
        title.setPosition({cx - titleBounds.size.x / 2.f, wy * 0.10f});

        sf::Text subtitle(font, "Over-engineered Blackjack", 14);
        subtitle.setFillColor(sf::Color(180, 180, 180));
        sf::FloatRect subBounds = subtitle.getLocalBounds();
        subtitle.setPosition({cx - subBounds.size.x / 2.f, wy * 0.10f + 50.f});

        sf::Vector2f btnSize = {200.f, 35.f};
        float btnX = cx - btnSize.x / 2.f;
        float btnStartY = wy * 0.45f;
        float btnGap = 45.f;

        MenuButton btnLocal(font, "Local Game", {btnX, btnStartY}, btnSize);
        MenuButton btnHost(font, "Host Online Game", {btnX, btnStartY + btnGap}, btnSize);
        MenuButton btnJoin(font, "Join Online Game", {btnX, btnStartY + btnGap * 2}, btnSize);
        MenuButton btnOptions(font, "Options", {btnX, btnStartY + btnGap * 3}, btnSize);

        sf::Text infoText(font, "IP: 127.0.0.1  Port: " + std::to_string(NET_DEFAULT_PORT), 10);
        infoText.setFillColor(sf::Color(150, 150, 150));
        sf::FloatRect infoBounds = infoText.getLocalBounds();
        infoText.setPosition({cx - infoBounds.size.x / 2.f, btnStartY - infoBounds.size.y - 10.f});

        bool openOptions = false;

        // Menu loop
        while (window.isOpen() && !menuDone && !openOptions) {
            while (std::optional event = window.pollEvent()) {
                if (event->is<sf::Event::Closed>()) {
                    window.close();
                    return 0;
                }
                if (const auto* mouseMoved = event->getIf<sf::Event::MouseMoved>()) {
                    sf::Vector2f mp = window.mapPixelToCoords({mouseMoved->position.x, mouseMoved->position.y});
                    btnLocal.hovered = btnLocal.contains(mp);
                    btnHost.hovered = btnHost.contains(mp);
                    btnJoin.hovered = btnJoin.contains(mp);
                    btnOptions.hovered = btnOptions.contains(mp);
                }
                if (const auto* mouseBtn = event->getIf<sf::Event::MouseButtonPressed>()) {
                    if (mouseBtn->button == sf::Mouse::Button::Left) {
                        sf::Vector2f mp = window.mapPixelToCoords({mouseBtn->position.x, mouseBtn->position.y});
                        if (btnLocal.contains(mp)) { mode = NetworkMode::LOCAL; menuDone = true; }
                        if (btnHost.contains(mp))  { mode = NetworkMode::HOST;  menuDone = true; }
                        if (btnJoin.contains(mp))  { mode = NetworkMode::CLIENT; menuDone = true; }
                        if (btnOptions.contains(mp)) { openOptions = true; }
                    }
                }
            }

            window.clear(sf::Color(20, 30, 20));
            window.draw(title);
            window.draw(subtitle);
            btnLocal.draw(window);
            btnHost.draw(window);
            btnJoin.draw(window);
            btnOptions.draw(window);
            window.draw(infoText);
            window.display();
        }

        if (!window.isOpen() || menuDone) break;

        // ── Options screen ──
        if (openOptions) {
            auto& settings = GameSettings::instance();
            bool optionsDone = false;

            while (window.isOpen() && !optionsDone) {
                float ocx = window.getSize().x / 2.f;
                float ocy = (float)window.getSize().y;

                // Title
                sf::Text optTitle(font, "Options", 36);
                optTitle.setFillColor(sf::Color(245, 224, 32));
                sf::FloatRect otBounds = optTitle.getLocalBounds();
                optTitle.setPosition({ocx - otBounds.size.x / 2.f, ocy * 0.08f});

                // Window size section
                sf::Text winLabel(font, "Window Size", 16);
                winLabel.setFillColor(sf::Color(200, 200, 200));
                sf::FloatRect wlBounds = winLabel.getLocalBounds();
                winLabel.setPosition({ocx - wlBounds.size.x / 2.f, ocy * 0.22f});

                sf::Vector2f sBtnSize = {140.f, 30.f};
                float sGap = 150.f;
                float sY = ocy * 0.30f;

                MenuButton btnLarge(font, "90% Desktop", {ocx - sGap + 5.f, sY}, sBtnSize);
                MenuButton btnClassic(font, "Classic (683x384)", {ocx + 5.f, sY}, sBtnSize);

                if (!settings.smallWindow) {
                    btnLarge.shape.setOutlineColor(sf::Color(245, 224, 32));
                    btnLarge.shape.setOutlineThickness(3.f);
                } else {
                    btnClassic.shape.setOutlineColor(sf::Color(245, 224, 32));
                    btnClassic.shape.setOutlineThickness(3.f);
                }

                // Random skills toggle
                sf::Text skillLabel(font, "Random Skills Each Round", 16);
                skillLabel.setFillColor(sf::Color(200, 200, 200));
                sf::FloatRect slBounds = skillLabel.getLocalBounds();
                skillLabel.setPosition({ocx - slBounds.size.x / 2.f, ocy * 0.45f});

                std::string toggleText = settings.randomSkillsEachRound ? "ON" : "OFF";
                sf::Color toggleColor = settings.randomSkillsEachRound
                    ? sf::Color(100, 255, 100) : sf::Color(255, 100, 100);
                MenuButton btnToggle(font, toggleText, {ocx - 50.f, ocy * 0.53f}, {100.f, 30.f});
                btnToggle.label.setFillColor(toggleColor);

                // Back button
                MenuButton btnBack(font, "Back", {ocx - 60.f, ocy * 0.75f}, {120.f, 35.f});

                while (std::optional event = window.pollEvent()) {
                    if (event->is<sf::Event::Closed>()) {
                        window.close();
                        return 0;
                    }
                    if (const auto* mouseMoved = event->getIf<sf::Event::MouseMoved>()) {
                        sf::Vector2f mp = window.mapPixelToCoords({mouseMoved->position.x, mouseMoved->position.y});
                        btnLarge.hovered = btnLarge.contains(mp);
                        btnClassic.hovered = btnClassic.contains(mp);
                        btnToggle.hovered = btnToggle.contains(mp);
                        btnBack.hovered = btnBack.contains(mp);
                    }
                    if (const auto* mouseBtn = event->getIf<sf::Event::MouseButtonPressed>()) {
                        if (mouseBtn->button == sf::Mouse::Button::Left) {
                            sf::Vector2f mp = window.mapPixelToCoords({mouseBtn->position.x, mouseBtn->position.y});
                            if (btnLarge.contains(mp) && settings.smallWindow) {
                                settings.smallWindow = false;
                                applyWindowSize(); break;
                            }
                            if (btnClassic.contains(mp) && !settings.smallWindow) {
                                settings.smallWindow = true;
                                applyWindowSize(); break;
                            }
                            if (btnToggle.contains(mp)) {
                                settings.randomSkillsEachRound = !settings.randomSkillsEachRound;
                                break;
                            }
                            if (btnBack.contains(mp)) { optionsDone = true; }
                        }
                    }
                }

                window.clear(sf::Color(20, 30, 20));
                window.draw(optTitle);
                window.draw(winLabel);
                btnLarge.draw(window);
                btnClassic.draw(window);
                window.draw(skillLabel);
                btnToggle.draw(window);
                btnBack.draw(window);
                window.display();
            }
            // Loop back to rebuild main menu with new window size
            continue;
        }
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

        float lobbyBtnX = window.getSize().x / 2.f - 100.f;
        MenuButton btnStart(font, "Start Game", {lobbyBtnX, window.getSize().y * 0.55f + 90.f}, {200.f, 35.f});

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
                window.getSize().y * 0.55f + 45.f
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
        if (connectedCount == 1){
            newGame.SetupGame(1, connectedCount, 1);
        }
        else newGame.SetupGame(1, connectedCount , 1);

    } else if (mode == NetworkMode::CLIENT) {
        // ── IP input screen ──
        {
            std::string ipInput;
            bool ipDone = false;
            sf::Clock cursorBlink;

            sf::Text ipTitle(font, "Enter Host IP", 36);
            ipTitle.setFillColor(sf::Color(245, 224, 32));
            sf::FloatRect itBounds = ipTitle.getLocalBounds();
            ipTitle.setPosition({
                window.getSize().x / 2.f - itBounds.size.x / 2.f,
                40.f
            });

            sf::Text ipHint(font, "Leave empty for localhost (127.0.0.1)", 10);
            ipHint.setFillColor(sf::Color(140, 140, 140));
            sf::FloatRect ihBounds = ipHint.getLocalBounds();
            ipHint.setPosition({
                window.getSize().x / 2.f - ihBounds.size.x / 2.f,
                90.f
            });

            sf::Text pasteHint(font, "Ctrl+V to paste | Enter to connect", 10);
            pasteHint.setFillColor(sf::Color(120, 120, 120));
            sf::FloatRect phBounds = pasteHint.getLocalBounds();
            pasteHint.setPosition({
                window.getSize().x / 2.f - phBounds.size.x / 2.f,
                108.f
            });

            // Input box
            float boxW = 260.f, boxH = 30.f;
            float boxX = window.getSize().x / 2.f - boxW / 2.f;
            float boxY = window.getSize().y * 0.45f;

            while (window.isOpen() && !ipDone) {
                while (std::optional event = window.pollEvent()) {
                    if (event->is<sf::Event::Closed>()) {
                        window.close();
                        return 0;
                    }
                    if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                        if (keyPressed->scancode == sf::Keyboard::Scancode::Enter) {
                            ipDone = true;
                        }
                        if (keyPressed->scancode == sf::Keyboard::Scancode::Backspace && !ipInput.empty()) {
                            ipInput.pop_back();
                        }
                        if (keyPressed->scancode == sf::Keyboard::Scancode::Escape) {
                            window.close();
                            return 0;
                        }
                        // Ctrl+V paste
                        if (keyPressed->scancode == sf::Keyboard::Scancode::V &&
                            keyPressed->control) {
                            sf::String clip = sf::Clipboard::getString();
                            std::string clipStr = clip.toAnsiString();
                            // Strip whitespace/newlines
                            for (char c : clipStr) {
                                if (c > 31 && c < 127) ipInput += c;
                            }
                        }
                    }
                    if (const auto* textEntered = event->getIf<sf::Event::TextEntered>()) {
                        char c = static_cast<char>(textEntered->unicode);
                        // Allow printable ASCII (digits, dots, letters for hostnames)
                        if (c > 31 && c < 127 && c != '\r' && c != '\n') {
                            ipInput += c;
                        }
                    }
                }

                // Blinking cursor
                bool showCursor = (int)(cursorBlink.getElapsedTime().asSeconds() * 2.f) % 2 == 0;
                std::string displayStr = ipInput + (showCursor ? "|" : "");

                sf::RectangleShape box({boxW, boxH});
                box.setPosition({boxX, boxY});
                box.setFillColor(sf::Color(30, 30, 40));
                box.setOutlineThickness(2.f);
                box.setOutlineColor(sf::Color(180, 180, 200));

                sf::Text ipText(font, displayStr, 14);
                ipText.setFillColor(sf::Color::White);
                ipText.setPosition({boxX + 8.f, boxY + 6.f});

                window.clear(sf::Color(20, 30, 20));
                window.draw(ipTitle);
                window.draw(ipHint);
                window.draw(pasteHint);
                window.draw(box);
                window.draw(ipText);
                window.display();
            }

            if (!window.isOpen()) return 0;
            if (!ipInput.empty()) serverIP = ipInput;
        }

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

        // Client skips SetupGame — server state will rebuild players in clientReceive()
    } else {
        newGame.SetupGame();
    }

    newGame.RunGame();
    return 0;
}
