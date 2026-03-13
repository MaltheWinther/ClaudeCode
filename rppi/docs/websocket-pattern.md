# TCP-netvaerksmoensteret med Qt Network

## Hvorfor dette er vigtigt

ALLE 4 netvaerksklasser (LobbyServer, LobbyClient, GameRoom, GameClient)
bruger det SAMME moenser. Forstaar du et, forstaar du alle fire.

## Det grundlaeggende moenser

Alle netvaerksklasser arver `QObject` og bruger Qt's signal/slot system
til non-blocking I/O. Servere bruger `QTcpServer`, clients bruger `QTcpSocket`.

### 1. Server-side: Opret QTcpServer

```cpp
class LobbyServer : public QObject {
    Q_OBJECT
public:
    LobbyServer(int port) {
        tcpServer_ = new QTcpServer(this);
        connect(tcpServer_, &QTcpServer::newConnection,
                this, &LobbyServer::onNewConnection);
        tcpServer_->listen(QHostAddress::Any, port);
    }

private slots:
    void onNewConnection() {
        while (QTcpSocket* socket = tcpServer_->nextPendingConnection()) {
            readers_[socket];  // opret TcpFrameReader for denne client
            connect(socket, &QTcpSocket::readyRead,
                    this, &LobbyServer::onClientReadyRead);
            connect(socket, &QTcpSocket::disconnected,
                    this, &LobbyServer::onClientDisconnected);
        }
    }
};
```

### 2. Client-side: Opret QTcpSocket

```cpp
class LobbyClient : public QObject {
    Q_OBJECT
public:
    void connectToServer() {
        socket_ = new QTcpSocket(this);
        connect(socket_, &QTcpSocket::connected,
                this, &LobbyClient::onConnected);
        connect(socket_, &QTcpSocket::readyRead,
                this, &LobbyClient::onReadyRead);
        connect(socket_, &QTcpSocket::disconnected,
                this, &LobbyClient::onDisconnected);
        socket_->connectToHost(host, port);  // non-blocking!
    }
};
```

**connectToHost()** returnerer med det samme. Naar forbindelsen er klar,
fires `connected` signalet og `onConnected()` slot korer.

### 3. Length-prefix framing (TcpFraming.hpp)

TCP er en byte-stream -- den har ikke besked-graenser som WebSocket.
Vi bruger en simpel framing-protokol:

```
[4 bytes: besked-laengde (big-endian)][JSON payload]
```

**Sende:**
```cpp
void sendTo(QTcpSocket* socket, const json& msg) {
    socket->write(frameMessage(msg.dump()));
}
```

`frameMessage()` prepender 4 bytes med laengden, sa modtageren ved
praecis hvor mange bytes der hoerer til denne besked.

**Modtage:**
```cpp
void onClientReadyRead() {
    auto* socket = qobject_cast<QTcpSocket*>(sender());
    auto messages = readers_[socket].feed(socket->readAll());
    for (const auto& raw : messages) {
        onMessage(socket, raw);
    }
}
```

`TcpFrameReader` akkumulerer bytes og returnerer faerdige beskeder.
Den haandterer automatisk partial reads (hvis TCP splitter en besked
over flere readyRead-events).

### 4. Sende beskeder

Med Qt Network er det meget simpelt — bare skriv direkte:

```cpp
void sendTo(QTcpSocket* socket, const json& msg) {
    socket->write(frameMessage(msg.dump()));
}
```

Ingen write-queue nødvendig (som med lws). Qt haandterer buffering internt.
Man kan skrive til en QTcpSocket naar som helst fra Qt main thread.

### 5. Qt event loop i stedet for blocking run()

I stedet for en blocking `while(running) { poll(); }` loop bruger vi
Qt's event loop:

```cpp
// Server:
int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    LobbyServer server(9000);
    return app.exec();  // korer event loop
}

// Client: connectToServer() er non-blocking
lobby->connectToServer();
// ... Qt event loop (app.exec()) haandterer resten
```

## Fordele ved Qt Network vs libwebsockets

| | libwebsockets (foer) | Qt Network (nu) |
|---|---|---|
| Event loop | Manuel `while(running_) lws_service(ctx, 50ms)` | Qt's built-in `app.exec()` |
| Sende | Queue + `lws_callback_on_writable()` + `onWritable()` | `socket->write(...)` direkte |
| Traade (client) | Separat `std::thread` for netvaerk | Korer i Qt main thread |
| Traad-sikkerhed | `lws_cancel_service()` + mutex | Ikke nødvendig (samme traad) |
| Callback-stil | Static function + global pointer trick | Qt signals/slots |
| Framing | WebSocket framing (built-in) | Manual length-prefix (TcpFraming.hpp) |

## De 4 netvaerksklasser

| Klasse | Type | Port | Arver |
|--------|------|------|-------|
| LobbyServer | QTcpServer (server) | 9000 | QObject |
| LobbyClient | QTcpSocket (client) | 9000 | QObject |
| GameRoom | QTcpServer (server) | 900x | QObject |
| GameClient | QTcpSocket (client) | 900x | QObject |

Alle fire folger praecis det samme moenser ovenfor.

## GameRoom: QTimer til polling

GameRoom bruger en QTimer (5ms interval) til at polle game state
fra fysik-traaden:

```cpp
pollTimer_ = new QTimer(this);
pollTimer_->setInterval(5);
connect(pollTimer_, &QTimer::timeout, this, &GameRoom::onPollTick);
pollTimer_->start();

void GameRoom::onPollTick() {
    if (stateDirty_.exchange(false))
        broadcastGameState();
    if (pendingStopCountdown_ > 0 && --pendingStopCountdown_ == 0)
        stop();
}
```

## GameClient: QTimer til gyro

Performer's GameClient bruger en QTimer (25ms, ~40Hz) til at
streame gyroscope data:

```cpp
void GameClient::onConnected() {
    // ...
    if (role_ == Role::PERFORMER && gyro_) {
        gyroTimer_ = new QTimer(this);
        gyroTimer_->setInterval(25);
        connect(gyroTimer_, &QTimer::timeout, this, &GameClient::onGyroTick);
        gyroTimer_->start();
    }
}

void GameClient::onGyroTick() {
    auto angles = gyro_->read();
    socket_->write(frameMessage(MsgGyroData::build(angles.pitch, angles.roll).dump()));
}
```
