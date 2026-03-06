#pragma once

#include <string>
#include <map>
#include <queue>
#include <thread>
#include <mutex>
#include <atomic>
#include <memory>
#include <chrono>
#include <libwebsockets.h>
#include "../common/Messages.hpp"
#include "IGame.hpp"
#include "NotePiGame.hpp"

class GameRoom {
public:
    GameRoom(const std::string& roomCode, int port,
             const std::string& hostName = "",
             const std::string& guestName = "",
             const std::string& gameType = "gyropi");
    ~GameRoom();

    void run();   // blocking — lws event loop + game loop thread
    void stop();

    static int wsCallback(lws* wsi, lws_callback_reasons reason,
                          void* user, void* in, size_t len);

private:
    // ── Event handlers ───────────────────────────────────────────────────
    void onConnect(lws* wsi);
    void onMessage(lws* wsi, const std::string& raw);
    void onDisconnect(lws* wsi);
    void onWritable(lws* wsi);

    // ── Game message handlers ────────────────────────────────────────────
    void handleIdentify(lws* wsi, const json& j);
    void handleGyroData(lws* wsi, const json& j);
    void handleNoteSubmit(lws* wsi, const json& j);

    // ── Game loop (runs in separate thread) ──────────────────────────────
    void gameLoopThread();
    void broadcastGameState();
    bool bothPlayersConnected() const;

    // ── Helpers ──────────────────────────────────────────────────────────
    void sendTo(lws* wsi, const json& msg);
    std::string currentDate() const;

    // ── Members ──────────────────────────────────────────────────────────
    std::string  roomCode_;
    int          port_;
    std::string  players_;    // "Host & Guest" for leaderboard
    std::string  gameType_;   // "gyropi" or "notepi"
    bool         running_    = false;
    lws_context* context_    = nullptr;

    lws* performer_    = nullptr;
    lws* communicator_ = nullptr;

    using clock = std::chrono::steady_clock;
    std::chrono::time_point<clock> gameStartTime_;

    // Latest gyro input — written by lws thread, read by game loop thread
    struct GyroInput { float pitch = 0.f; float roll = 0.f; };
    GyroInput  gyro_;
    std::mutex gyroMutex_;

    // Game state to broadcast — written by game loop, read by lws thread
    MsgGameState pendingState_{};
    std::atomic<bool> stateDirty_{ false };
    std::mutex   stateMutex_;

    std::unique_ptr<IGame> game_;          // GyroPi only
    std::unique_ptr<NotePiGame> notePi_;   // NotePi only
    std::thread gameThread_;

    std::map<lws*, std::queue<std::string>> writeQueues_;
};
