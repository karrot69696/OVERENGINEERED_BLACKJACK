#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <enet/enet.h>
#include <vector>
#include <queue>
#include <functional>
#include <string>
#include <iostream>
#include <unordered_map>

#include "NetMessage.h"
#include "NetSerializer.h"
#include "lowLevelEntities/GameState.h"
#include "lowLevelEntities/GameEvent.h"

// Represents a connected remote player on the server side
struct RemotePlayer {
    ENetPeer* peer = nullptr;
    int playerId = -1;      // assigned by server
    bool ready = false;
};

// Incoming message from a remote peer
struct IncomingMessage {
    NetMsgType type;
    int playerId;           // resolved from peer
    ByteBuffer payload;
};

class NetworkManager {
private:
    NetworkMode mode = NetworkMode::LOCAL;
    ENetHost* host = nullptr;           // ENet host (server or client)
    ENetPeer* serverPeer = nullptr;     // client only: connection to server

    // Server: track connected clients
    std::vector<RemotePlayer> remoteClients;
    int nextClientPlayerId = 1;         // server assigns IDs starting from 1 (0 = host)

    // Incoming message queue (both server and client)
    std::queue<IncomingMessage> incomingMessages;

    // Client: received state and events from server
    bool hasNewGameState = false;
    GameState receivedGameState;
    std::vector<GameEvent> receivedEvents;

    // Client: assigned player ID
    int localPlayerId = 0;

    // Helper: send raw packet to a peer
    void sendPacket(ENetPeer* peer, NetMsgType type, const ByteBuffer& payload, int channel, bool reliable);

    // Process a received ENet packet
    void handlePacket(ENetPeer* peer, ENetPacket* packet);

    // Server handlers
    void handleClientJoin(ENetPeer* peer, ByteBuffer& buf);
    void handleClientAction(ENetPeer* peer, ByteBuffer& buf);
    void handleClientTarget(ENetPeer* peer, ByteBuffer& buf);
    void handleClientDisconnect(ENetPeer* peer);

    // Server handlers
    void handleClientReactiveResponse(ENetPeer* peer, ByteBuffer& buf);
    void handleClientChronoResponse(ENetPeer* peer, ByteBuffer& buf);

    // Client handlers
    void handleServerWelcome(ByteBuffer& buf);
    void handleServerGameState(ByteBuffer& buf);
    void handleServerEvent(ByteBuffer& buf);
    void handleServerEventBatch(ByteBuffer& buf);
    void handleServerGameSetup(ByteBuffer& buf);
    void handleServerTargetRequest(ByteBuffer& buf);
    void handleServerReactivePrompt(ByteBuffer& buf);
    void handleServerChronoPrompt(ByteBuffer& buf);

    // Find RemotePlayer by peer
    RemotePlayer* findRemoteByPeer(ENetPeer* peer);
    int findPlayerIdByPeer(ENetPeer* peer);

public:
    NetworkManager() = default;
    ~NetworkManager();

    // Lifecycle
    bool initServer(uint16_t port = NET_DEFAULT_PORT);
    bool initClient(const std::string& serverIP, uint16_t port = NET_DEFAULT_PORT);
    void shutdown();

    // Call every frame to process ENet events
    void poll();

    NetworkMode getMode() const { return mode; }
    bool isServer() const { return mode == NetworkMode::HOST; }
    bool isClient() const { return mode == NetworkMode::CLIENT; }
    bool isLocal() const { return mode == NetworkMode::LOCAL; }
    int getLocalPlayerId() const { return localPlayerId; }
    int getConnectedClientCount() const { return static_cast<int>(remoteClients.size()); }

    // ========================================================================
    // Server API
    // ========================================================================

    // Broadcast GameState to all clients
    void broadcastGameState(GameState& state);

    // Broadcast a batch of GameEvents to all clients
    void broadcastEvents(const std::vector<GameEvent>& events);

    // Send welcome message to a newly connected client
    void sendWelcome(ENetPeer* peer, int assignedPlayerId, int totalPlayers);

    // Check if a remote player has a pending action
    bool hasRemoteAction(int playerId) const;

    // Consume pending action for a remote player (returns IDLE if none)
    PlayerAction consumeRemoteAction(int playerId);

    // Check/consume pending targeting for a remote player
    bool hasRemoteTarget(int playerId) const;
    PlayerTargeting consumeRemoteTarget(int playerId);

    // Flush all stale remote actions/targets (call on turn transitions)
    void clearAllRemoteInputs();

    // ========================================================================
    // Client API
    // ========================================================================

    // Send player action to server
    void sendAction(PlayerAction action);

    // Send targeting to server
    void sendTarget(const PlayerTargeting& targeting);

    // Check if new GameState arrived from server
    bool hasNewState() const { return hasNewGameState; }

    // Consume the latest GameState from server
    GameState& consumeGameState() { hasNewGameState = false; return receivedGameState; }

    // Drain received events
    std::vector<GameEvent> drainReceivedEvents();

    // Server: broadcast game start signal to all clients
    void broadcastGameStart();

    // Server: send a target-pick request to a specific client (multi-actor skills)
    void sendTargetRequest(int targetPlayerId, const TargetRequestData& req);

    // Server: send reactive skill prompt to a specific client (yes/no)
    void sendReactivePrompt(int playerId, SkillName skill,std::string extraInfo, float timerDuration);
    // Server: check/consume reactive response from a remote player
    bool hasReactiveResponse(int playerId) const;
    bool consumeReactiveResponse(int playerId);  // returns accepted (true/false)

    // Client: has the server signaled game start?
    bool hasGameStarted() const { return gameStarted; }

    // Client: pending target request from server (multi-actor skills)
    bool hasPendingTargetRequest() const { return hasPendingTargetRequestFlag; }
    TargetRequestData consumePendingTargetRequest() {
        hasPendingTargetRequestFlag = false;
        return pendingTargetRequest;
    }

    // Client: pending reactive prompt from server
    bool hasPendingReactivePrompt() const { return hasPendingReactivePromptFlag; }
    ReactivePromptData consumePendingReactivePrompt() {
        hasPendingReactivePromptFlag = false;
        return pendingReactivePromptData;
    }
    // Client: send reactive response to server
    void sendReactiveResponse(bool accepted);

    // Server: send Chronosphere choice prompt to a specific client
    void sendChronoPrompt(int playerId, bool hasSnapshot);
    // Server: check/consume chrono response from a remote player
    bool hasChronoResponse(int playerId) const;
    ChronoChoice consumeChronoResponse(int playerId);

    // Client: send chrono choice response to server
    void sendChronoResponse(ChronoChoice choice);
    // Client: pending chrono prompt from server
    bool hasPendingChronoPrompt() const { return hasPendingChronoPromptFlag; }
    ChronoPromptData consumePendingChronoPrompt() {
        hasPendingChronoPromptFlag = false;
        return pendingChronoPromptData;
    }

    // Callbacks for connection events
    std::function<void(int playerId)> onClientConnected;    // server: new client joined
    std::function<void(int playerId)> onClientDisconnected; // server: client left
    std::function<void(int assignedId)> onConnectedToServer; // client: got welcome
    std::function<void()> onDisconnectedFromServer;          // client: lost connection

private:
    // Pending actions/targets from remote players (server-side)
    std::unordered_map<int, PlayerAction> pendingActions;
    std::unordered_map<int, PlayerTargeting> pendingTargets;
    bool gameStarted = false;   // client: set true when SERVER_GAME_START received

    // Client: incoming target-pick request from server
    bool hasPendingTargetRequestFlag = false;
    TargetRequestData pendingTargetRequest;

    // Reactive prompt state
    std::unordered_map<int, bool> pendingReactiveResponses;  // server: playerId → accepted
    bool hasPendingReactivePromptFlag = false;               // client
    ReactivePromptData pendingReactivePromptData;            // client

    // Chronosphere prompt state
    std::unordered_map<int, ChronoChoice> pendingChronoResponses;  // server: playerId → choice
    bool hasPendingChronoPromptFlag = false;                       // client
    ChronoPromptData pendingChronoPromptData;                      // client
};

#endif
