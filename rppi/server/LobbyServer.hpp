#pragma once

#include <string>
#include <map>
#include <queue>
#include <libwebsockets.h>
#include "../common/Messages.hpp"

class LobbyServer {
public:
    explicit LobbyServer(int port);
    ~LobbyServer();

    void run();   // blocking — starts the lws event loop
    void stop();

    // libwebsockets requires a static callback; it forwards to the instance
    static int wsCallback(lws* wsi, lws_callback_reasons reason,
                          void* user, void* in, size_t len);

private:
    // ── Event handlers ───────────────────────────────────────────────────
    void onConnect(lws* wsi);
    void onMessage(lws* wsi, const std::string& raw);
    void onDisconnect(lws* wsi);
    void onWritable(lws* wsi);

    // ── Lobby message handlers ───────────────────────────────────────────
    void handleCreateRoom(lws* wsi, const json& j);
    void handleJoinRoom(lws* wsi, const json& j);
    void handleStartGame(lws* wsi);

    // ── Helpers ──────────────────────────────────────────────────────────
    void sendTo(lws* wsi, const json& msg);
    std::string generateRoomCode();
    int spawnGameRoom(const std::string& roomCode,
                      const std::string& hostName,
                      const std::string& guestName);  // returns port

    // ── Room state ───────────────────────────────────────────────────────
    struct Room {
        std::string code;
        std::string hostName;
        std::string guestName;
        lws* host  = nullptr;   // created the room
        lws* guest = nullptr;   // joined the room
        enum class State { WAITING, FULL, PLAYING } state = State::WAITING;
    };

    // ── Members ──────────────────────────────────────────────────────────
    int           port_;
    bool          running_ = false;
    lws_context*  context_ = nullptr;

    std::map<std::string, Room>         rooms_;        // code  → Room
    std::map<lws*, std::string>         clientRooms_;  // wsi   → room code
    std::map<lws*, std::queue<std::string>> writeQueues_; // wsi → pending sends

    int nextGamePort_ = 9001;  // incremented each time a room spawns
};
