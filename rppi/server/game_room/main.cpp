#include "../GameRoom.hpp"

#include <iostream>
#include <csignal>
#include <cstdlib>  // atoi

static GameRoom* g_room = nullptr;

static void onSignal(int /*sig*/) {
    if (g_room) g_room->stop();
}

// Called by LobbyServer via:
//   execl("./game_room", "game_room", roomCode, port, hostName, guestName, nullptr)
int main(int argc, char* argv[]) {
    if (argc != 5) {
        std::cerr << "Usage: game_room <room_code> <port> <host_name> <guest_name>\n";
        return 1;
    }

    const std::string roomCode  = argv[1];
    const int         port      = std::atoi(argv[2]);
    const std::string hostName  = argv[3];
    const std::string guestName = argv[4];

    std::signal(SIGINT,  onSignal);
    std::signal(SIGTERM, onSignal);

    try {
        GameRoom room(roomCode, port, hostName, guestName);
        g_room = &room;
        room.run();
    } catch (const std::exception& e) {
        std::cerr << "[Room " << roomCode << "] Fatal: " << e.what() << "\n";
        return 1;
    }

    std::cout << "[Room " << roomCode << "] Exiting cleanly\n";
    return 0;
}
