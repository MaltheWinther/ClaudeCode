#include "GameRoom.hpp"
#include "GyroPiGame.hpp"
#include "Leaderboard.hpp"

#include <iostream>
#include <chrono>
#include <ctime>
#include <QCoreApplication>

// ── Constructor / Destructor ─────────────────────────────────────────────────

GameRoom::GameRoom(const std::string& roomCode, int port,
                   const std::string& hostName, const std::string& guestName,
                   const std::string& gameType, QObject* parent)
    : QObject(parent), roomCode_(roomCode), port_(port),
      players_(hostName.empty() ? "Unknown" : hostName + " & " + guestName),
      gameType_(gameType)
{
    tcpServer_ = new QTcpServer(this);
    connect(tcpServer_, &QTcpServer::newConnection,
            this, &GameRoom::onNewConnection);

    if (!tcpServer_->listen(QHostAddress::Any, port_)) {
        throw std::runtime_error("Failed to listen on port " + std::to_string(port_));
    }

    // Poll timer replaces the while-loop in the old run()
    pollTimer_ = new QTimer(this);
    pollTimer_->setInterval(5);
    connect(pollTimer_, &QTimer::timeout, this, &GameRoom::onPollTick);
    pollTimer_->start();

    std::cout << "[Room " << roomCode_ << "] Listening on port " << port_ << "\n";
}

GameRoom::~GameRoom() {
    if (gameThread_.joinable())
        gameThread_.join();
}

void GameRoom::stop() {
    pollTimer_->stop();
    tcpServer_->close();
    if (gameThread_.joinable()) {
        // Signal game loop to stop, then join
        stateDirty_ = false;  // prevent further broadcasts
        game_.reset();         // causes gameLoopThread to exit
    }
    QCoreApplication::quit();
}

// ── Poll tick (replaces blocking run() loop) ─────────────────────────────────

void GameRoom::onPollTick() {
    if (stateDirty_.exchange(false)) {
        broadcastGameState();
    }

    if (pendingStopCountdown_ > 0) {
        if (--pendingStopCountdown_ == 0) {
            stop();
        }
    }
}

// ── Connection handling ──────────────────────────────────────────────────────

void GameRoom::onNewConnection() {
    while (QTcpSocket* socket = tcpServer_->nextPendingConnection()) {
        readers_[socket];
        connect(socket, &QTcpSocket::readyRead,
                this, &GameRoom::onClientReadyRead);
        connect(socket, &QTcpSocket::disconnected,
                this, &GameRoom::onClientDisconnected);
        std::cout << "[Room " << roomCode_ << "] Client connected, waiting for identify\n";
    }
}

void GameRoom::onClientReadyRead() {
    auto* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;

    auto it = readers_.find(socket);
    if (it == readers_.end()) return;

    auto messages = it->second.feed(socket->readAll());
    for (const auto& raw : messages) {
        onMessage(socket, raw);
    }
}

void GameRoom::onClientDisconnected() {
    auto* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;

    QTcpSocket* remaining = nullptr;
    if (socket == performer_) {
        performer_ = nullptr;
        remaining  = communicator_;
    } else if (socket == communicator_) {
        communicator_ = nullptr;
        remaining     = performer_;
    }

    readers_.erase(socket);
    socket->deleteLater();

    if (remaining) {
        queueTo(remaining, MsgError::build("The other player disconnected"));
        pendingStopCountdown_ = 20;
    } else {
        stop();
    }
    std::cout << "[Room " << roomCode_ << "] A player disconnected, shutting down\n";
}

// ── Message dispatch ─────────────────────────────────────────────────────────

void GameRoom::onMessage(QTcpSocket* socket, const std::string& raw) {
    std::string type;
    try { type = getMsgType(raw); }
    catch (...) { sendTo(socket, MsgError::build("Invalid JSON")); return; }

    const json j = json::parse(raw);

    if      (type == MsgType::IDENTIFY)    handleIdentify(socket, j);
    else if (type == MsgType::GYRO_DATA)   handleGyroData(socket, j);
    else if (type == MsgType::NOTE_SUBMIT) handleNoteSubmit(socket, j);
    else    sendTo(socket, MsgError::build("Unknown message type in game: " + type));
}

// ── Game message handlers ────────────────────────────────────────────────────

void GameRoom::handleIdentify(QTcpSocket* socket, const json& j) {
    auto msg = MsgIdentify::parse(j);

    if (msg.role == Role::PERFORMER && !performer_) {
        performer_ = socket;
        std::cout << "[Room " << roomCode_ << "] Performer identified\n";
    } else if (msg.role == Role::COMMUNICATOR && !communicator_) {
        communicator_ = socket;
        std::cout << "[Room " << roomCode_ << "] Communicator identified\n";
    } else {
        sendTo(socket, MsgError::build("Invalid or duplicate role: " + msg.role));
        return;
    }

    if (bothPlayersConnected()) {
        gameStartTime_ = clock::now();
        if (gameType_ == "notepi") {
            notePi_ = std::make_unique<NotePiGame>();
            std::cout << "[Room " << roomCode_ << "] Both players ready — NotePi starting\n";
        } else {
            game_ = std::make_unique<GyroPiGame>();
            gameThread_ = std::thread(&GameRoom::gameLoopThread, this);
            std::cout << "[Room " << roomCode_ << "] Both players ready — GyroPi starting\n";
        }
    }
}

void GameRoom::handleGyroData(QTcpSocket* socket, const json& j) {
    if (socket != performer_) return;

    auto msg = MsgGyroData::parse(j);
    std::lock_guard<std::mutex> lock(gyroMutex_);
    gyro_.pitch = msg.pitch;
    gyro_.roll  = msg.roll;
}

void GameRoom::handleNoteSubmit(QTcpSocket* socket, const json& j) {
    if (socket != performer_) return;
    if (!notePi_ || notePi_->isFinished()) return;

    auto msg = MsgNoteSubmit::parse(j);
    if ((int)msg.notes.size() != 5) {
        sendTo(socket, MsgError::build("Must submit exactly 5 notes"));
        return;
    }

    auto result = notePi_->submitGuess(msg.notes);

    auto stateMsg = MsgNoteState::build(
        notePi_->attemptCount(), msg.notes, result.colors,
        result.correct, notePi_->history());

    if (performer_)    queueTo(performer_,    stateMsg);
    if (communicator_) queueTo(communicator_, stateMsg);

    if (result.correct) {
        int elapsed = (int)std::chrono::duration<double>(
                          clock::now() - gameStartTime_).count();
        int attempts = notePi_->attemptCount();

        LeaderboardEntry entry{ players_, elapsed, attempts, true, currentDate(), gameType_ };
        Leaderboard::addEntry(entry);
        auto top = Leaderboard::topN(10, gameType_);

        auto overMsg = MsgGameOver::build(true, elapsed, attempts, top);
        if (performer_)    queueTo(performer_,    overMsg);
        if (communicator_) queueTo(communicator_, overMsg);

        pendingStopCountdown_ = 20;
    }
}

// ── Game loop ────────────────────────────────────────────────────────────────

void GameRoom::gameLoopThread() {
    using clock = std::chrono::steady_clock;
    constexpr auto targetDt = std::chrono::milliseconds(25);

    auto lastTime = clock::now();

    while (game_ && !game_->isFinished()) {
        auto now = clock::now();
        float dt = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;

        GyroInput gyro;
        {
            std::lock_guard<std::mutex> lock(gyroMutex_);
            gyro = gyro_;
        }

        game_->update(gyro.pitch, gyro.roll, dt);

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

    if (game_) {
        std::lock_guard<std::mutex> lock(stateMutex_);
        pendingState_.ballX = -1;
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

    if (state.ballX < 0) {
        int  elapsed = (int)std::chrono::duration<double>(
                           clock::now() - gameStartTime_).count();
        int  levels  = game_ ? game_->getState().level : 1;
        bool win     = game_ && game_->isWon();

        LeaderboardEntry entry{ players_, elapsed, levels, win, currentDate(), gameType_ };
        Leaderboard::addEntry(entry);
        auto top = Leaderboard::topN(10, gameType_);

        auto msg = MsgGameOver::build(win, elapsed, levels, top);
        if (performer_)    queueTo(performer_,    msg);
        if (communicator_) queueTo(communicator_, msg);
        pendingStopCountdown_ = 20;
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

void GameRoom::sendTo(QTcpSocket* socket, const json& msg) {
    socket->write(frameMessage(msg.dump()));
}

void GameRoom::queueTo(QTcpSocket* socket, const json& msg) {
    socket->write(frameMessage(msg.dump()));
}

std::string GameRoom::currentDate() const {
    std::time_t t = std::time(nullptr);
    char buf[11];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d", std::localtime(&t));
    return buf;
}
