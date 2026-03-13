#include "GameClient.hpp"
#include <iostream>

GameClient::GameClient(const std::string& host, int port,
                       const std::string& role,
                       GyroReader* gyro,
                       QObject* parent)
    : QObject(parent), host_(host), port_(port), role_(role), gyro_(gyro)
{
}

void GameClient::connectToServer() {
    socket_ = new QTcpSocket(this);
    connect(socket_, &QTcpSocket::connected,
            this, &GameClient::onConnected);
    connect(socket_, &QTcpSocket::readyRead,
            this, &GameClient::onReadyRead);
    connect(socket_, &QTcpSocket::disconnected,
            this, &GameClient::onDisconnected);
    connect(socket_, &QAbstractSocket::errorOccurred,
            this, &GameClient::onSocketError);

    socket_->connectToHost(QString::fromStdString(host_), port_);
}

void GameClient::stop() {
    if (gyroTimer_) {
        gyroTimer_->stop();
        gyroTimer_ = nullptr;
    }
    if (socket_) {
        socket_->disconnectFromHost();
        socket_->deleteLater();
        socket_ = nullptr;
    }
}

void GameClient::submitNotes(const std::vector<std::string>& notes) {
    if (!socket_) return;
    socket_->write(frameMessage(MsgNoteSubmit::build(notes).dump()));
}

// ── Slots ────────────────────────────────────────────────────────────────────

void GameClient::onConnected() {
    std::cout << "[Game] Connected as " << role_ << "\n";
    socket_->write(frameMessage(MsgIdentify::build(role_).dump()));

    // Start gyro streaming for performer (GyroPi)
    if (role_ == Role::PERFORMER && gyro_) {
        gyroTimer_ = new QTimer(this);
        gyroTimer_->setInterval(25);  // ~40 Hz
        connect(gyroTimer_, &QTimer::timeout, this, &GameClient::onGyroTick);
        gyroTimer_->start();
    }
}

void GameClient::onReadyRead() {
    if (!socket_) return;
    auto messages = reader_.feed(socket_->readAll());
    for (const auto& raw : messages) {
        onMessage(raw);
    }
}

void GameClient::onDisconnected() {
    if (!gameOver_ && onError_cb_) {
        onError_cb_("Connection closed unexpectedly");
    }
}

void GameClient::onSocketError(QAbstractSocket::SocketError /*err*/) {
    std::cerr << "[Game] Connection error\n";
    if (onError_cb_) onError_cb_("Could not connect to game server");
}

void GameClient::onGyroTick() {
    if (!socket_ || !gyro_) return;
    auto angles = gyro_->read();
    socket_->write(frameMessage(MsgGyroData::build(angles.pitch, angles.roll).dump()));
}

// ── Message handling ─────────────────────────────────────────────────────────

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
        gameOver_ = true;
        onError_cb_ = nullptr;
        if (onGameOver_)
            onGameOver_(msg.win, msg.elapsedSecs, msg.levelsReached, msg.leaderboard);

    } else if (type == MsgType::NOTE_STATE) {
        if (onNoteState_) onNoteState_(MsgNoteState::parse(j));

    } else if (type == MsgType::ERROR) {
        auto errMsg = MsgError::parse(j).message;
        std::cerr << "[Game] Error: " << errMsg << "\n";
        if (onError_cb_) onError_cb_(errMsg);
    }
}
