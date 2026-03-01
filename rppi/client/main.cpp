#include <QApplication>
#include <QMetaObject>
#include <thread>
#include <chrono>
#include <string>

#include "ui/AppWindow.hpp"
#include "Credentials.hpp"
#include "LobbyClient.hpp"
#include "GameClient.hpp"
#include "GyroReader.hpp"
#include "../common/Messages.hpp"

// Usage: ./client <server_ip>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <server_ip>\n", argv[0]);
        return 1;
    }
    const std::string serverIp = argv[1];

    QApplication app(argc, argv);
    app.setStyle("Fusion");

    AppWindow window;
    window.setWindowTitle("Ready Player Pi");
    window.resize(800, 600);
    window.show();

    // ── Application state ────────────────────────────────────────────────────
    std::string  username;
    std::string  role;
    int          gamePort  = -1;
    LobbyClient* lobby     = nullptr;
    GameClient*  game      = nullptr;
    GyroReader   gyro;
    std::thread  netThread;

    // Join and discard the current network thread (safe after it has stopped)
    auto joinNetThread = [&] {
        if (netThread.joinable()) netThread.join();
    };

    // ── USERNAME PHASE ───────────────────────────────────────────────────────
    username = Credentials::load();
    if (username.empty())
        window.showUsername();
    else
        window.showMainMenu(QString::fromStdString(username));

    QObject::connect(&window, &AppWindow::usernameSubmitted,
                     [&](const QString& u) {
        std::string s = u.toStdString();
        if (!Credentials::isValid(s)) {
            window.setUsernameError(
                QString::fromStdString(Credentials::invalidReason(s)));
            return;
        }
        Credentials::save(s);
        username = s;
        window.showMainMenu(u);
    });

    // ── Lobby callback setup (shared by host and guest) ───────────────────────
    auto setupLobbyCallbacks = [&]() {
        lobby->setOnRoomCreated([&window](std::string code) {
            QMetaObject::invokeMethod(&window,
                [&window, c = std::move(code)]() mutable {
                    window.showLobbyHost(QString::fromStdString(c), 1);
                }, Qt::QueuedConnection);
        });

        lobby->setOnPlayerJoined([&window](int count) {
            QMetaObject::invokeMethod(&window,
                [&window, count]() {
                    window.updateLobbyPlayerCount(count);
                }, Qt::QueuedConnection);
        });

        // Write role/gamePort in the Qt main thread to avoid data races
        lobby->setOnRoleAssigned([&window, &role, &gamePort]
                                 (std::string r, int port) {
            QMetaObject::invokeMethod(&window,
                [&window, &role, &gamePort, r = std::move(r), port]() mutable {
                    role     = r;
                    gamePort = port;
                    window.showRole(QString::fromStdString(r),
                                    QStringLiteral("GyroPi"));
                }, Qt::QueuedConnection);
        });

        lobby->setOnError([&window, &username](std::string msg) {
            QMetaObject::invokeMethod(&window,
                [&window, &username, m = std::move(msg)]() mutable {
                    window.showMainMenu(QString::fromStdString(username));
                    window.setMainMenuError(QString::fromStdString(m));
                }, Qt::QueuedConnection);
        });
    };

    // ── HOST ROOM ────────────────────────────────────────────────────────────
    QObject::connect(&window, &AppWindow::hostRoomClicked, [&]() {
        joinNetThread();
        delete lobby;
        lobby = new LobbyClient(serverIp, 9000);
        setupLobbyCallbacks();
        lobby->createRoom(username);
        netThread = std::thread([lp = lobby] { lp->run(); });
    });

    // ── JOIN ROOM ────────────────────────────────────────────────────────────
    QObject::connect(&window, &AppWindow::joinRoomClicked,
                     [&](const QString& code) {
        joinNetThread();
        delete lobby;
        lobby = new LobbyClient(serverIp, 9000);
        setupLobbyCallbacks();
        lobby->joinRoom(code.toStdString(), username);
        window.showLobbyGuest(code);
        netThread = std::thread([lp = lobby] { lp->run(); });
    });

    // ── HOST PRESSES PLAY ────────────────────────────────────────────────────
    QObject::connect(&window, &AppWindow::startGameClicked, [&]() {
        if (lobby) lobby->requestStart();
    });

    // ── ROLE SCREEN AUTO-ADVANCES → START GAME ───────────────────────────────
    QObject::connect(&window, &AppWindow::roleDisplayDone, [&]() {
        window.showGame(QString::fromStdString(role));

        if (role == Role::PERFORMER) gyro.start();

        joinNetThread();  // join lobby thread (exits quickly after role assigned)
        delete game;
        game = new GameClient(serverIp, gamePort, role,
                              (role == Role::PERFORMER) ? &gyro : nullptr);

        game->setOnGameState([&window](MsgGameState gs) {
            QMetaObject::invokeMethod(&window,
                [&window, gs]() {
                    window.updateGameState(gs.ballX, gs.ballY,
                                           gs.pitch, gs.roll, gs.level);
                }, Qt::QueuedConnection);
        });

        game->setOnGameOver([&window, &gyro](bool win) {
            gyro.stop();
            QMetaObject::invokeMethod(&window,
                [&window, win]() {
                    window.showGameOver(win);
                }, Qt::QueuedConnection);
        });

        // Sleep 500 ms in the network thread so game_room has time to bind
        netThread = std::thread([gp = game]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            gp->run();
        });
    });

    // ── PLAY AGAIN → BACK TO MAIN MENU ───────────────────────────────────────
    QObject::connect(&window, &AppWindow::playAgainClicked, [&]() {
        window.showMainMenu(QString::fromStdString(username));
    });

    // ── Run Qt event loop ─────────────────────────────────────────────────────
    int ret = app.exec();

    joinNetThread();
    delete game;
    delete lobby;
    return ret;
}
