#include "LobbyServer.hpp"

#include <iostream>
#include <stdexcept>
#include <random>
#include <unistd.h>   // fork, execl
#include <cstring>

// ── Static instance pointer so the lws callback can reach us ────────────────
static LobbyServer* s_instance = nullptr;

// ── Constructor / Destructor ─────────────────────────────────────────────────

LobbyServer::LobbyServer(int port) : port_(port) {
    s_instance = this;

    lws_protocols protocols[] = {
        { "rppi-lobby", LobbyServer::wsCallback, 0, 4096, 0, nullptr, 0 },
        { nullptr, nullptr, 0, 0, 0, nullptr, 0 }  // terminator
    };

    lws_context_creation_info info{};
    info.port      = port_;
    info.protocols = protocols;
    info.options   = LWS_SERVER_OPTION_VALIDATE_UTF8; //Tjekker at beskeden fra klienten sender gyldig tekst. Hvis ikke, så dropper vi forbindelsen.

    context_ = lws_create_context(&info); //Starter serveren og giver den information fra vores info-struct.
    if (!context_)
        throw std::runtime_error("Failed to create libwebsockets context");

    std::cout << "[Lobby] Listening on port " << port_ << "\n";
}

LobbyServer::~LobbyServer() {
    if (context_) lws_context_destroy(context_);
}

// ── Public interface ─────────────────────────────────────────────────────────

void LobbyServer::run() {
    running_ = true;
    while (running_)
        lws_service(context_, 50);  // 50ms timeout per poll
}

void LobbyServer::stop() {
    running_ = false;
}

// ── Static lws callback — forwards to instance ──────────────────────────────

// lws* wsi = en pointer til den klient der 

int LobbyServer::wsCallback(lws* wsi, lws_callback_reasons reason,
                             void* /*user*/, void* in, size_t len) {
    if (!s_instance) return 0;

    switch (reason) { // fortsæt her
        case LWS_CALLBACK_ESTABLISHED:
            s_instance->onConnect(wsi);
            break;

        case LWS_CALLBACK_RECEIVE:
            s_instance->onMessage(wsi, std::string(static_cast<char*>(in), len));
            break;

        case LWS_CALLBACK_SERVER_WRITEABLE:
            s_instance->onWritable(wsi);
            break;

        case LWS_CALLBACK_CLOSED:
            s_instance->onDisconnect(wsi);
            break;

        default:
            break;
    }
    return 0;
}

// ── Event handlers ───────────────────────────────────────────────────────────

void LobbyServer::onConnect(lws* wsi) {
    writeQueues_[wsi];  // initialise empty send queue
    std::cout << "[Lobby] Client connected\n";
}

void LobbyServer::onMessage(lws* wsi, const std::string& raw) {
    std::string type;
    try {
        type = getMsgType(raw);
    } catch (...) {
        sendTo(wsi, MsgError::build("Invalid JSON"));
        return;
    }

    const json j = json::parse(raw);
    if      (type == MsgType::CREATE_ROOM) handleCreateRoom(wsi, j);
    else if (type == MsgType::JOIN_ROOM)   handleJoinRoom(wsi, j);
    else if (type == MsgType::START_GAME)  handleStartGame(wsi, j);
    else    sendTo(wsi, MsgError::build("Unknown message type: " + type));
}

void LobbyServer::onWritable(lws* wsi) {
    auto& queue = writeQueues_[wsi];
    if (queue.empty()) return;

    const std::string& msg = queue.front();
    // lws requires LWS_PRE bytes of padding before the payload
    std::vector<unsigned char> buf(LWS_PRE + msg.size());
    std::memcpy(buf.data() + LWS_PRE, msg.data(), msg.size());
    lws_write(wsi, buf.data() + LWS_PRE, msg.size(), LWS_WRITE_TEXT);

    queue.pop();
    if (!queue.empty())
        lws_callback_on_writable(wsi);  // more to send
}

void LobbyServer::onDisconnect(lws* wsi) {
    auto it = clientRooms_.find(wsi);
    if (it != clientRooms_.end()) {
        const std::string& code = it->second;
        rooms_.erase(code);
        clientRooms_.erase(it);
        std::cout << "[Lobby] Client disconnected, room " << code << " removed\n";
    }
    writeQueues_.erase(wsi);
}

// ── Lobby message handlers ───────────────────────────────────────────────────

void LobbyServer::handleCreateRoom(lws* wsi, const json& j) {
    // Reject if client is already in a room
    if (clientRooms_.count(wsi)) {
        sendTo(wsi, MsgError::build("Already in a room"));
        return;
    }

    std::string hostName = j.value("username", "Host");

    std::string code = generateRoomCode();
    rooms_[code]     = { code, hostName, "", wsi, nullptr, Room::State::WAITING };
    clientRooms_[wsi] = code;

    sendTo(wsi, MsgRoomCreated::build(code));
    std::cout << "[Lobby] Room created: " << code << " by " << hostName << "\n";
}

void LobbyServer::handleJoinRoom(lws* wsi, const json& j) {
    // Reject if client is already in a room
    if (clientRooms_.count(wsi)) {
        sendTo(wsi, MsgError::build("Already in a room"));
        return;
    }

    std::string code;
    try { code = MsgJoinRoom::parse(j).code; }
    catch (...) { sendTo(wsi, MsgError::build("Missing room code")); return; }

    auto it = rooms_.find(code);
    if (it == rooms_.end()) {
        sendTo(wsi, MsgError::build("Room not found: " + code));
        return;
    }
    Room& room = it->second;
    if (room.state != Room::State::WAITING) {
        sendTo(wsi, MsgError::build("Room is full or already started"));
        return;
    }

    std::string guestName = j.value("username", "Guest");

    room.guest     = wsi;
    room.guestName = guestName;
    room.state     = Room::State::FULL;
    clientRooms_[wsi] = code;

    // Send PLAYER_JOINED to both players (replaces separate ROOM_JOINED for host)
    auto pjMsg = MsgPlayerJoined::build(room.hostName, guestName, 2);
    sendTo(wsi,       pjMsg);
    sendTo(room.host, pjMsg);

    // Guest still needs ROOM_JOINED so it knows the room code
    sendTo(wsi, MsgRoomJoined::build(code));

    std::cout << "[Lobby] Room " << code << " is now full ("
              << room.hostName << " & " << guestName << ")\n";
}

void LobbyServer::handleStartGame(lws* wsi, const json& j) {
    auto it = clientRooms_.find(wsi);
    if (it == clientRooms_.end()) {
        sendTo(wsi, MsgError::build("Not in a room"));
        return;
    }

    Room& room = rooms_[it->second];
    if (room.state != Room::State::FULL) {
        sendTo(wsi, MsgError::build("Room is not full yet"));
        return;
    }

    room.state = Room::State::PLAYING;

    std::string gameType = j.value("game", "gyropi");

    // Randomly assign roles
    std::mt19937 rng(std::random_device{}());
    bool hostIsPerformer = std::uniform_int_distribution<>(0, 1)(rng);

    lws* performer   = hostIsPerformer ? room.host : room.guest;
    lws* communicator = hostIsPerformer ? room.guest : room.host;

    int gamePort = spawnGameRoom(room.code, room.hostName, room.guestName, gameType);

    sendTo(performer,    MsgRoleAssigned::build(Role::PERFORMER,    gamePort, gameType));
    sendTo(communicator, MsgRoleAssigned::build(Role::COMMUNICATOR, gamePort, gameType));

    std::cout << "[Lobby] " << gameType << " started for room " << room.code
              << " on port " << gamePort << "\n";
}

// ── Helpers ──────────────────────────────────────────────────────────────────

void LobbyServer::sendTo(lws* wsi, const json& msg) {
    writeQueues_[wsi].push(msg.dump());
    lws_callback_on_writable(wsi);
}

std::string LobbyServer::generateRoomCode() {
    constexpr char chars[] = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";  // no 0/O/1/I
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<> dist(0, sizeof(chars) - 2);

    std::string code(4, ' ');
    do {
        for (char& c : code) c = chars[dist(rng)];
    } while (rooms_.count(code));  // ensure uniqueness

    return code;
}

int LobbyServer::spawnGameRoom(const std::string& roomCode,
                               const std::string& hostName,
                               const std::string& guestName,
                               const std::string& gameType) {
    int port = nextGamePort_++;

    pid_t pid = fork();
    if (pid == 0) {
        // Child process — exec the game_room binary
        execl("./bin/game_room", "game_room",
              roomCode.c_str(),
              std::to_string(port).c_str(),
              hostName.c_str(),
              guestName.c_str(),
              gameType.c_str(),
              nullptr);
        std::cerr << "[Lobby] execl failed\n";
        _exit(1);
    } else if (pid < 0) {
        throw std::runtime_error("fork() failed");
    }

    // Parent continues — child runs independently
    return port;
}
