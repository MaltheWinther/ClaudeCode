#pragma once

// ── GameState ────────────────────────────────────────────────────────────────
// Plain data struct passed from IGame::getState() to GameRoom for broadcasting.
// Extend this as new games need additional fields.

struct GameState {
    float ballX  = 0.f;
    float ballY  = 0.f;
    int   level  = 1;
};

// ── IGame ────────────────────────────────────────────────────────────────────
// Interface for all game types hosted by a GameRoom.
// GameRoom owns one IGame instance and drives it via update() at ~40Hz.

class IGame {
public:
    virtual ~IGame() = default;

    // Advance game logic by dt seconds with the current tilt input.
    // Called by the game loop thread at ~40Hz.
    virtual void update(float pitch, float roll, float dt) = 0;

    // Return a snapshot of the current game state for broadcasting.
    virtual GameState getState() const = 0;

    // True once the game has reached a terminal state (win or loss).
    virtual bool isFinished() const = 0;

    // True if the terminal state is a win. Only meaningful if isFinished().
    virtual bool isWon() const = 0;
};
