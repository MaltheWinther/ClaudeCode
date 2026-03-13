#pragma once

#include <string>
#include <functional>
#include <QObject>
#include <QTcpSocket>
#include "../common/Messages.hpp"
#include "../common/TcpFraming.hpp"

// Connects to LobbyServer via QTcpSocket. Non-blocking — runs in Qt event loop.
// Call connectToServer() after setting intent and callbacks.

class LobbyClient : public QObject {
    Q_OBJECT
public:
    LobbyClient(const std::string& host, int port, QObject* parent = nullptr);

    void createRoom(const std::string& username);
    void joinRoom(const std::string& code, const std::string& username);
    void connectToServer();   // non-blocking — starts async connect
    void stop();

    void requestStart();
    void requestStartWithGame(const std::string& gameType);

    std::string getRoomCode() const { return roomCode_; }
    std::string getRole()     const { return role_; }
    int         getGamePort() const { return gamePort_; }

    // Callbacks (set before calling connectToServer())
    void setOnRoomCreated (std::function<void(std::string code)>                        cb) { onRoomCreated_  = std::move(cb); }
    void setOnPlayerJoined(std::function<void(int, std::string, std::string)>           cb) { onPlayerJoined_ = std::move(cb); }
    void setOnRoleAssigned(std::function<void(std::string, int port, std::string game)> cb) { onRoleAssigned_ = std::move(cb); }
    void setOnError       (std::function<void(std::string msg)>                         cb) { onError_        = std::move(cb); }

private slots:
    void onConnected();
    void onReadyRead();
    void onDisconnected();
    void onSocketError(QAbstractSocket::SocketError err);

private:
    void onMessage(const std::string& raw);

    std::string  host_;
    int          port_;
    QTcpSocket*  socket_ = nullptr;
    TcpFrameReader reader_;

    enum class Intent { CREATE, JOIN } intent_ = Intent::CREATE;
    std::string joinCode_;
    std::string username_;

    // Results
    std::string roomCode_;
    std::string role_;
    int         gamePort_ = -1;

    // Callbacks
    std::function<void(std::string)>                     onRoomCreated_;
    std::function<void(int, std::string, std::string)>   onPlayerJoined_;
    std::function<void(std::string, int, std::string)>   onRoleAssigned_;
    std::function<void(std::string)>                     onError_;
};
