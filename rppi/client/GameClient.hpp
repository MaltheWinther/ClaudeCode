#pragma once

#include <string>
#include <queue>
#include <functional>
#include <libwebsockets.h>
#include "../common/Messages.hpp"
#include "GyroReader.hpp"

// Connects to a GameRoom process. Behaviour depends on assigned role:
//   Performer    — streams gyro_data to the server at ~40Hz
//   Communicator — receives game_state and fires onGameState callback
//
// Runs in a background thread. Use QMetaObject::invokeMethod with
// Qt::QueuedConnection in the callbacks to safely update the UI.

class GameClient {
public:
    GameClient(const std::string& host, int port,
               const std::string& role,
               GyroReader* gyro);    // nullptr for communicator
    ~GameClient();

    void run();  // blocking — returns when game is over

    void setOnGameState(std::function<void(MsgGameState)> cb) { onGameState_ = std::move(cb); }
    void setOnGameOver (std::function<void(bool win)>     cb) { onGameOver_  = std::move(cb); }

    static int wsCallback(lws* wsi, lws_callback_reasons reason,
                          void* user, void* in, size_t len);
private:
    void onConnect(lws* wsi);
    void onMessage(const std::string& raw);
    void onWritable(lws* wsi);
    void onError();
    void sendGyroIfDue();
    void enqueue(const json& msg);

    std::string  host_;
    int          port_;
    std::string  role_;
    GyroReader*  gyro_;

    bool         running_  = false;
    lws_context* context_  = nullptr;
    lws*         wsi_      = nullptr;

    std::queue<std::string> outQueue_;

    std::function<void(MsgGameState)> onGameState_;
    std::function<void(bool)>         onGameOver_;

    double lastGyroSendMs_ = 0.0;
    static constexpr double GYRO_INTERVAL_MS = 25.0;  // ~40 Hz
};
