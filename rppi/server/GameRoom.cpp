#include "GameRoom.hpp"
#include "GyroPiGame.hpp"

#include <iostream>
#include <chrono>
#include <cstring>

static GameRoom* s_instance = nullptr;

// ── Constructor / Destructor ─────────────────────────────────────────────────

GameRoom::GameRoom(const std::string& roomCode, int port)
    : roomCode_(roomCode), port_(port)
{
    s_instance = this;

    lws_protocols protocols[] = {
        { "rppi-game", GameRoom::wsCallback, 0, 4096, 0, nullptr, 0 },
        { nullptr, nullptr, 0, 0, 0, nullptr, 0 }
    };

    lws_context_creation_info info{};
    info.port      = port_;
    info.protocols = protocols;

    context_ = lws_create_context(&info);
    if (!context_)
        throw std::runtime_error("Failed to create GameRoom context");

    std::cout << "[Room " << roomCode_ << "] Listening on port " << port_ << "\n";
}

GameRoom::~GameRoom() {
    if (context_) lws_context_destroy(context_);
}

// ── Public interface ─────────────────────────────────────────────────────────

void GameRoom::run() {
    running_ = true;

    while (running_) {
        lws_service(context_, 5);   // 5ms max — keeps loop responsive

        // If game loop has produced a new state, broadcast it from this thread
        if (stateDirty_.exchange(false)) {
            broadcastGameState();
        }
    }

    if (gameThread_.joinable())
        gameThread_.join();
}

void GameRoom::stop() {
    running_ = false;
}

// ── Static lws callback ──────────────────────────────────────────────────────

int GameRoom::wsCallback(lws* wsi, lws_callback_reasons reason,
                          void* /*user*/, void* in, size_t len) {
    if (!s_instance) return 0;

    switch (reason) {
        case LWS_CALLBACK_ESTABLISHED:
            s_instance->onConnect(wsi);
            break;
        case LWS_CALLBACK_RECEIVE:
            s_instance->onMessage(wsi, std::string(static_cast<char*>(in), len));
            break;
        case LWS_CALLBACK_SERVER_WRITEABLE:
            s_instance->onWritable(wsi);
            break;
        case LWS_CALLBACK_CLOSED:
            s_instance->onDisconnect(wsi);
            break;
        default:
            break;
    }
    return 0;
}

// ── Event handlers ───────────────────────────────────────────────────────────

void GameRoom::onConnect(lws* wsi) {
    writeQueues_[wsi];  // initialise empty queue
    std::cout << "[Room " << roomCode_ << "] Client connected, waiting for identify\n";
}

void GameRoom::onMessage(lws* wsi, const std::string& raw) {
    std::string type;
    try { type = getMsgType(raw); }
    catch (...) { sendTo(wsi, MsgError::build("Invalid JSON")); return; }

    const json j = json::parse(raw);

    if      (type == MsgType::IDENTIFY)  handleIdentify(wsi, j);
    else if (type == MsgType::GYRO_DATA) handleGyroData(wsi, j);
    else    sendTo(wsi, MsgError::build("Unknown message type in game: " + type));
}

void GameRoom::onWritable(lws* wsi) {
    auto& queue = writeQueues_[wsi];
    if (queue.empty()) return;

    const std::string& msg = queue.front();
    std::vector<unsigned char> buf(LWS_PRE + msg.size());
    std::memcpy(buf.data() + LWS_PRE, msg.data(), msg.size());
    lws_write(wsi, buf.data() + LWS_PRE, msg.size(), LWS_WRITE_TEXT);

    queue.pop();
    if (!queue.empty())
        lws_callback_on_writable(wsi);
}

void GameRoom::onDisconnect(lws* wsi) {
    if (wsi == performer_)    performer_    = nullptr;
    if (wsi == communicator_) communicator_ = nullptr;
    writeQueues_.erase(wsi);
    stop();  // one player disconnected — end the game
    std::cout << "[Room " << roomCode_ << "] A player disconnected, shutting down\n";
}

// ── Game message handlers ────────────────────────────────────────────────────

void GameRoom::handleIdentify(lws* wsi, const json& j) {
    auto msg = MsgIdentify::parse(j);

    if (msg.role == Role::PERFORMER && !performer_) {
        performer_ = wsi;
        std::cout << "[Room " << roomCode_ << "] Performer identified\n";
    } else if (msg.role == Role::COMMUNICATOR && !communicator_) {
        communicator_ = wsi;
        std::cout << "[Room " << roomCode_ << "] Communicator identified\n";
    } else {
        sendTo(wsi, MsgError::build("Invalid or duplicate role: " + msg.role));
        return;
    }

    // Start game once both players are identified
    if (bothPlayersConnected()) {
        game_ = std::make_unique<GyroPiGame>();
        gameThread_ = std::thread(&GameRoom::gameLoopThread, this);
        std::cout << "[Room " << roomCode_ << "] Both players ready — game starting\n";
    }
}

void GameRoom::handleGyroData(lws* wsi, const json& j) {
    if (wsi != performer_) return;  // ignore gyro from wrong client

    auto msg = MsgGyroData::parse(j);
    std::lock_guard<std::mutex> lock(gyroMutex_);
    gyro_.pitch = msg.pitch;
    gyro_.roll  = msg.roll;
}

// ── Game loop ────────────────────────────────────────────────────────────────

void GameRoom::gameLoopThread() {
    using clock = std::chrono::steady_clock;
    constexpr auto targetDt = std::chrono::milliseconds(25);  // ~40Hz

    auto lastTime = clock::now();

    while (running_ && game_ && !game_->isFinished()) {
        auto now = clock::now();
        float dt = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;

        // Read latest gyro input
        GyroInput gyro;
        {
            std::lock_guard<std::mutex> lock(gyroMutex_);
            gyro = gyro_;
        }

        // Advance game physics
        game_->update(gyro.pitch, gyro.roll, dt);

        // Package new state for broadcast
        GameState state = game_->getState();
        {
            std::lock_guard<std::mutex> lock(stateMutex_);
            pendingState_ = MsgGameState{
                state.ballX, state.ballY,
                gyro.pitch, gyro.roll,
                state.level
            };
        }
        stateDirty_ = true;

        std::this_thread::sleep_until(lastTime + targetDt);
    }

    // Game finished — send result
    if (game_) {
        std::lock_guard<std::mutex> lock(stateMutex_);
        pendingState_.ballX = -1;  // sentinel: signals game over
    }
    stateDirty_ = true;
}

// ── Helpers ──────────────────────────────────────────────────────────────────

void GameRoom::broadcastGameState() {
    MsgGameState state;
    {
        std::lock_guard<std::mutex> lock(stateMutex_);
        state = pendingState_;
    }

    // Check for game over sentinel
    if (state.ballX < 0) {
        bool win = game_ && game_->isWon();
        if (performer_)    sendTo(performer_,    MsgGameOver::build(win));
        if (communicator_) sendTo(communicator_, MsgGameOver::build(win));
        stop();
        return;
    }

    json stateMsg = MsgGameState::build(
        state.ballX, state.ballY, state.pitch, state.roll, state.level);

    if (performer_)    sendTo(performer_,    stateMsg);
    if (communicator_) sendTo(communicator_, stateMsg);
}

bool GameRoom::bothPlayersConnected() const {
    return performer_ != nullptr && communicator_ != nullptr;
}

void GameRoom::sendTo(lws* wsi, const json& msg) {
    auto& q = writeQueues_[wsi];
    while (!q.empty()) q.pop();  // keep only latest state
    q.push(msg.dump());
    lws_callback_on_writable(wsi);
}
