#include "LobbyServer.hpp"

#include <iostream>
#include <csignal>
#include <sys/wait.h>
#include <QCoreApplication>

static LobbyServer* g_server = nullptr;

static void onSignal(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        std::cout << "\n[Server] Shutting down...\n";
        if (g_server) g_server->stop();
    }
    if (sig == SIGCHLD) {
        while (waitpid(-1, nullptr, WNOHANG) > 0) {}
    }
}

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);

    constexpr int LOBBY_PORT = 9000;

    std::signal(SIGINT,  onSignal);
    std::signal(SIGTERM, onSignal);
    std::signal(SIGCHLD, onSignal);

    try {
        LobbyServer server(LOBBY_PORT);
        g_server = &server;
        return app.exec();
    } catch (const std::exception& e) {
        std::cerr << "[Server] Fatal: " << e.what() << "\n";
        return 1;
    }
}
