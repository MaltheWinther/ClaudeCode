#include "LobbyServer.hpp"

#include <iostream>
#include <csignal>
#include <sys/wait.h>  // waitpid — reap zombie game_room children

static LobbyServer* g_server = nullptr;

static void onSignal(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        std::cout << "\n[Server] Shutting down...\n";
        if (g_server) g_server->stop();
    }
    if (sig == SIGCHLD) {
        // Reap any finished game_room child processes to avoid zombies
        while (waitpid(-1, nullptr, WNOHANG) > 0) {}
    }
}

int main() {
    constexpr int LOBBY_PORT = 9000;

    std::signal(SIGINT,  onSignal);
    std::signal(SIGTERM, onSignal);
    std::signal(SIGCHLD, onSignal);

    try {
        LobbyServer server(LOBBY_PORT);
        g_server = &server;
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "[Server] Fatal: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
