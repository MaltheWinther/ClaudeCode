#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// ─────────────────────────────────────────────
//  Message type identifiers (used in "type" field)
// ─────────────────────────────────────────────
namespace MsgType {
    // Client → Server (Lobby)
    constexpr auto CREATE_ROOM  = "create_room";
    constexpr auto JOIN_ROOM    = "join_room";
    constexpr auto START_GAME   = "start_game";

    // Client → Server (Game)
    constexpr auto IDENTIFY     = "identify";   // first msg sent to GameRoom
    constexpr auto GYRO_DATA    = "gyro_data";
    constexpr auto NOTE_SUBMIT  = "note_submit";  // performer submits 5-note guess

    // Server → Client (Lobby)
    constexpr auto ROOM_CREATED  = "room_created";
    constexpr auto ROOM_JOINED   = "room_joined";
    constexpr auto PLAYER_JOINED = "player_joined";  // replaces ROOM_JOINED for host notify
    constexpr auto ROLE_ASSIGNED = "role_assigned";
    constexpr auto ERROR         = "error";

    // Server → Client (Game)
    constexpr auto GAME_STATE   = "game_state";
    constexpr auto GAME_OVER    = "game_over";
    constexpr auto NOTE_STATE   = "note_state";   // Wordle feedback for NotePi
}

// ─────────────────────────────────────────────
//  Role identifiers
// ─────────────────────────────────────────────
namespace Role {
    constexpr auto PERFORMER    = "performer";
    constexpr auto COMMUNICATOR = "communicator";
}

// ─────────────────────────────────────────────
//  Leaderboard entry
// ─────────────────────────────────────────────
struct LeaderboardEntry {
    std::string players;      // "Alice & Bob"
    int         elapsedSecs;
    int         levelsReached;
    bool        won;
    std::string date;         // "2026-03-01"
    std::string gameType;     // "gyropi" or "notepi"
};

// ─────────────────────────────────────────────
//  Client → Server messages
// ─────────────────────────────────────────────

struct MsgCreateRoom {
    static json build(const std::string& username = "") {
        json j = { {"type", MsgType::CREATE_ROOM} };
        if (!username.empty()) j["username"] = username;
        return j;
    }
};

struct MsgJoinRoom {
    std::string code;

    static json build(const std::string& code, const std::string& username = "") {
        json j = { {"type", MsgType::JOIN_ROOM}, {"code", code} };
        if (!username.empty()) j["username"] = username;
        return j;
    }
    static MsgJoinRoom parse(const json& j) {
        return { j.at("code").get<std::string>() };
    }
};

struct MsgStartGame {
    static json build(const std::string& game = "") {
        json j = { {"type", MsgType::START_GAME} };
        if (!game.empty()) j["game"] = game;
        return j;
    }
};

struct MsgIdentify {
    std::string role;  // Role::PERFORMER or Role::COMMUNICATOR

    static json build(const std::string& role) {
        return { {"type", MsgType::IDENTIFY}, {"role", role} };
    }
    static MsgIdentify parse(const json& j) {
        return { j.at("role").get<std::string>() };
    }
};

struct MsgGyroData {
    float pitch;
    float roll;

    static json build(float pitch, float roll) {
        return { {"type", MsgType::GYRO_DATA}, {"pitch", pitch}, {"roll", roll} };
    }
    static MsgGyroData parse(const json& j) {
        return { j.at("pitch").get<float>(), j.at("roll").get<float>() };
    }
};

// ─────────────────────────────────────────────
//  Server → Client messages
// ─────────────────────────────────────────────

struct MsgRoomCreated {
    std::string code;

    static json build(const std::string& code) {
        return { {"type", MsgType::ROOM_CREATED}, {"code", code} };
    }
    static MsgRoomCreated parse(const json& j) {
        return { j.at("code").get<std::string>() };
    }
};

struct MsgRoomJoined {
    std::string code;

    static json build(const std::string& code) {
        return { {"type", MsgType::ROOM_JOINED}, {"code", code} };
    }
    static MsgRoomJoined parse(const json& j) {
        return { j.at("code").get<std::string>() };
    }
};

struct MsgPlayerJoined {
    std::string hostName;
    std::string guestName;
    int         count;

    static json build(const std::string& h, const std::string& g, int count) {
        return { {"type", MsgType::PLAYER_JOINED},
                 {"host_name", h}, {"guest_name", g}, {"count", count} };
    }
    static MsgPlayerJoined parse(const json& j) {
        return { j.at("host_name").get<std::string>(),
                 j.at("guest_name").get<std::string>(),
                 j.at("count").get<int>() };
    }
};

struct MsgRoleAssigned {
    std::string role;  // Role::PERFORMER or Role::COMMUNICATOR
    int gamePort;      // port the GameRoom process is listening on
    std::string game;  // "gyropi" or "notepi"

    static json build(const std::string& role, int gamePort,
                      const std::string& game = "gyropi") {
        return { {"type", MsgType::ROLE_ASSIGNED}, {"role", role},
                 {"game_port", gamePort}, {"game", game} };
    }
    static MsgRoleAssigned parse(const json& j) {
        return { j.at("role").get<std::string>(),
                 j.at("game_port").get<int>(),
                 j.value("game", "gyropi") };
    }
};

struct MsgError {
    std::string message;

    static json build(const std::string& message) {
        return { {"type", MsgType::ERROR}, {"message", message} };
    }
    static MsgError parse(const json& j) {
        return { j.at("message").get<std::string>() };
    }
};

struct MsgGameState {
    float ballX;
    float ballY;
    float pitch;   // echoed back so Performer display stays in sync via server
    float roll;
    int   level;

    static json build(float ballX, float ballY, float pitch, float roll, int level) {
        return {
            {"type",   MsgType::GAME_STATE},
            {"ball_x", ballX},
            {"ball_y", ballY},
            {"pitch",  pitch},
            {"roll",   roll},
            {"level",  level}
        };
    }
    static MsgGameState parse(const json& j) {
        return {
            j.at("ball_x").get<float>(),
            j.at("ball_y").get<float>(),
            j.at("pitch").get<float>(),
            j.at("roll").get<float>(),
            j.at("level").get<int>()
        };
    }
};

struct MsgGameOver {
    bool win;
    int  elapsedSecs   = 0;
    int  levelsReached = 1;
    std::vector<LeaderboardEntry> leaderboard;

    static json build(bool win, int elapsed = 0, int levels = 1,
                      const std::vector<LeaderboardEntry>& lb = {}) {
        json j = { {"type", MsgType::GAME_OVER}, {"win", win},
                   {"elapsed_secs", elapsed}, {"levels_reached", levels} };
        json lbArr = json::array();
        for (const auto& e : lb) {
            lbArr.push_back({
                {"players", e.players},
                {"elapsed_secs", e.elapsedSecs},
                {"levels_reached", e.levelsReached},
                {"won", e.won},
                {"date", e.date},
                {"game_type", e.gameType}
            });
        }
        j["leaderboard"] = lbArr;
        return j;
    }
    static MsgGameOver parse(const json& j) {
        MsgGameOver msg;
        msg.win           = j.at("win").get<bool>();
        msg.elapsedSecs   = j.value("elapsed_secs",   0);
        msg.levelsReached = j.value("levels_reached",  1);
        if (j.contains("leaderboard")) {
            for (const auto& e : j.at("leaderboard")) {
                msg.leaderboard.push_back({
                    e.value("players",        ""),
                    e.value("elapsed_secs",   0),
                    e.value("levels_reached", 1),
                    e.value("won",            false),
                    e.value("date",           ""),
                    e.value("game_type",      "")
                });
            }
        }
        return msg;
    }
};

// ─────────────────────────────────────────────
//  NotePi messages
// ─────────────────────────────────────────────

struct MsgNoteSubmit {
    std::vector<std::string> notes;

    static json build(const std::vector<std::string>& notes) {
        return { {"type", MsgType::NOTE_SUBMIT}, {"notes", notes} };
    }
    static MsgNoteSubmit parse(const json& j) {
        return { j.at("notes").get<std::vector<std::string>>() };
    }
};

struct NoteAttempt {
    std::vector<std::string> notes;
    std::vector<std::string> colors;  // "green", "yellow", "red"
};

struct MsgNoteState {
    int attempt;
    std::vector<std::string> notes;
    std::vector<std::string> colors;
    bool correct;
    std::vector<NoteAttempt> history;

    static json build(int attempt, const std::vector<std::string>& notes,
                      const std::vector<std::string>& colors, bool correct,
                      const std::vector<NoteAttempt>& history) {
        json histArr = json::array();
        for (const auto& h : history) {
            histArr.push_back({{"notes", h.notes}, {"colors", h.colors}});
        }
        return { {"type", MsgType::NOTE_STATE}, {"attempt", attempt},
                 {"notes", notes}, {"colors", colors},
                 {"correct", correct}, {"history", histArr} };
    }
    static MsgNoteState parse(const json& j) {
        MsgNoteState msg;
        msg.attempt = j.at("attempt").get<int>();
        msg.notes   = j.at("notes").get<std::vector<std::string>>();
        msg.colors  = j.at("colors").get<std::vector<std::string>>();
        msg.correct = j.at("correct").get<bool>();
        if (j.contains("history")) {
            for (const auto& h : j.at("history")) {
                msg.history.push_back({
                    h.at("notes").get<std::vector<std::string>>(),
                    h.at("colors").get<std::vector<std::string>>()
                });
            }
        }
        return msg;
    }
};

// ─────────────────────────────────────────────
//  Helper: get message type from raw JSON string
// ─────────────────────────────────────────────
inline std::string getMsgType(const std::string& raw) {
    return json::parse(raw).at("type").get<std::string>();
}
