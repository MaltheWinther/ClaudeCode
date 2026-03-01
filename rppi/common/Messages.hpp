#pragma once

#include <string>
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

    // Server → Client (Lobby)
    constexpr auto ROOM_CREATED = "room_created";
    constexpr auto ROOM_JOINED  = "room_joined";
    constexpr auto ROLE_ASSIGNED = "role_assigned";
    constexpr auto ERROR        = "error";

    // Server → Client (Game)
    constexpr auto GAME_STATE   = "game_state";
    constexpr auto GAME_OVER    = "game_over";
}

// ─────────────────────────────────────────────
//  Role identifiers
// ─────────────────────────────────────────────
namespace Role {
    constexpr auto PERFORMER    = "performer";
    constexpr auto COMMUNICATOR = "communicator";
}

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
    static json build() {
        return { {"type", MsgType::START_GAME} };
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

struct MsgRoleAssigned {
    std::string role;  // Role::PERFORMER or Role::COMMUNICATOR
    int gamePort;      // port the GameRoom process is listening on

    static json build(const std::string& role, int gamePort) {
        return { {"type", MsgType::ROLE_ASSIGNED}, {"role", role}, {"game_port", gamePort} };
    }
    static MsgRoleAssigned parse(const json& j) {
        return { j.at("role").get<std::string>(), j.at("game_port").get<int>() };
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

    static json build(bool win) {
        return { {"type", MsgType::GAME_OVER}, {"win", win} };
    }
    static MsgGameOver parse(const json& j) {
        return { j.at("win").get<bool>() };
    }
};

// ─────────────────────────────────────────────
//  Helper: get message type from raw JSON string
// ─────────────────────────────────────────────
inline std::string getMsgType(const std::string& raw) {
    return json::parse(raw).at("type").get<std::string>();
}
