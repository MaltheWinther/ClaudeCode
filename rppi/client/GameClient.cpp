#include "GameClient.hpp"
#include <iostream>
#include <cstring>
#include <chrono>

static GameClient* s_game = nullptr;

static double nowMs() {
    using namespace std::chrono;
    return duration<double, std::milli>(
        steady_clock::now().time_since_epoch()).count();
}

GameClient::GameClient(const std::string& host, int port,
                       const std::string& role,
                       GyroReader* gyro)
    : host_(host), port_(port), role_(role), gyro_(gyro)
{
    s_game = this;

    lws_protocols protocols[] = {
        { "rppi-game", GameClient::wsCallback, 0, 4096, 0, nullptr, 0 },
        { nullptr, nullptr, 0, 0, 0, nullptr, 0 }
    };

    lws_context_creation_info info{};
    info.port      = CONTEXT_PORT_NO_LISTEN;
    info.protocols = protocols;

    context_ = lws_create_context(&info);
    if (!context_) throw std::runtime_error("GameClient: failed to create context");
}

GameClient::~GameClient() {
    if (context_) lws_context_destroy(context_);
}

void GameClient::run() {
    lws_client_connect_info ccinfo{};
    ccinfo.context  = context_;
    ccinfo.address  = host_.c_str();
    ccinfo.port     = port_;
    ccinfo.path     = "/";
    ccinfo.host     = host_.c_str();
    ccinfo.origin   = host_.c_str();
    ccinfo.protocol = "rppi-game";

    lws_client_connect_via_info(&ccinfo);

    running_ = true;
    while (running_) {
        lws_service(context_, 5);
        if (role_ == Role::PERFORMER) sendGyroIfDue();
    }
}

int GameClient::wsCallback(lws* wsi, lws_callback_reasons reason,
                            void* /*user*/, void* in, size_t len) {
    if (!s_game) return 0;
    switch (reason) {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            s_game->onConnect(wsi);
            break;
        case LWS_CALLBACK_CLIENT_RECEIVE:
            s_game->onMessage(std::string(static_cast<char*>(in), len));
            break;
        case LWS_CALLBACK_CLIENT_WRITEABLE:
            s_game->onWritable(wsi);
            break;
        case LWS_CALLBACK_CLIENT_CLOSED:
            s_game->wsi_     = nullptr;
            s_game->running_ = false;
            break;
        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            s_game->onError();
            break;
        default:
            break;
    }
    return 0;
}

void GameClient::onConnect(lws* wsi) {
    wsi_ = wsi;
    std::cout << "[Game] Connected as " << role_ << "\n";
    enqueue(MsgIdentify::build(role_));
    lws_callback_on_writable(wsi_);
}

void GameClient::onMessage(const std::string& raw) {
    std::string type;
    try { type = getMsgType(raw); }
    catch (...) { return; }

    const json j = json::parse(raw);

    if (type == MsgType::GAME_STATE) {
        if (onGameState_) onGameState_(MsgGameState::parse(j));

    } else if (type == MsgType::GAME_OVER) {
        auto msg = MsgGameOver::parse(j);
        std::cout << "[Game] Game over — " << (msg.win ? "WIN" : "LOSS") << "\n";
        if (onGameOver_)
            onGameOver_(msg.win, msg.elapsedSecs, msg.levelsReached, msg.leaderboard);
        running_ = false;

    } else if (type == MsgType::ERROR) {
        auto errMsg = MsgError::parse(j).message;
        std::cerr << "[Game] Error: " << errMsg << "\n";
        if (onError_cb_) onError_cb_(errMsg);
        running_ = false;
    }
}

void GameClient::onWritable(lws* wsi) {
    if (outQueue_.empty()) return;

    const std::string& msg = outQueue_.front();
    std::vector<unsigned char> buf(LWS_PRE + msg.size());
    std::memcpy(buf.data() + LWS_PRE, msg.data(), msg.size());
    lws_write(wsi, buf.data() + LWS_PRE, msg.size(), LWS_WRITE_TEXT);
    outQueue_.pop();

    if (!outQueue_.empty()) lws_callback_on_writable(wsi);
}

void GameClient::onError() {
    std::cerr << "[Game] Connection error\n";
    if (onError_cb_) onError_cb_("The other player disconnected");
    running_ = false;
}

void GameClient::sendGyroIfDue() {
    if (!wsi_ || !gyro_) return;

    double now = nowMs();
    if (now - lastGyroSendMs_ < GYRO_INTERVAL_MS) return;
    lastGyroSendMs_ = now;

    auto angles = gyro_->read();
    enqueue(MsgGyroData::build(angles.pitch, angles.roll));
    lws_callback_on_writable(wsi_);
}

void GameClient::enqueue(const json& msg) {
    outQueue_.push(msg.dump());
}
