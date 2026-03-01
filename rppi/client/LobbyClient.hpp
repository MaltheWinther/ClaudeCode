#pragma once

#include <string>
#include <queue>
#include <libwebsockets.h>
#include "../common/Messages.hpp"

// Connects to the LobbyServer, handles room create/join, and blocks in
// run() until a role is assigned. After run() returns, getRole() and
// getGamePort() give the results.

class LobbyClient {
public:
    LobbyClient(const std::string& host, int port);
    ~LobbyClient();

    void createRoom();              // call before run()
    void joinRoom(const std::string& code);  // call before run()
    void run();                     // blocking — returns when role is assigned

    std::string getRole()     const { return role_; }
    int         getGamePort() const { return gamePort_; }
    std::string getRoomCode() const { return roomCode_; }

    static int wsCallback(lws* wsi, lws_callback_reasons reason,
                          void* user, void* in, size_t len);
private:
    void onConnect(lws* wsi);
    void onMessage(const std::string& raw);
    void onWritable(lws* wsi);
    void onError();

    void sendQueued(lws* wsi);
    void enqueue(const json& msg);

    std::string host_;
    int         port_;
    bool        running_  = false;
    lws_context* context_ = nullptr;
    lws*         wsi_     = nullptr;

    // Queued outgoing messages (sent once connection is established)
    std::queue<std::string> outQueue_;

    // Set on connect: what to do first
    enum class Intent { CREATE, JOIN } intent_ = Intent::CREATE;
    std::string joinCode_;

    // Results — populated by onMessage
    std::string roomCode_;
    std::string role_;
    int         gamePort_ = -1;
};
