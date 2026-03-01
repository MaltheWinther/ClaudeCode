#pragma once

#include <string>
#include <queue>
#include <libwebsockets.h>
#include "../common/Messages.hpp"
#include "GyroReader.hpp"
#include "LocalBrowser.hpp"

// Connects to a GameRoom process. Behaviour depends on assigned role:
//   Performer    — reads GyroReader and streams gyro_data to server
//   Communicator — receives game_state and pushes it to LocalBrowser

class GameClient {
public:
    GameClient(const std::string& host, int port,
               const std::string& role,
               GyroReader* gyro,        // may be nullptr for communicator
               LocalBrowser* browser);
    ~GameClient();

    void run();  // blocking — returns when game is over

    static int wsCallback(lws* wsi, lws_callback_reasons reason,
                          void* user, void* in, size_t len);
private:
    void onConnect(lws* wsi);
    void onMessage(const std::string& raw);
    void onWritable(lws* wsi);
    void onError();

    void sendGyroIfDue();   // called from the lws loop in performer mode
    void enqueue(const json& msg);

    std::string   host_;
    int           port_;
    std::string   role_;
    GyroReader*   gyro_;
    LocalBrowser* browser_;

    bool          running_  = false;
    lws_context*  context_  = nullptr;
    lws*          wsi_      = nullptr;

    std::queue<std::string> outQueue_;

    // Gyro send timing
    double lastGyroSendMs_ = 0.0;
    static constexpr double GYRO_INTERVAL_MS = 25.0;  // ~40Hz
};
