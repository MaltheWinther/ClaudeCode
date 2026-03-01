#include "LobbyClient.hpp"
#include "GameClient.hpp"
#include "GyroReader.hpp"
#include "LocalBrowser.hpp"

#include <iostream>
#include <string>
#include <thread>
#include <chrono>

// Usage:
//   ./client <server_ip> create
//   ./client <server_ip> join <room_code>

static void printUsage(const char* prog) {
    std::cerr << "Usage:\n"
              << "  " << prog << " <server_ip> create\n"
              << "  " << prog << " <server_ip> join <room_code>\n";
}

int main(int argc, char* argv[]) {
    if (argc < 3) { printUsage(argv[0]); return 1; }

    const std::string serverIp = argv[1];
    const std::string action   = argv[2];

    if (action != "create" && action != "join") { printUsage(argv[0]); return 1; }
    if (action == "join" && argc < 4)           { printUsage(argv[0]); return 1; }

    // ── Step 1: Lobby ────────────────────────────────────────────────────
    LobbyClient lobby(serverIp, 9000);

    if (action == "create")
        lobby.createRoom();
    else
        lobby.joinRoom(argv[3]);

    lobby.run();  // blocks until role is assigned

    const std::string role     = lobby.getRole();
    const int         gamePort = lobby.getGamePort();

    if (role.empty() || gamePort < 0) {
        std::cerr << "[Client] Failed to get role assignment\n";
        return 1;
    }

    // ── Step 2: Start local browser display ─────────────────────────────
    const bool isCommunicator = (role == Role::COMMUNICATOR);
    const std::string htmlFile = isCommunicator ? "web/game.html" : "web/tilt.html";
    const int browserPort      = isCommunicator ? 8080 : 8081;

    LocalBrowser browser(browserPort, htmlFile);
    browser.start();

    std::cout << "[Client] Open browser at http://localhost:" << browserPort << "\n";

    // ── Step 3: Start gyro only if performer ─────────────────────────────
    GyroReader gyro;
    if (role == Role::PERFORMER)
        gyro.start();

    // ── Step 4: Connect to game room and play ────────────────────────────
    // Wait for game_room process to finish starting up before connecting
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    GameClient game(serverIp, gamePort, role,
                    (role == Role::PERFORMER) ? &gyro : nullptr,
                    &browser);

    game.run();  // blocks until game is over

    // ── Cleanup ──────────────────────────────────────────────────────────
    gyro.stop();
    browser.stop();

    std::cout << "[Client] Done\n";
    return 0;
}
