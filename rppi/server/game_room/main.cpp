#include "../GameRoom.hpp"

#include <iostream>
#include <csignal>
#include <cstdlib>  // atoi

static GameRoom* g_room = nullptr;

static void onSignal(int /*sig*/) {
    if (g_room) g_room->stop();
}

// Called by LobbyServer via:
//   execl("./game_room", "game_room", roomCode, port, nullptr)
int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: game_room <room_code> <port>\n";
        return 1;
    }

    const std::string roomCode = argv[1];
    const int         port     = std::atoi(argv[2]);

    std::signal(SIGINT,  onSignal);
    std::signal(SIGTERM, onSignal);

    try {
        GameRoom room(roomCode, port);
        g_room = &room;
        room.run();
    } catch (const std::exception& e) {
        std::cerr << "[Room " << roomCode << "] Fatal: " << e.what() << "\n";
        return 1;
    }

    std::cout << "[Room " << roomCode << "] Exiting cleanly\n";
    return 0;
}
