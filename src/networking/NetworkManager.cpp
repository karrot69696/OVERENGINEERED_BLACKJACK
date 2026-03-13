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
