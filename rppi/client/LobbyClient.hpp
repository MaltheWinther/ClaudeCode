#pragma once

#include <string>
#include <queue>
#include <mutex>
#include <atomic>
#include <functional>
#include <libwebsockets.h>
#include "../common/Messages.hpp"

// Connects to LobbyServer, handles room create/join, and blocks in run()
// until a role is assigned (or an error occurs).
//
// Designed to run in a background thread.
// All callbacks are invoked from that thread — use Qt::QueuedConnection
// (via QMetaObject::invokeMethod) to safely update the UI from them.
//
// requestStart() is thread-safe and can be called from the Qt main thread.

class LobbyClient {
public:
    LobbyClient(const std::string& host, int port);
    ~LobbyClient();

    void createRoom(const std::string& username);
    void joinRoom(const std::string& code, const std::string& username);
    void run();          // blocking — call from a background thread
    void requestStart(); // thread-safe — sends START_GAME from any thread
    void requestStartWithGame(const std::string& gameType); // sends START_GAME with game type

    std::string getRoomCode() const { return roomCode_; }
    std::string getRole()     const { return role_; }
    int         getGamePort() const { return gamePort_; }

    // Callbacks (set before calling run())
    void setOnRoomCreated (std::function<void(std::string code)>                        cb) { onRoomCreated_  = std::move(cb); }
    void setOnPlayerJoined(std::function<void(int, std::string, std::string)>           cb) { onPlayerJoined_ = std::move(cb); }
    void setOnRoleAssigned(std::function<void(std::string, int port, std::string game)> cb) { onRoleAssigned_ = std::move(cb); }
    void setOnError       (std::function<void(std::string msg)>                         cb) { onError_        = std::move(cb); }

    static int wsCallback(lws* wsi, lws_callback_reasons reason,
                          void* user, void* in, size_t len);
private:
    void onConnect(lws* wsi);
    void onMessage(const std::string& raw);
    void onWritable(lws* wsi);
    void onError();
    void onWaitCancelled();

    std::string  host_;
    int          port_;
    bool         running_  = false;
    lws_context* context_  = nullptr;
    lws*         wsi_      = nullptr;

    std::queue<std::string> outQueue_;
    std::mutex              queueMutex_;
    std::atomic<bool>       pendingWake_{ false };

    enum class Intent { CREATE, JOIN } intent_ = Intent::CREATE;
    std::string joinCode_;
    std::string username_;

    // Results
    std::string roomCode_;
    std::string role_;
    int         gamePort_ = -1;

    // Callbacks
    std::function<void(std::string)>                     onRoomCreated_;
    std::function<void(int, std::string, std::string)>   onPlayerJoined_;
    std::function<void(std::string, int, std::string)>   onRoleAssigned_;
    std::function<void(std::string)>                     onError_;
};
