#include <QApplication>
#include <QTimer>
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

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <server_ip> [username]\n", argv[0]);
        return 1;
    }
    const std::string serverIp    = argv[1];
    const std::string cliUsername  = (argc >= 3) ? argv[2] : "";

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

    // ── USERNAME PHASE ───────────────────────────────────────────────────────
    if (!cliUsername.empty()) {
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

    // ── Lobby callback setup (shared by host and guest) ──────────────────────
    auto setupLobbyCallbacks = [&]() {
        lobby->setOnRoomCreated([&window](std::string code) {
            window.showLobbyHost(QString::fromStdString(code), 1);
        });

        lobby->setOnPlayerJoined([&window](int count, std::string h, std::string g) {
            window.updateLobbyPlayers(count,
                QString::fromStdString(h), QString::fromStdString(g));
        });

        lobby->setOnRoleAssigned([&window, &role, &gamePort, &gameType]
                                 (std::string r, int port, std::string gt) {
            role     = r;
            gamePort = port;
            gameType = gt;
            window.showRole(QString::fromStdString(r),
                            QString::fromStdString(gameType));
        });

        lobby->setOnError([&window, &username](std::string msg) {
            window.showMainMenu(QString::fromStdString(username));
            window.setMainMenuError(QString::fromStdString(msg));
        });
    };

    // ── HOST ROOM ────────────────────────────────────────────────────────────
    QObject::connect(&window, &AppWindow::hostRoomClicked, [&]() {
        isHost = true;
        delete lobby;
        lobby = new LobbyClient(serverIp, 9000);
        setupLobbyCallbacks();
        lobby->createRoom(username);
        lobby->connectToServer();
    });

    // ── JOIN ROOM ────────────────────────────────────────────────────────────
    QObject::connect(&window, &AppWindow::joinRoomClicked,
                     [&](const QString& code) {
        isHost = false;
        delete lobby;
        lobby = new LobbyClient(serverIp, 9000);
        setupLobbyCallbacks();
        lobby->joinRoom(code.toStdString(), username);
        window.showLobbyGuest(code);
        lobby->connectToServer();
    });

    // ── HOST PRESSES PLAY → GAME SELECT SCREEN ──────────────────────────────
    QObject::connect(&window, &AppWindow::startGameClicked, [&]() {
        window.showGameSelect(isHost);
    });

    // ── GAME SELECTED → SEND START_GAME WITH GAME TYPE ──────────────────────
    QObject::connect(&window, &AppWindow::gameSelected,
                     [&](const QString& gt) {
        gameType = gt.toStdString();
        if (lobby) lobby->requestStartWithGame(gameType);
    });

    // ── ROLE SCREEN AUTO-ADVANCES → START GAME ──────────────────────────────
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

        delete game;
        game = new GameClient(serverIp, gamePort, role,
                              (gameType != "notepi" && role == Role::PERFORMER) ? &gyro : nullptr);

        game->setOnGameState([&window](MsgGameState gs) {
            window.updateGameState(gs.ballX, gs.ballY,
                                   gs.pitch, gs.roll, gs.level);
        });

        game->setOnNoteState([&window](MsgNoteState ns) {
            window.updateNoteState(ns);
        });

        game->setOnGameOver([&window, &gyro, &noteReader, &gameType]
                            (bool win, int elapsed, int levels,
                             std::vector<LeaderboardEntry> lb) {
            gyro.stop();
            noteReader.stop();
            window.showGameOver(win, elapsed, levels, lb,
                                QString::fromStdString(gameType));
        });

        game->setOnError([&window, &gyro, &noteReader](std::string msg) {
            gyro.stop();
            noteReader.stop();
            window.showDisconnect(QString::fromStdString(msg));
        });

        // Delay connect by 500ms so game_room has time to bind its port
        QTimer::singleShot(500, game, [gp = game]() {
            gp->connectToServer();
        });
    });

    // ── NOTEPI: PERFORMER SUBMITS NOTES ──────────────────────────────────────
    QObject::connect(&window, &AppWindow::notePiNotesSubmitted,
                     [&](const std::vector<std::string>& notes) {
        if (game) game->submitNotes(notes);
    });

    // ── EXIT LOBBY → STOP LOBBY, BACK TO MAIN MENU ─────────────────────────
    QObject::connect(&window, &AppWindow::exitLobbyClicked, [&]() {
        if (lobby) { lobby->stop(); delete lobby; lobby = nullptr; }
        window.showMainMenu(QString::fromStdString(username));
    });

    // ── EXIT GAME → STOP EVERYTHING, BACK TO MAIN MENU ─────────────────────
    QObject::connect(&window, &AppWindow::exitGameClicked, [&]() {
        gyro.stop();
        noteReader.stop();
        if (game) { game->stop(); delete game; game = nullptr; }
        window.showMainMenu(QString::fromStdString(username));
    });

    // ── PLAY AGAIN → BACK TO MAIN MENU ──────────────────────────────────────
    QObject::connect(&window, &AppWindow::playAgainClicked, [&]() {
        window.showMainMenu(QString::fromStdString(username));
    });

    // ── Run Qt event loop ────────────────────────────────────────────────────
    int ret = app.exec();

    delete game;
    delete lobby;
    return ret;
}
