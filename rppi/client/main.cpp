#include <QApplication>
#include <QMetaObject>
#include <thread>
#include <chrono>
#include <string>
#include <vector>

#include "ui/AppWindow.hpp"
#include "Credentials.hpp"
#include "LobbyClient.hpp"
#include "GameClient.hpp"
#include "GyroReader.hpp"
#include "NoteReader.hpp"
#include "../common/Messages.hpp"

// Usage: ./client <server_ip> [username]
//   Optional username bypasses saved credentials (useful for testing 2 clients on same machine)

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <server_ip> [username]\n", argv[0]);
        return 1;
    }
    const std::string serverIp      = argv[1];
    const std::string cliUsername   = (argc >= 3) ? argv[2] : "";

    QApplication app(argc, argv);
    app.setStyle("Fusion");

    AppWindow window;
    window.setWindowTitle("Ready Player Pi");
    window.resize(800, 600);
    window.show();

    // ── Application state ────────────────────────────────────────────────────
    std::string  username;
    std::string  role;
    std::string  gameType;
    int          gamePort  = -1;
    bool         isHost    = false;
    LobbyClient* lobby     = nullptr;
    GameClient*  game      = nullptr;
    GyroReader   gyro;
    NoteReader   noteReader;
    std::thread  netThread;

    // Join and discard the current network thread (safe after it has stopped)
    auto joinNetThread = [&] {
        if (netThread.joinable()) netThread.join();
    };

    // ── USERNAME PHASE ───────────────────────────────────────────────────────
    if (!cliUsername.empty()) {
        // CLI override: skip credential file, go straight to main menu
        username = cliUsername;
        window.showMainMenu(QString::fromStdString(username));
    } else {
        username = Credentials::load();
        if (username.empty())
            window.showUsername();
        else
            window.showMainMenu(QString::fromStdString(username));
    }

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

        lobby->setOnPlayerJoined([&window](int count, std::string h, std::string g) {
            QMetaObject::invokeMethod(&window,
                [&window, count, h = std::move(h), g = std::move(g)]() mutable {
                    window.updateLobbyPlayers(count,
                        QString::fromStdString(h), QString::fromStdString(g));
                }, Qt::QueuedConnection);
        });

        // Write role/gamePort/gameType in the Qt main thread to avoid data races
        lobby->setOnRoleAssigned([&window, &role, &gamePort, &gameType]
                                 (std::string r, int port, std::string gt) {
            QMetaObject::invokeMethod(&window,
                [&window, &role, &gamePort, &gameType,
                 r = std::move(r), port, gt = std::move(gt)]() mutable {
                    role     = r;
                    gamePort = port;
                    gameType = gt;  // set from server (important for guest)
                    window.showRole(QString::fromStdString(r),
                                    QString::fromStdString(gameType));
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
        isHost = true;
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
        isHost = false;
        joinNetThread();
        delete lobby;
        lobby = new LobbyClient(serverIp, 9000);
        setupLobbyCallbacks();
        lobby->joinRoom(code.toStdString(), username);
        window.showLobbyGuest(code);
        netThread = std::thread([lp = lobby] { lp->run(); });
    });

    // ── HOST PRESSES PLAY → GAME SELECT SCREEN ──────────────────────────────
    QObject::connect(&window, &AppWindow::startGameClicked, [&]() {
        window.showGameSelect(isHost);
    });

    // ── GAME SELECTED (host picks) → SEND START_GAME WITH GAME TYPE ─────────
    QObject::connect(&window, &AppWindow::gameSelected,
                     [&](const QString& gt) {
        gameType = gt.toStdString();
        if (lobby) lobby->requestStartWithGame(gameType);
    });

    // ── ROLE SCREEN AUTO-ADVANCES → START GAME ───────────────────────────────
    QObject::connect(&window, &AppWindow::roleDisplayDone, [&]() {
        if (gameType == "notepi") {
            window.showNotePiGame(QString::fromStdString(role));

            if (role == Role::PERFORMER) {
                noteReader.setOnNoteDetected([&window](NoteReader::NoteEvent ev) {
                    QMetaObject::invokeMethod(&window,
                        [&window, n = std::move(ev.note)]() mutable {
                            window.addDetectedNote(QString::fromStdString(n));
                        }, Qt::QueuedConnection);
                });
                noteReader.start();
            }
        } else {
            window.showGame(QString::fromStdString(role));
            if (role == Role::PERFORMER) gyro.start();
        }

        joinNetThread();  // join lobby thread (exits quickly after role assigned)
        delete game;
        game = new GameClient(serverIp, gamePort, role,
                              (gameType != "notepi" && role == Role::PERFORMER) ? &gyro : nullptr);

        game->setOnGameState([&window](MsgGameState gs) {
            QMetaObject::invokeMethod(&window,
                [&window, gs]() {
                    window.updateGameState(gs.ballX, gs.ballY,
                                           gs.pitch, gs.roll, gs.level);
                }, Qt::QueuedConnection);
        });

        game->setOnNoteState([&window](MsgNoteState ns) {
            QMetaObject::invokeMethod(&window,
                [&window, ns = std::move(ns)]() mutable {
                    window.updateNoteState(ns);
                }, Qt::QueuedConnection);
        });

        game->setOnGameOver([&window, &gyro, &noteReader, &gameType]
                            (bool win, int elapsed, int levels,
                             std::vector<LeaderboardEntry> lb) {
            // Don't block the network thread with stop() calls —
            // do all cleanup in the Qt main thread instead
            QMetaObject::invokeMethod(&window,
                [&window, &gyro, &noteReader, win, elapsed, levels,
                 lb = std::move(lb), gt = std::string(gameType)]() mutable {
                    gyro.stop();
                    noteReader.stop();
                    window.showGameOver(win, elapsed, levels, lb,
                                        QString::fromStdString(gt));
                }, Qt::QueuedConnection);
        });

        game->setOnError([&window, &gyro, &noteReader](std::string msg) {
            QMetaObject::invokeMethod(&window,
                [&window, &gyro, &noteReader, m = std::move(msg)]() mutable {
                    gyro.stop();
                    noteReader.stop();
                    window.showDisconnect(QString::fromStdString(m));
                }, Qt::QueuedConnection);
        });

        // Sleep 500 ms in the network thread so game_room has time to bind
        netThread = std::thread([gp = game]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            gp->run();
        });
    });

    // ── NOTEPI: PERFORMER SUBMITS NOTES ──────────────────────────────────────
    QObject::connect(&window, &AppWindow::notePiNotesSubmitted,
                     [&](const std::vector<std::string>& notes) {
        if (game) game->submitNotes(notes);
    });

    // ── EXIT LOBBY → STOP LOBBY, BACK TO MAIN MENU ─────────────────────────
    QObject::connect(&window, &AppWindow::exitLobbyClicked, [&]() {
        if (lobby) { lobby->stop(); joinNetThread(); delete lobby; lobby = nullptr; }
        window.showMainMenu(QString::fromStdString(username));
    });

    // ── EXIT GAME → STOP EVERYTHING, BACK TO MAIN MENU ─────────────────────
    QObject::connect(&window, &AppWindow::exitGameClicked, [&]() {
        gyro.stop();
        noteReader.stop();
        if (game) { game->stop(); joinNetThread(); delete game; game = nullptr; }
        window.showMainMenu(QString::fromStdString(username));
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
