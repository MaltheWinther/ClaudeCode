#pragma once

#include <string>
#include <vector>
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include <libwebsockets.h>
#include "../common/Messages.hpp"

// Serves a single HTML file over HTTP and a WebSocket endpoint on the
// same port. The browser connects to the WebSocket to receive live updates.
// push() is thread-safe — call it from the game loop.

class LocalBrowser {
public:
    LocalBrowser(int port, const std::string& htmlPath);
    ~LocalBrowser();

    void start();   // spawns background thread
    void stop();

    void push(const json& msg);   // thread-safe — queues msg for browser

    static int wsCallback(lws* wsi, lws_callback_reasons reason,
                          void* user, void* in, size_t len);
private:
    void loop();

    void onHttpRequest(lws* wsi);
    void onBrowserConnect(lws* wsi);
    void onBrowserDisconnect(lws* wsi);
    void onWritable(lws* wsi);

    void sendTo(lws* wsi, const std::string& msg);
    void broadcastPending();

    int         port_;
    std::string htmlPath_;
    std::string htmlContent_;   // cached file content

    std::thread       thread_;
    std::atomic<bool> running_{ false };
    lws_context*      context_ = nullptr;

    std::vector<lws*>  browsers_;       // connected browser WebSocket clients
    std::queue<std::string> pending_;   // messages waiting to be broadcast
    std::mutex              pendingMutex_;
    std::atomic<bool>       dirty_{ false };

    std::mutex writeMutex_;
    std::vector<std::queue<std::string>> writeQueues_;  // per-browser send queue
    std::vector<lws*> writeClients_;                    // parallel to writeQueues_
};
