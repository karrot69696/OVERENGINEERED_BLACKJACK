#include "NetworkManager.h"

// ============================================================================
// Lifecycle
// ============================================================================

NetworkManager::~NetworkManager() {
    shutdown();
}

bool NetworkManager::initServer(uint16_t port) {
    if (enet_initialize() != 0) {
        std::cerr << "[NetworkManager] Failed to initialize ENet" << std::endl;
        return false;
    }

    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = port;

    host = enet_host_create(&address, NET_MAX_CLIENTS, NET_CHANNEL_COUNT, 0, 0);
    if (!host) {
        std::cerr << "[NetworkManager] Failed to create ENet server on port " << port << std::endl;
        return false;
    }

    mode = NetworkMode::HOST;
    localPlayerId = 0; // host is always player 0
    std::cout << "[NetworkManager] Server started on port " << port << std::endl;
    return true;
}

bool NetworkManager::initClient(const std::string& serverIP, uint16_t port) {
    if (enet_initialize() != 0) {
        std::cerr << "[NetworkManager] Failed to initialize ENet" << std::endl;
        return false;
    }

    host = enet_host_create(nullptr, 1, NET_CHANNEL_COUNT, 0, 0);
    if (!host) {
        std::cerr << "[NetworkManager] Failed to create ENet client" << std::endl;
        return false;
    }

    ENetAddress address;
    enet_address_set_host(&address, serverIP.c_str());
    address.port = port;

    serverPeer = enet_host_connect(host, &address, NET_CHANNEL_COUNT, 0);
    if (!serverPeer) {
        std::cerr << "[NetworkManager] Failed to initiate connection to " << serverIP << ":" << port << std::endl;
        return false;
    }

    mode = NetworkMode::CLIENT;
    std::cout << "[NetworkManager] Connecting to " << serverIP << ":" << port << std::endl;
    return true;
}

void NetworkManager::shutdown() {
    if (host) {
        // Disconnect all peers gracefully
        if (mode == NetworkMode::CLIENT && serverPeer) {
            enet_peer_disconnect(serverPeer, 0);
            // Flush disconnect
            ENetEvent event;
            while (enet_host_service(host, &event, 500) > 0) {
                if (event.type == ENET_EVENT_TYPE_DISCONNECT) break;
                if (event.type == ENET_EVENT_TYPE_RECEIVE)
                    enet_packet_destroy(event.packet);
            }
            serverPeer = nullptr;
        }

        enet_host_destroy(host);
        host = nullptr;
        enet_deinitialize();
    }
    mode = NetworkMode::LOCAL;
    remoteClients.clear();
    pendingActions.clear();
    pendingTargets.clear();
    while (!incomingMessages.empty()) incomingMessages.pop();
}

// ============================================================================
// Polling
// ============================================================================

void NetworkManager::poll() {
    if (!host) return;

    ENetEvent event;
    while (enet_host_service(host, &event, 0) > 0) {
        switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
                if (isServer()) {
                    std::cout << "[NetworkManager] Client connected from "
                              << event.peer->address.host << ":" << event.peer->address.port << std::endl;
                    // Don't assign player ID yet — wait for CLIENT_JOIN message
                } else {
                    // Client: we've connected to server, send JOIN request
                    std::cout << "[NetworkManager] Connected to server! Sending JOIN..." << std::endl;
                    ByteBuffer joinBuf;
                    sendPacket(event.peer, NetMsgType::CLIENT_JOIN, joinBuf, 0, true);
                }
                break;

            case ENET_EVENT_TYPE_RECEIVE:
                handlePacket(event.peer, event.packet);
                enet_packet_destroy(event.packet);
                break;

            case ENET_EVENT_TYPE_DISCONNECT:
                if (isServer()) {
                    handleClientDisconnect(event.peer);
                } else {
                    std::cout << "[NetworkManager] Disconnected from server" << std::endl;
                    serverPeer = nullptr;
                    if (onDisconnectedFromServer) onDisconnectedFromServer();
                }
                event.peer->data = nullptr;
                break;

            case ENET_EVENT_TYPE_NONE:
                break;
        }
    }
}

// ============================================================================
// Packet handling
// ============================================================================

void NetworkManager::handlePacket(ENetPeer* peer, ENetPacket* packet) {
    if (packet->dataLength < sizeof(NetPacketHeader)) return;

    NetPacketHeader header;
    std::memcpy(&header, packet->data, sizeof(NetPacketHeader));
    ByteBuffer buf(packet->data + sizeof(NetPacketHeader), header.payloadSize);

    if (isServer()) {
        switch (header.type) {
            case NetMsgType::CLIENT_JOIN:
                handleClientJoin(peer, buf);
                break;
            case NetMsgType::CLIENT_ACTION:
                handleClientAction(peer, buf);
                break;
            case NetMsgType::CLIENT_TARGET:
                handleClientTarget(peer, buf);
                break;
            case NetMsgType::CLIENT_DISCONNECT:
                handleClientDisconnect(peer);
                break;
            case NetMsgType::CLIENT_REACTIVE_RESPONSE:
                handleClientReactiveResponse(peer, buf);
                break;
            case NetMsgType::CLIENT_CHRONO_RESPONSE:
                handleClientChronoResponse(peer, buf);
                break;
            default:
                std::cerr << "[NetworkManager] Server got unexpected msg type: "
                          << static_cast<int>(header.type) << std::endl;
                break;
        }
    } else {
        switch (header.type) {
            case NetMsgType::SERVER_WELCOME:
                handleServerWelcome(buf);
                break;
            case NetMsgType::SERVER_GAMESTATE:
                handleServerGameState(buf);
                break;
            case NetMsgType::SERVER_EVENT:
                handleServerEvent(buf);
                break;
            case NetMsgType::SERVER_EVENT_BATCH:
                handleServerEventBatch(buf);
                break;
            case NetMsgType::SERVER_GAME_START:
                std::cout << "[NetworkManager] Server signaled game start!" << std::endl;
                gameStarted = true;
                break;
            case NetMsgType::SERVER_TARGET_REQUEST:
                handleServerTargetRequest(buf);
                break;
            case NetMsgType::SERVER_REACTIVE_PROMPT:
                handleServerReactivePrompt(buf);
                break;
            case NetMsgType::SERVER_CHRONO_PROMPT:
                handleServerChronoPrompt(buf);
                break;
            default:
                std::cerr << "[NetworkManager] Client got unexpected msg type: "
                          << static_cast<int>(header.type) << std::endl;
                break;
        }
    }
}

void NetworkManager::sendPacket(ENetPeer* peer, NetMsgType type, const ByteBuffer& payload, int channel, bool reliable) {
    NetPacketHeader header;
    header.type = type;
    header.payloadSize = static_cast<uint16_t>(payload.getSize());

    size_t totalSize = sizeof(NetPacketHeader) + payload.getSize();
    ENetPacket* packet = enet_packet_create(nullptr, totalSize,
        reliable ? ENET_PACKET_FLAG_RELIABLE : 0);

    std::memcpy(packet->data, &header, sizeof(NetPacketHeader));
    if (payload.getSize() > 0) {
        std::memcpy(packet->data + sizeof(NetPacketHeader), payload.getData(), payload.getSize());
    }

    enet_peer_send(peer, channel, packet);
}

// ============================================================================
// Server handlers
// ============================================================================

void NetworkManager::handleClientJoin(ENetPeer* peer, ByteBuffer& /*buf*/) {
    RemotePlayer rp;
    rp.peer = peer;
    rp.playerId = nextClientPlayerId++;
    rp.ready = true;
    remoteClients.push_back(rp);

    std::cout << "[NetworkManager] Assigned player ID " << rp.playerId << " to new client" << std::endl;

    // Send welcome with assigned ID
    sendWelcome(peer, rp.playerId, static_cast<int>(remoteClients.size()) + 1); // +1 for host

    if (onClientConnected) onClientConnected(rp.playerId);
}

void NetworkManager::handleClientAction(ENetPeer* peer, ByteBuffer& buf) {
    int playerId;
    PlayerAction action;
    NetSerializer::readPlayerAction(buf, playerId, action);

    // Verify the peer owns this player ID
    int actualId = findPlayerIdByPeer(peer);
    if (actualId != playerId) {
        std::cerr << "[NetworkManager] Action from wrong player ID (claimed "
                  << playerId << " but is " << actualId << ")" << std::endl;
        return;
    }

    pendingActions[playerId] = action;
    std::cout << "[NetworkManager] Got action from player " << playerId
              << ": " << static_cast<int>(action) << std::endl;
}

void NetworkManager::handleClientTarget(ENetPeer* peer, ByteBuffer& buf) {
    int playerId;
    PlayerTargeting targeting;
    NetSerializer::readPlayerTargeting(buf, playerId, targeting);

    int actualId = findPlayerIdByPeer(peer);
    if (actualId != playerId) return;

    pendingTargets[playerId] = targeting;
    std::cout << "[NetworkManager] Got targeting from player " << playerId << std::endl;
}

void NetworkManager::handleClientDisconnect(ENetPeer* peer) {
    int playerId = findPlayerIdByPeer(peer);
    if (playerId >= 0) {
        std::cout << "[NetworkManager] Player " << playerId << " disconnected" << std::endl;
        remoteClients.erase(
            std::remove_if(remoteClients.begin(), remoteClients.end(),
                [peer](const RemotePlayer& rp) { return rp.peer == peer; }),
            remoteClients.end());
        pendingActions.erase(playerId);
        pendingTargets.erase(playerId);
        if (onClientDisconnected) onClientDisconnected(playerId);
    }
}

// ============================================================================
// Client handlers
// ============================================================================

void NetworkManager::handleServerWelcome(ByteBuffer& buf) {
    localPlayerId = buf.readI32();
    int totalPlayers = buf.readI32();
    std::cout << "[NetworkManager] Welcome! Assigned player ID: " << localPlayerId
              << ", total players: " << totalPlayers << std::endl;

    if (onConnectedToServer) onConnectedToServer(localPlayerId);
}

void NetworkManager::handleServerGameState(ByteBuffer& buf) {
    NetSerializer::readGameState(buf, receivedGameState);
    hasNewGameState = true;
    // No log here — fires every frame, too noisy
}

void NetworkManager::handleServerEvent(ByteBuffer& buf) {
    GameEvent event = NetSerializer::readGameEvent(buf);
    receivedEvents.push_back(event);
}

void NetworkManager::handleServerEventBatch(ByteBuffer& buf) {
    auto events = NetSerializer::readEventBatch(buf);
    for (auto& e : events) {
        receivedEvents.push_back(std::move(e));
    }
}

void NetworkManager::handleServerGameSetup(ByteBuffer& /*buf*/) {
    // Future: receive full player list, skills, etc.
}

void NetworkManager::handleServerTargetRequest(ByteBuffer& buf) {
    pendingTargetRequest = NetSerializer::readTargetRequest(buf);
    hasPendingTargetRequestFlag = true;
    std::cout << "[NetworkManager] Got target request from server (isBoostPick="
              << pendingTargetRequest.isBoostPick << ", "
              << pendingTargetRequest.allowedCardIds.size() << " allowed cards)" << std::endl;
}

// ============================================================================
// Server API
// ============================================================================

void NetworkManager::broadcastGameState(GameState& state) {
    ByteBuffer buf;
    NetSerializer::writeGameState(buf, state);
    for (auto& client : remoteClients) {
        sendPacket(client.peer, NetMsgType::SERVER_GAMESTATE, buf, 0, true);
    }
}

void NetworkManager::broadcastEvents(const std::vector<GameEvent>& events) {
    if (events.empty()) return;

    ByteBuffer buf;
    NetSerializer::writeEventBatch(buf, events);
    for (auto& client : remoteClients) {
        sendPacket(client.peer, NetMsgType::SERVER_EVENT_BATCH, buf, 1, true);
    }
}

void NetworkManager::sendTargetRequest(int targetPlayerId, const TargetRequestData& req) {
    for (auto& client : remoteClients) {
        if (client.playerId == targetPlayerId) {
            ByteBuffer buf;
            NetSerializer::writeTargetRequest(buf, req);
            sendPacket(client.peer, NetMsgType::SERVER_TARGET_REQUEST, buf, 0, true);
            std::cout << "[NetworkManager] Sent target request to player " << targetPlayerId
                      << " (isBoostPick=" << req.isBoostPick << ")" << std::endl;
            return;
        }
    }
    std::cerr << "[NetworkManager] sendTargetRequest: player " << targetPlayerId << " not found" << std::endl;
}

void NetworkManager::broadcastGameStart() {
    ByteBuffer buf; // empty payload
    for (auto& client : remoteClients) {
        sendPacket(client.peer, NetMsgType::SERVER_GAME_START, buf, 0, true);
    }
    std::cout << "[NetworkManager] Broadcast game start to " << remoteClients.size() << " client(s)" << std::endl;
}

void NetworkManager::sendWelcome(ENetPeer* peer, int assignedPlayerId, int totalPlayers) {
    ByteBuffer buf;
    buf.writeI32(assignedPlayerId);
    buf.writeI32(totalPlayers);
    sendPacket(peer, NetMsgType::SERVER_WELCOME, buf, 0, true);
}

bool NetworkManager::hasRemoteAction(int playerId) const {
    return pendingActions.find(playerId) != pendingActions.end();
}

PlayerAction NetworkManager::consumeRemoteAction(int playerId) {
    auto it = pendingActions.find(playerId);
    if (it != pendingActions.end()) {
        PlayerAction action = it->second;
        pendingActions.erase(it);
        return action;
    }
    return PlayerAction::IDLE;
}

bool NetworkManager::hasRemoteTarget(int playerId) const {
    return pendingTargets.find(playerId) != pendingTargets.end();
}

PlayerTargeting NetworkManager::consumeRemoteTarget(int playerId) {
    auto it = pendingTargets.find(playerId);
    if (it != pendingTargets.end()) {
        PlayerTargeting t = it->second;
        pendingTargets.erase(it);
        return t;
    }
    return {};
}

void NetworkManager::clearAllRemoteInputs() {
    pendingActions.clear();
    pendingTargets.clear();
    pendingReactiveResponses.clear();
    pendingChronoResponses.clear();
}

void NetworkManager::sendReactivePrompt(int playerId, SkillName skill,std::string extraInfo, float timerDuration) {
    for (auto& client : remoteClients) {
        if (client.playerId == playerId) {
            ByteBuffer buf;
            NetSerializer::writeReactivePrompt(buf, skill, extraInfo, timerDuration);
            sendPacket(client.peer, NetMsgType::SERVER_REACTIVE_PROMPT, buf, 0, true);
            std::cout << "[NetworkManager] Sent reactive prompt to P" << playerId << std::endl;
            return;
        }
    }
    std::cerr << "[NetworkManager] sendReactivePrompt: player " << playerId << " not found" << std::endl;
}

bool NetworkManager::hasReactiveResponse(int playerId) const {
    return pendingReactiveResponses.find(playerId) != pendingReactiveResponses.end();
}

bool NetworkManager::consumeReactiveResponse(int playerId) {
    auto it = pendingReactiveResponses.find(playerId);
    if (it != pendingReactiveResponses.end()) {
        bool accepted = it->second;
        pendingReactiveResponses.erase(it);
        return accepted;
    }
    return false;
}

void NetworkManager::handleClientReactiveResponse(ENetPeer* peer, ByteBuffer& buf) {
    int playerId;
    bool accepted;
    NetSerializer::readReactiveResponse(buf, playerId, accepted);
    pendingReactiveResponses[playerId] = accepted;
    std::cout << "[NetworkManager] Received reactive response from P" << playerId
              << ": " << (accepted ? "YES" : "NO") << std::endl;
}

// ============================================================================
// Client API
// ============================================================================

void NetworkManager::sendAction(PlayerAction action) {
    if (!serverPeer) return;
    ByteBuffer buf;
    NetSerializer::writePlayerAction(buf, localPlayerId, action);
    sendPacket(serverPeer, NetMsgType::CLIENT_ACTION, buf, 0, true);
}

void NetworkManager::sendTarget(const PlayerTargeting& targeting) {
    if (!serverPeer) return;
    ByteBuffer buf;
    NetSerializer::writePlayerTargeting(buf, localPlayerId, targeting);
    sendPacket(serverPeer, NetMsgType::CLIENT_TARGET, buf, 0, true);
}

void NetworkManager::sendReactiveResponse(bool accepted) {
    if (!serverPeer) return;
    ByteBuffer buf;
    NetSerializer::writeReactiveResponse(buf, localPlayerId, accepted);
    sendPacket(serverPeer, NetMsgType::CLIENT_REACTIVE_RESPONSE, buf, 0, true);
    std::cout << "[NetworkManager] Sent reactive response: " << (accepted ? "YES" : "NO") << std::endl;
}

void NetworkManager::handleServerReactivePrompt(ByteBuffer& buf) {
    pendingReactivePromptData = NetSerializer::readReactivePrompt(buf);
    hasPendingReactivePromptFlag = true;
    std::cout << "[NetworkManager] Got reactive prompt from server" << std::endl;
}

// ============================================================================
// Chronosphere prompt/response
// ============================================================================

void NetworkManager::sendChronoPrompt(int playerId, bool hasSnapshot) {
    for (auto& client : remoteClients) {
        if (client.playerId == playerId) {
            ByteBuffer buf;
            NetSerializer::writeChronoPrompt(buf, hasSnapshot);
            sendPacket(client.peer, NetMsgType::SERVER_CHRONO_PROMPT, buf, 0, true);
            std::cout << "[NetworkManager] Sent chrono prompt to P" << playerId
                      << " (hasSnapshot=" << hasSnapshot << ")" << std::endl;
            return;
        }
    }
    std::cerr << "[NetworkManager] sendChronoPrompt: player " << playerId << " not found" << std::endl;
}

bool NetworkManager::hasChronoResponse(int playerId) const {
    return pendingChronoResponses.find(playerId) != pendingChronoResponses.end();
}

ChronoChoice NetworkManager::consumeChronoResponse(int playerId) {
    auto it = pendingChronoResponses.find(playerId);
    if (it != pendingChronoResponses.end()) {
        ChronoChoice choice = it->second;
        pendingChronoResponses.erase(it);
        return choice;
    }
    return ChronoChoice::NONE;
}

void NetworkManager::handleClientChronoResponse(ENetPeer* peer, ByteBuffer& buf) {
    int playerId;
    ChronoChoice choice;
    NetSerializer::readChronoResponse(buf, playerId, choice);
    pendingChronoResponses[playerId] = choice;
    std::cout << "[NetworkManager] Received chrono response from P" << playerId
              << ": " << static_cast<int>(choice) << std::endl;
}

void NetworkManager::sendChronoResponse(ChronoChoice choice) {
    if (!serverPeer) return;
    ByteBuffer buf;
    NetSerializer::writeChronoResponse(buf, localPlayerId, choice);
    sendPacket(serverPeer, NetMsgType::CLIENT_CHRONO_RESPONSE, buf, 0, true);
    std::cout << "[NetworkManager] Sent chrono response: " << static_cast<int>(choice) << std::endl;
}

void NetworkManager::handleServerChronoPrompt(ByteBuffer& buf) {
    pendingChronoPromptData = NetSerializer::readChronoPrompt(buf);
    hasPendingChronoPromptFlag = true;
    std::cout << "[NetworkManager] Got chrono prompt from server (hasSnapshot="
              << pendingChronoPromptData.hasSnapshot << ")" << std::endl;
}

std::vector<GameEvent> NetworkManager::drainReceivedEvents() {
    std::vector<GameEvent> result = std::move(receivedEvents);
    receivedEvents.clear();
    return result;
}

// ============================================================================
// Helpers
// ============================================================================

RemotePlayer* NetworkManager::findRemoteByPeer(ENetPeer* peer) {
    for (auto& rp : remoteClients) {
        if (rp.peer == peer) return &rp;
    }
    return nullptr;
}

int NetworkManager::findPlayerIdByPeer(ENetPeer* peer) {
    for (auto& rp : remoteClients) {
        if (rp.peer == peer) return rp.playerId;
    }
    return -1;
}
