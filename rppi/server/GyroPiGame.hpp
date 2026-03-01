#pragma once

#include "IGame.hpp"
#include <vector>

class GyroPiGame : public IGame {
public:
    GyroPiGame();

    void      update(float pitch, float roll, float dt) override;
    GameState getState()    const override;
    bool      isFinished()  const override;
    bool      isWon()       const override;

private:
    // ── Level data types ─────────────────────────────────────────────────
    struct Rect   { float x, y, w, h; };
    struct Circle { float x, y, r;    };

    struct Level {
        std::vector<Rect>   walls;
        std::vector<Circle> holes;
        Circle              goal;
        float               startX, startY;
    };

    // ── Physics helpers ──────────────────────────────────────────────────
    void  resolveWall(const Rect& wall);
    float dist(float ax, float ay, float bx, float by) const;

    // ── State ────────────────────────────────────────────────────────────
    std::vector<Level> levels_;
    int   currentLevel_ = 0;
    float bx_, by_;         // ball position
    float vx_ = 0.f;        // ball velocity
    float vy_ = 0.f;
    bool  finished_ = false;
    bool  won_      = false;

    // ── Physics constants (matching game.html) ───────────────────────────
    static constexpr float BALL_R       = 14.f;
    static constexpr float G            = 1350.f;
    static constexpr float ROLL_FRICTION = 2.0f;
    static constexpr float WALL_BOUNCE  = 0.25f;
    static constexpr float MAX_DT       = 0.05f;  // cap dt to avoid lag spikes
};
