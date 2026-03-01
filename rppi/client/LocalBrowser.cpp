#include "LocalBrowser.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <algorithm>

static LocalBrowser* s_browser = nullptr;

// ── Constructor / Destructor ─────────────────────────────────────────────────

LocalBrowser::LocalBrowser(int port, const std::string& htmlPath)
    : port_(port), htmlPath_(htmlPath)
{
    s_browser = this;

    // Cache HTML file content at startup
    std::ifstream f(htmlPath_);
    if (!f) throw std::runtime_error("Cannot open HTML file: " + htmlPath_);
    std::ostringstream ss;
    ss << f.rdbuf();
    htmlContent_ = ss.str();
}

LocalBrowser::~LocalBrowser() { stop(); }

// ── Public interface ─────────────────────────────────────────────────────────

void LocalBrowser::start() {
    lws_protocols protocols[] = {
        // HTTP protocol — serves the HTML file
        { "http", LocalBrowser::wsCallback, 0, 0, 0, nullptr, 0 },
        // WebSocket protocol — receives live updates from C++
        { "rppi-display", LocalBrowser::wsCallback, 0, 4096, 0, nullptr, 0 },
        { nullptr, nullptr, 0, 0, 0, nullptr, 0 }
    };

    lws_context_creation_info info{};
    info.port      = port_;
    info.protocols = protocols;

    context_ = lws_create_context(&info);
    if (!context_) throw std::runtime_error("LocalBrowser: failed to create context");

    running_ = true;
    thread_  = std::thread(&LocalBrowser::loop, this);
    std::cout << "[Browser] Serving on port " << port_ << "\n";
}

void LocalBrowser::stop() {
    running_ = false;
    if (thread_.joinable()) thread_.join();
    if (context_) { lws_context_destroy(context_); context_ = nullptr; }
}

void LocalBrowser::push(const json& msg) {
    {
        std::lock_guard<std::mutex> lock(pendingMutex_);
        while (!pending_.empty()) pending_.pop();  // keep only latest
        pending_.push(msg.dump());
    }
    dirty_ = true;
    if (context_) lws_cancel_service(context_);  // wake lws thread
}

// ── Static lws callback ──────────────────────────────────────────────────────

int LocalBrowser::wsCallback(lws* wsi, lws_callback_reasons reason,
                              void* /*user*/, void* in, size_t len) {
    if (!s_browser) return 0;

    switch (reason) {
        case LWS_CALLBACK_HTTP:
            s_browser->onHttpRequest(wsi);
            break;

        case LWS_CALLBACK_HTTP_FILE_COMPLETION:
            return -1;  // close connection once file is fully sent

        case LWS_CALLBACK_ESTABLISHED:
            s_browser->onBrowserConnect(wsi);
            break;

        case LWS_CALLBACK_CLOSED:
            s_browser->onBrowserDisconnect(wsi);
            break;

        case LWS_CALLBACK_SERVER_WRITEABLE:
            s_browser->onWritable(wsi);
            break;

        default:
            break;
    }
    return 0;
}

// ── Event handlers ───────────────────────────────────────────────────────────

void LocalBrowser::onHttpRequest(lws* wsi) {
    if (lws_serve_http_file(wsi, htmlPath_.c_str(), "text/html", nullptr, 0) < 0)
        std::cerr << "[Browser] Failed to serve " << htmlPath_ << "\n";
}

void LocalBrowser::onBrowserConnect(lws* wsi) {
    std::lock_guard<std::mutex> lock(writeMutex_);
    browsers_.push_back(wsi);
    writeClients_.push_back(wsi);
    writeQueues_.emplace_back();
    std::cout << "[Browser] Browser connected\n";
}

void LocalBrowser::onBrowserDisconnect(lws* wsi) {
    std::lock_guard<std::mutex> lock(writeMutex_);
    auto it = std::find(writeClients_.begin(), writeClients_.end(), wsi);
    if (it != writeClients_.end()) {
        size_t idx = it - writeClients_.begin();
        writeClients_.erase(it);
        writeQueues_.erase(writeQueues_.begin() + idx);
    }
    browsers_.erase(std::remove(browsers_.begin(), browsers_.end(), wsi), browsers_.end());
}

void LocalBrowser::onWritable(lws* wsi) {
    std::lock_guard<std::mutex> lock(writeMutex_);
    auto it = std::find(writeClients_.begin(), writeClients_.end(), wsi);
    if (it == writeClients_.end()) return;

    auto& queue = writeQueues_[it - writeClients_.begin()];
    if (queue.empty()) return;

    const std::string& msg = queue.front();
    std::vector<unsigned char> buf(LWS_PRE + msg.size());
    std::memcpy(buf.data() + LWS_PRE, msg.data(), msg.size());
    lws_write(wsi, buf.data() + LWS_PRE, msg.size(), LWS_WRITE_TEXT);
    queue.pop();

    if (!queue.empty()) lws_callback_on_writable(wsi);
}

// ── Helpers ──────────────────────────────────────────────────────────────────

void LocalBrowser::loop() {
    while (running_) {
        lws_service(context_, 5);
        if (dirty_.exchange(false))
            broadcastPending();
    }
}

void LocalBrowser::broadcastPending() {
    std::queue<std::string> toSend;
    {
        std::lock_guard<std::mutex> lock(pendingMutex_);
        std::swap(toSend, pending_);
    }
    while (!toSend.empty()) {
        const std::string& msg = toSend.front();
        std::lock_guard<std::mutex> lock(writeMutex_);
        for (size_t i = 0; i < writeClients_.size(); ++i) {
            writeQueues_[i].push(msg);
            lws_callback_on_writable(writeClients_[i]);
        }
        toSend.pop();
    }
}
