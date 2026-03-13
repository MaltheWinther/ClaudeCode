#pragma once

#include <string>
#include <map>
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include "../common/Messages.hpp"
#include "../common/TcpFraming.hpp"

class LobbyServer : public QObject {
    Q_OBJECT
public:
    explicit LobbyServer(int port, QObject* parent = nullptr);

    void stop();

private slots:
    void onNewConnection();
    void onClientReadyRead();
    void onClientDisconnected();

private:
    // ── Message dispatch ──────────────────────────────────────────────────
    void onMessage(QTcpSocket* socket, const std::string& raw);

    // ── Lobby message handlers ────────────────────────────────────────────
    void handleCreateRoom(QTcpSocket* socket, const json& j);
    void handleJoinRoom(QTcpSocket* socket, const json& j);
    void handleStartGame(QTcpSocket* socket, const json& j);

    // ── Helpers ───────────────────────────────────────────────────────────
    void sendTo(QTcpSocket* socket, const json& msg);
    std::string generateRoomCode();
    int spawnGameRoom(const std::string& roomCode,
                      const std::string& hostName,
                      const std::string& guestName,
                      const std::string& gameType);

    // ── Room state ────────────────────────────────────────────────────────
    struct Room {
        std::string code;
        std::string hostName;
        std::string guestName;
        QTcpSocket* host  = nullptr;
        QTcpSocket* guest = nullptr;
        enum class State { WAITING, FULL, PLAYING } state = State::WAITING;
    };

    // ── Members ───────────────────────────────────────────────────────────
    QTcpServer* tcpServer_ = nullptr;

    std::map<std::string, Room>            rooms_;        // code   → Room
    std::map<QTcpSocket*, std::string>     clientRooms_;  // socket → room code
    std::map<QTcpSocket*, TcpFrameReader>  readers_;      // socket → frame reader

    int nextGamePort_ = 9001;
};
