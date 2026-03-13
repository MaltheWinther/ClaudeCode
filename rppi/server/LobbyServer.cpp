#include "LobbyServer.hpp"

#include <iostream>
#include <random>
#include <unistd.h>   // fork, execl
#include <QCoreApplication>

// ── Constructor ──────────────────────────────────────────────────────────────

LobbyServer::LobbyServer(int port, QObject* parent)
    : QObject(parent)
{
    tcpServer_ = new QTcpServer(this);
    connect(tcpServer_, &QTcpServer::newConnection,
            this, &LobbyServer::onNewConnection);

    if (!tcpServer_->listen(QHostAddress::Any, port)) {
        throw std::runtime_error("Failed to listen on port " + std::to_string(port));
    }
    std::cout << "[Lobby] Listening on port " << port << "\n";
}

void LobbyServer::stop() {
    tcpServer_->close();
    QCoreApplication::quit();
}

// ── Connection handling ──────────────────────────────────────────────────────

void LobbyServer::onNewConnection() {
    while (QTcpSocket* socket = tcpServer_->nextPendingConnection()) {
        readers_[socket];  // initialise empty frame reader
        connect(socket, &QTcpSocket::readyRead,
                this, &LobbyServer::onClientReadyRead);
        connect(socket, &QTcpSocket::disconnected,
                this, &LobbyServer::onClientDisconnected);
        std::cout << "[Lobby] Client connected\n";
    }
}

void LobbyServer::onClientReadyRead() {
    auto* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;

    auto it = readers_.find(socket);
    if (it == readers_.end()) return;

    auto messages = it->second.feed(socket->readAll());
    for (const auto& raw : messages) {
        onMessage(socket, raw);
    }
}

void LobbyServer::onClientDisconnected() {
    auto* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;

    auto it = clientRooms_.find(socket);
    if (it != clientRooms_.end()) {
        const std::string& code = it->second;
        rooms_.erase(code);
        clientRooms_.erase(it);
        std::cout << "[Lobby] Client disconnected, room " << code << " removed\n";
    }
    readers_.erase(socket);
    socket->deleteLater();
}

// ── Message dispatch ─────────────────────────────────────────────────────────

void LobbyServer::onMessage(QTcpSocket* socket, const std::string& raw) {
    std::string type;
    try {
        type = getMsgType(raw);
    } catch (...) {
        sendTo(socket, MsgError::build("Invalid JSON"));
        return;
    }

    const json j = json::parse(raw);
    if      (type == MsgType::CREATE_ROOM) handleCreateRoom(socket, j);
    else if (type == MsgType::JOIN_ROOM)   handleJoinRoom(socket, j);
    else if (type == MsgType::START_GAME)  handleStartGame(socket, j);
    else    sendTo(socket, MsgError::build("Unknown message type: " + type));
}

// ── Lobby message handlers ───────────────────────────────────────────────────

void LobbyServer::handleCreateRoom(QTcpSocket* socket, const json& j) {
    if (clientRooms_.count(socket)) {
        sendTo(socket, MsgError::build("Already in a room"));
        return;
    }

    std::string hostName = j.value("username", "Host");

    std::string code = generateRoomCode();
    rooms_[code]       = { code, hostName, "", socket, nullptr, Room::State::WAITING };
    clientRooms_[socket] = code;

    sendTo(socket, MsgRoomCreated::build(code));
    std::cout << "[Lobby] Room created: " << code << " by " << hostName << "\n";
}

void LobbyServer::handleJoinRoom(QTcpSocket* socket, const json& j) {
    if (clientRooms_.count(socket)) {
        sendTo(socket, MsgError::build("Already in a room"));
        return;
    }

    std::string code;
    try { code = MsgJoinRoom::parse(j).code; }
    catch (...) { sendTo(socket, MsgError::build("Missing room code")); return; }

    auto it = rooms_.find(code);
    if (it == rooms_.end()) {
        sendTo(socket, MsgError::build("Room not found: " + code));
        return;
    }
    Room& room = it->second;
    if (room.state != Room::State::WAITING) {
        sendTo(socket, MsgError::build("Room is full or already started"));
        return;
    }

    std::string guestName = j.value("username", "Guest");

    room.guest     = socket;
    room.guestName = guestName;
    room.state     = Room::State::FULL;
    clientRooms_[socket] = code;

    auto pjMsg = MsgPlayerJoined::build(room.hostName, guestName, 2);
    sendTo(socket,    pjMsg);
    sendTo(room.host, pjMsg);

    sendTo(socket, MsgRoomJoined::build(code));

    std::cout << "[Lobby] Room " << code << " is now full ("
              << room.hostName << " & " << guestName << ")\n";
}

void LobbyServer::handleStartGame(QTcpSocket* socket, const json& j) {
    auto it = clientRooms_.find(socket);
    if (it == clientRooms_.end()) {
        sendTo(socket, MsgError::build("Not in a room"));
        return;
    }

    Room& room = rooms_[it->second];
    if (room.state != Room::State::FULL) {
        sendTo(socket, MsgError::build("Room is not full yet"));
        return;
    }

    room.state = Room::State::PLAYING;

    std::string gameType = j.value("game", "gyropi");

    std::mt19937 rng(std::random_device{}());
    bool hostIsPerformer = std::uniform_int_distribution<>(0, 1)(rng);

    QTcpSocket* performer   = hostIsPerformer ? room.host : room.guest;
    QTcpSocket* communicator = hostIsPerformer ? room.guest : room.host;

    int gamePort = spawnGameRoom(room.code, room.hostName, room.guestName, gameType);

    sendTo(performer,    MsgRoleAssigned::build(Role::PERFORMER,    gamePort, gameType));
    sendTo(communicator, MsgRoleAssigned::build(Role::COMMUNICATOR, gamePort, gameType));

    std::cout << "[Lobby] " << gameType << " started for room " << room.code
              << " on port " << gamePort << "\n";
}

// ── Helpers ──────────────────────────────────────────────────────────────────

void LobbyServer::sendTo(QTcpSocket* socket, const json& msg) {
    socket->write(frameMessage(msg.dump()));
}

std::string LobbyServer::generateRoomCode() {
    constexpr char chars[] = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<> dist(0, sizeof(chars) - 2);

    std::string code(4, ' ');
    do {
        for (char& c : code) c = chars[dist(rng)];
    } while (rooms_.count(code));

    return code;
}

int LobbyServer::spawnGameRoom(const std::string& roomCode,
                               const std::string& hostName,
                               const std::string& guestName,
                               const std::string& gameType) {
    int port = nextGamePort_++;

    pid_t pid = fork();
    if (pid == 0) {
        execl("./bin/game_room", "game_room",
              roomCode.c_str(),
              std::to_string(port).c_str(),
              hostName.c_str(),
              guestName.c_str(),
              gameType.c_str(),
              nullptr);
        std::cerr << "[Lobby] execl failed\n";
        _exit(1);
    } else if (pid < 0) {
        throw std::runtime_error("fork() failed");
    }

    return port;
}
