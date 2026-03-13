#pragma once

#include <string>
#include <vector>
#include <functional>
#include <QObject>
#include <QTcpSocket>
#include <QTimer>
#include "../common/Messages.hpp"
#include "../common/TcpFraming.hpp"
#include "GyroReader.hpp"

// Connects to a GameRoom process via QTcpSocket. Non-blocking — runs in Qt event loop.
// Call connectToServer() after setting callbacks.

class GameClient : public QObject {
    Q_OBJECT
public:
    GameClient(const std::string& host, int port,
               const std::string& role,
               GyroReader* gyro,
               QObject* parent = nullptr);

    void connectToServer();   // non-blocking
    void stop();

    void setOnGameState(std::function<void(MsgGameState)>                                  cb) { onGameState_ = std::move(cb); }
    void setOnGameOver (std::function<void(bool, int, int, std::vector<LeaderboardEntry>)> cb) { onGameOver_  = std::move(cb); }
    void setOnNoteState(std::function<void(MsgNoteState)>                                  cb) { onNoteState_ = std::move(cb); }
    void setOnError    (std::function<void(std::string)>                                   cb) { onError_cb_  = std::move(cb); }

    void submitNotes(const std::vector<std::string>& notes);

private slots:
    void onConnected();
    void onReadyRead();
    void onDisconnected();
    void onSocketError(QAbstractSocket::SocketError err);
    void onGyroTick();

private:
    void onMessage(const std::string& raw);

    std::string  host_;
    int          port_;
    std::string  role_;
    GyroReader*  gyro_;

    QTcpSocket*    socket_    = nullptr;
    QTimer*        gyroTimer_ = nullptr;
    TcpFrameReader reader_;
    bool           gameOver_  = false;   // suppress disconnect error after game over

    std::function<void(MsgGameState)>                                  onGameState_;
    std::function<void(bool, int, int, std::vector<LeaderboardEntry>)> onGameOver_;
    std::function<void(MsgNoteState)>                                  onNoteState_;
    std::function<void(std::string)>                                   onError_cb_;
};
