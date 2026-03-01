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
    info.port      = CONTEXT_PORT_NO_LISTEN;  // client — no listening port
    info.protocols = protocols;

    context_ = lws_create_context(&info);
    if (!context_) throw std::runtime_error("LobbyClient: failed to create context");
}

LobbyClient::~LobbyClient() {
    if (context_) lws_context_destroy(context_);
}

void LobbyClient::createRoom() { intent_ = Intent::CREATE; }

void LobbyClient::joinRoom(const std::string& code) {
    intent_   = Intent::JOIN;
    joinCode_ = code;
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
        default:
            break;
    }
    return 0;
}

void LobbyClient::onConnect(lws* wsi) {
    wsi_ = wsi;
    std::cout << "[Lobby] Connected to server\n";

    // Send the first lobby action immediately
    if (intent_ == Intent::CREATE)
        enqueue(MsgCreateRoom::build());
    else
        enqueue(MsgJoinRoom::build(joinCode_));

    lws_callback_on_writable(wsi_);
}

void LobbyClient::onMessage(const std::string& raw) {
    std::string type;
    try { type = getMsgType(raw); }
    catch (...) { std::cerr << "[Lobby] Bad JSON: " << raw << "\n"; return; }

    const json j = json::parse(raw);

    if (type == MsgType::ROOM_CREATED) {
        roomCode_ = MsgRoomCreated::parse(j).code;
        std::cout << "[Lobby] Room created: " << roomCode_
                  << " — waiting for another player...\n";

    } else if (type == MsgType::ROOM_JOINED) {
        roomCode_ = MsgRoomJoined::parse(j).code;
        std::cout << "[Lobby] Joined room: " << roomCode_ << "\n";

        // If we joined someone else's room, trigger start
        if (intent_ == Intent::JOIN) {
            enqueue(MsgStartGame::build());
            lws_callback_on_writable(wsi_);
        }

    } else if (type == MsgType::ROLE_ASSIGNED) {
        auto msg  = MsgRoleAssigned::parse(j);
        role_     = msg.role;
        gamePort_ = msg.gamePort;
        std::cout << "[Lobby] Role: " << role_
                  << " — game on port " << gamePort_ << "\n";
        running_ = false;  // unblock run()

    } else if (type == MsgType::ERROR) {
        std::cerr << "[Lobby] Server error: " << MsgError::parse(j).message << "\n";
        running_ = false;
    }
}

void LobbyClient::onWritable(lws* wsi) {
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
    running_ = false;
}

void LobbyClient::enqueue(const json& msg) {
    outQueue_.push(msg.dump());
}
