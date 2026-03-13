#pragma once

#include <string>
#include <map>
#include <thread>
#include <mutex>
#include <atomic>
#include <memory>
#include <chrono>
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include "../common/Messages.hpp"
#include "../common/TcpFraming.hpp"
#include "IGame.hpp"
#include "NotePiGame.hpp"

class GameRoom : public QObject {
    Q_OBJECT
public:
    GameRoom(const std::string& roomCode, int port,
             const std::string& hostName = "",
             const std::string& guestName = "",
             const std::string& gameType = "gyropi",
             QObject* parent = nullptr);
    ~GameRoom();

    void stop();

private slots:
    void onNewConnection();
    void onClientReadyRead();
    void onClientDisconnected();
    void onPollTick();

private:
    // ── Message dispatch ──────────────────────────────────────────────────
    void onMessage(QTcpSocket* socket, const std::string& raw);

    // ── Game message handlers ─────────────────────────────────────────────
    void handleIdentify(QTcpSocket* socket, const json& j);
    void handleGyroData(QTcpSocket* socket, const json& j);
    void handleNoteSubmit(QTcpSocket* socket, const json& j);

    // ── Game loop (runs in separate thread) ───────────────────────────────
    void gameLoopThread();
    void broadcastGameState();
    bool bothPlayersConnected() const;

    // ── Helpers ───────────────────────────────────────────────────────────
    void sendTo(QTcpSocket* socket, const json& msg);
    void queueTo(QTcpSocket* socket, const json& msg);
    std::string currentDate() const;

    // ── Members ───────────────────────────────────────────────────────────
    std::string  roomCode_;
    int          port_;
    std::string  players_;
    std::string  gameType_;

    QTcpServer* tcpServer_ = nullptr;
    QTimer*     pollTimer_ = nullptr;

    QTcpSocket* performer_    = nullptr;
    QTcpSocket* communicator_ = nullptr;

    std::map<QTcpSocket*, TcpFrameReader> readers_;

    using clock = std::chrono::steady_clock;
    std::chrono::time_point<clock> gameStartTime_;

    struct GyroInput { float pitch = 0.f; float roll = 0.f; };
    GyroInput  gyro_;
    std::mutex gyroMutex_;

    MsgGameState pendingState_{};
    std::atomic<bool> stateDirty_{ false };
    std::mutex   stateMutex_;

    std::unique_ptr<IGame> game_;
    std::unique_ptr<NotePiGame> notePi_;
    std::thread gameThread_;

    int pendingStopCountdown_ = 0;
};
