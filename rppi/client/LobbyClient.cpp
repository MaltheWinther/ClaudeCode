#include "LobbyClient.hpp"
#include <iostream>

LobbyClient::LobbyClient(const std::string& host, int port, QObject* parent)
    : QObject(parent), host_(host), port_(port)
{
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

void LobbyClient::connectToServer() {
    socket_ = new QTcpSocket(this);
    connect(socket_, &QTcpSocket::connected,
            this, &LobbyClient::onConnected);
    connect(socket_, &QTcpSocket::readyRead,
            this, &LobbyClient::onReadyRead);
    connect(socket_, &QTcpSocket::disconnected,
            this, &LobbyClient::onDisconnected);
    connect(socket_, &QAbstractSocket::errorOccurred,
            this, &LobbyClient::onSocketError);

    socket_->connectToHost(QString::fromStdString(host_), port_);
}

void LobbyClient::stop() {
    if (socket_) {
        socket_->disconnectFromHost();
        socket_->deleteLater();
        socket_ = nullptr;
    }
}

void LobbyClient::requestStart() {
    if (!socket_) return;
    socket_->write(frameMessage(MsgStartGame::build().dump()));
}

void LobbyClient::requestStartWithGame(const std::string& gameType) {
    if (!socket_) return;
    socket_->write(frameMessage(MsgStartGame::build(gameType).dump()));
}

// ── Slots ────────────────────────────────────────────────────────────────────

void LobbyClient::onConnected() {
    std::cout << "[Lobby] Connected to server\n";

    if (intent_ == Intent::CREATE)
        socket_->write(frameMessage(MsgCreateRoom::build(username_).dump()));
    else
        socket_->write(frameMessage(MsgJoinRoom::build(joinCode_, username_).dump()));
}

void LobbyClient::onReadyRead() {
    auto messages = reader_.feed(socket_->readAll());
    for (const auto& raw : messages) {
        onMessage(raw);
    }
}

void LobbyClient::onDisconnected() {
    // Only report if we didn't already get role assigned
    if (gamePort_ < 0 && onError_) {
        onError_("Connection lost");
    }
}

void LobbyClient::onSocketError(QAbstractSocket::SocketError /*err*/) {
    std::cerr << "[Lobby] Connection error\n";
    if (onError_) onError_("Could not connect to server");
}

// ── Message handling ─────────────────────────────────────────────────────────

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

    } else if (type == MsgType::PLAYER_JOINED) {
        auto msg = MsgPlayerJoined::parse(j);
        std::cout << "[Lobby] Players: " << msg.hostName << " & " << msg.guestName << "\n";
        if (onPlayerJoined_) onPlayerJoined_(msg.count, msg.hostName, msg.guestName);

    } else if (type == MsgType::ROLE_ASSIGNED) {
        auto msg  = MsgRoleAssigned::parse(j);
        role_     = msg.role;
        gamePort_ = msg.gamePort;
        std::cout << "[Lobby] Role: " << role_ << " — game port " << gamePort_
                  << " — game " << msg.game << "\n";
        if (onRoleAssigned_) onRoleAssigned_(role_, gamePort_, msg.game);

    } else if (type == MsgType::ERROR) {
        std::string msg = MsgError::parse(j).message;
        std::cerr << "[Lobby] Server error: " << msg << "\n";
        if (onError_) onError_(msg);
    }
}
