#include "LobbyClient.hpp"
#include <iostream>
#include <cstring>

static LobbyClient* s_lobby = nullptr;

LobbyClient::LobbyClient(const std::string& host, int port)
    : host_(host), port_(port)
{
    s_lobby = this;

    lws_protocols protocols[] = {
        { "rppi-lobby", LobbyClient::wsCallback, 0, 4096, 0, nullptr, 0 },
        { nullptr, nullptr, 0, 0, 0, nullptr, 0 }
    };

    lws_context_creation_info info{};
    info.port      = CONTEXT_PORT_NO_LISTEN;
    info.protocols = protocols;

    context_ = lws_create_context(&info);
    if (!context_) throw std::runtime_error("LobbyClient: failed to create context");
}

LobbyClient::~LobbyClient() {
    if (context_) lws_context_destroy(context_);
}

void LobbyClient::createRoom(const std::string& username) {
    intent_   = Intent::CREATE;
    username_ = username;
}

void LobbyClient::joinRoom(const std::string& code, const std::string& username) {
    intent_   = Intent::JOIN;
    joinCode_ = code;
    username_ = username;
}

void LobbyClient::run() {
    lws_client_connect_info ccinfo{};
    ccinfo.context  = context_;
    ccinfo.address  = host_.c_str();
    ccinfo.port     = port_;
    ccinfo.path     = "/";
    ccinfo.host     = host_.c_str();
    ccinfo.origin   = host_.c_str();
    ccinfo.protocol = "rppi-lobby";

    lws_client_connect_via_info(&ccinfo);

    running_ = true;
    while (running_)
        lws_service(context_, 50);
}

// Called from any thread — enqueues START_GAME and wakes the lws loop
void LobbyClient::requestStart() {
    {
        std::lock_guard<std::mutex> lk(queueMutex_);
        outQueue_.push(MsgStartGame::build().dump());
        pendingWake_ = true;
    }
    lws_cancel_service(context_);  // thread-safe — wakes lws_service()
}

// ── Static lws callback ───────────────────────────────────────────────────────

int LobbyClient::wsCallback(lws* wsi, lws_callback_reasons reason,
                             void* /*user*/, void* in, size_t len) {
    if (!s_lobby) return 0;
    switch (reason) {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            s_lobby->onConnect(wsi);
            break;
        case LWS_CALLBACK_CLIENT_RECEIVE:
            s_lobby->onMessage(std::string(static_cast<char*>(in), len));
            break;
        case LWS_CALLBACK_CLIENT_WRITEABLE:
            s_lobby->onWritable(wsi);
            break;
        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            s_lobby->onError();
            break;
        case LWS_CALLBACK_EVENT_WAIT_CANCELLED:
            s_lobby->onWaitCancelled();
            break;
        default:
            break;
    }
    return 0;
}

// ── Private handlers ──────────────────────────────────────────────────────────

void LobbyClient::onConnect(lws* wsi) {
    wsi_ = wsi;
    std::cout << "[Lobby] Connected to server\n";

    std::lock_guard<std::mutex> lk(queueMutex_);
    if (intent_ == Intent::CREATE)
        outQueue_.push(MsgCreateRoom::build(username_).dump());
    else
        outQueue_.push(MsgJoinRoom::build(joinCode_, username_).dump());

    lws_callback_on_writable(wsi_);
}

void LobbyClient::onMessage(const std::string& raw) {
    std::string type;
    try { type = getMsgType(raw); }
    catch (...) { std::cerr << "[Lobby] Bad JSON: " << raw << "\n"; return; }

    const json j = json::parse(raw);

    if (type == MsgType::ROOM_CREATED) {
        roomCode_ = MsgRoomCreated::parse(j).code;
        std::cout << "[Lobby] Room created: " << roomCode_ << "\n";
        if (onRoomCreated_) onRoomCreated_(roomCode_);

    } else if (type == MsgType::ROOM_JOINED) {
        roomCode_ = MsgRoomJoined::parse(j).code;
        std::cout << "[Lobby] Room joined: " << roomCode_ << "\n";
        // Guest side — room code stored; player count update comes via PLAYER_JOINED

    } else if (type == MsgType::PLAYER_JOINED) {
        auto msg = MsgPlayerJoined::parse(j);
        std::cout << "[Lobby] Players: " << msg.hostName << " & " << msg.guestName << "\n";
        if (onPlayerJoined_) onPlayerJoined_(msg.count, msg.hostName, msg.guestName);

    } else if (type == MsgType::ROLE_ASSIGNED) {
        auto msg  = MsgRoleAssigned::parse(j);
        role_     = msg.role;
        gamePort_ = msg.gamePort;
        std::cout << "[Lobby] Role: " << role_ << " — game port " << gamePort_ << "\n";
        if (onRoleAssigned_) onRoleAssigned_(role_, gamePort_);
        running_ = false;

    } else if (type == MsgType::ERROR) {
        std::string msg = MsgError::parse(j).message;
        std::cerr << "[Lobby] Server error: " << msg << "\n";
        if (onError_) onError_(msg);
        running_ = false;
    }
}

void LobbyClient::onWritable(lws* wsi) {
    std::lock_guard<std::mutex> lk(queueMutex_);
    if (outQueue_.empty()) return;

    const std::string& msg = outQueue_.front();
    std::vector<unsigned char> buf(LWS_PRE + msg.size());
    std::memcpy(buf.data() + LWS_PRE, msg.data(), msg.size());
    lws_write(wsi, buf.data() + LWS_PRE, msg.size(), LWS_WRITE_TEXT);
    outQueue_.pop();

    if (!outQueue_.empty()) lws_callback_on_writable(wsi);
}

void LobbyClient::onError() {
    std::cerr << "[Lobby] Connection error\n";
    if (onError_) onError_("Could not connect to server");
    running_ = false;
}

// Called in the lws thread after lws_cancel_service() wakes the loop
void LobbyClient::onWaitCancelled() {
    if (pendingWake_.exchange(false) && wsi_)
        lws_callback_on_writable(wsi_);
}
