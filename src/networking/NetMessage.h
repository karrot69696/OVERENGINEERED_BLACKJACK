#ifndef NETMESSAGE_H
#define NETMESSAGE_H

#include <cstdint>

// Network message types sent between server and client
enum class NetMsgType : uint8_t {
    // Client → Server
    CLIENT_JOIN,            // client requests to join (payload: none initially)
    CLIENT_ACTION,          // PlayerAction from a remote player
    CLIENT_TARGET,          // PlayerTargeting from a remote player
    CLIENT_DISCONNECT,      // client leaving

    // Server → Client
    SERVER_WELCOME,         // server assigns player ID + initial game config
    SERVER_GAMESTATE,       // full GameState snapshot
    SERVER_EVENT,           // single GameEvent for presentation layer
    SERVER_EVENT_BATCH,     // batch of GameEvents
    SERVER_KICK,            // server kicking client
    SERVER_GAME_SETUP,      // player list + initial setup info
    SERVER_GAME_START,      // server signals lobby → game transition
    SERVER_TARGET_REQUEST,  // server asks a specific client to pick a card (multi-actor skills)
    SERVER_REACTIVE_PROMPT, // server asks client: activate reactive skill? (yes/no)
    CLIENT_REACTIVE_RESPONSE, // client answers yes/no to reactive prompt
};

enum class NetworkMode : uint8_t {
    LOCAL,      // no networking, current behavior
    HOST,       // this instance runs the server + plays locally
    CLIENT      // this instance connects to a remote server
};

// Fixed header prepended to every ENet packet
struct NetPacketHeader {
    NetMsgType type;
    uint16_t payloadSize;   // size of data following this header
};

constexpr uint16_t NET_DEFAULT_PORT = 7777;
constexpr int NET_MAX_CLIENTS = 6;  // max remote players (host is local)
constexpr int NET_CHANNEL_COUNT = 2; // channel 0: reliable (state), channel 1: reliable (events)

#endif
