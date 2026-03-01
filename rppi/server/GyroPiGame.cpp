#include "GyroPiGame.hpp"

#include <cmath>
#include <algorithm>

// ── Constructor — defines level data ────────────────────────────────────────

GyroPiGame::GyroPiGame() {
    // ── Level 1 — 3 corridors, goal bottom-right ──────────────────────────
    Level l1;
    l1.walls = {
        // Border
        {0,   0,   600, 15},
        {0,   485, 600, 15},
        {0,   0,   15,  500},
        {585, 0,   15,  500},
        // Divider 1 — gap on RIGHT (x=415–585)
        {15,  165, 400, 15},
        // Divider 2 — gap on LEFT (x=15–200)
        {200, 330, 385, 15},
        // Extra wall in top corridor
        {180, 80,  160, 15},
        // Dead-end wall in middle corridor
        {360, 220, 15,  110},
    };
    l1.holes = {
        {280, 122, 18},
        {470, 255, 18},
        {100, 410, 18},
    };
    l1.goal   = {545, 435, 25};
    l1.startX = 45;
    l1.startY = 85;

    // ── Level 2 — 4 corridors, goal bottom-left, smaller goal ─────────────
    Level l2;
    l2.walls = {
        // Border
        {0,   0,   600, 15},
        {0,   485, 600, 15},
        {0,   0,   15,  500},
        {585, 0,   15,  500},
        // Divider 1 — gap on RIGHT (x=420–585)
        {15,  121, 405, 15},
        // Divider 2 — gap on LEFT (x=15–170)
        {170, 242, 415, 15},
        // Divider 3 — gap on RIGHT (x=420–585)
        {15,  363, 405, 15},
        // C1: horizontal wall (ball must go above)
        {200, 80,  180, 15},
        // C2: horizontal blocker
        {80,  196, 200, 15},
        // C3: horizontal block (ball must stay high)
        {100, 308, 260, 15},
        // C4: horizontal shelf
        {200, 415, 185, 15},
    };
    l2.holes = {
        {300, 100, 16},
        {490, 198, 16},
        {235, 338, 16},
        {300, 452, 16},
    };
    l2.goal   = {55, 438, 20};
    l2.startX = 45;
    l2.startY = 85;

    levels_ = { l1, l2 };

    // Start ball at level 1 start position
    bx_ = levels_[0].startX;
    by_ = levels_[0].startY;
}

// ── IGame interface ──────────────────────────────────────────────────────────

void GyroPiGame::update(float pitch, float roll, float dt) {
    if (finished_) return;

    dt = std::min(dt, MAX_DT);

    const Level& level = levels_[currentLevel_];

    // Accelerate from tilt (matching game.html exactly)
    vx_ += std::sin(roll  * M_PI / 180.f) * G * dt;
    vy_ += std::sin(pitch * M_PI / 180.f) * G * dt;

    // Rolling friction
    const float fric = std::exp(-ROLL_FRICTION * dt);
    vx_ *= fric;
    vy_ *= fric;

    // Move
    bx_ += vx_ * dt;
    by_ += vy_ * dt;

    // Wall collisions
    for (const Rect& w : level.walls)
        resolveWall(w);

    // Black holes — reset to start
    for (const Circle& h : level.holes) {
        if (dist(bx_, by_, h.x, h.y) < h.r + BALL_R) {
            bx_ = level.startX;
            by_ = level.startY;
            vx_ = 0.f;
            vy_ = 0.f;
            return;
        }
    }

    // Goal
    if (dist(bx_, by_, level.goal.x, level.goal.y) < level.goal.r) {
        const bool lastLevel = (currentLevel_ == static_cast<int>(levels_.size()) - 1);
        if (lastLevel) {
            finished_ = true;
            won_      = true;
        } else {
            // Advance to next level
            currentLevel_++;
            bx_ = levels_[currentLevel_].startX;
            by_ = levels_[currentLevel_].startY;
            vx_ = 0.f;
            vy_ = 0.f;
        }
    }
}

GameState GyroPiGame::getState() const {
    return { bx_, by_, currentLevel_ + 1 };  // level is 1-indexed for display
}

bool GyroPiGame::isFinished() const { return finished_; }
bool GyroPiGame::isWon()      const { return won_;      }

// ── Physics helpers ──────────────────────────────────────────────────────────

void GyroPiGame::resolveWall(const Rect& w) {
    // Closest point on the AABB to the ball centre
    const float cx = std::clamp(bx_, w.x, w.x + w.w);
    const float cy = std::clamp(by_, w.y, w.y + w.h);
    const float dx = bx_ - cx;
    const float dy = by_ - cy;
    const float d  = std::sqrt(dx * dx + dy * dy);

    if (d < BALL_R && d > 0.f) {
        // Push ball out of wall
        const float nx = dx / d;
        const float ny = dy / d;
        bx_ += nx * (BALL_R - d);
        by_ += ny * (BALL_R - d);

        // Reflect velocity component along normal (with energy loss)
        const float dot = vx_ * nx + vy_ * ny;
        if (dot < 0.f) {
            vx_ -= (1.f + WALL_BOUNCE) * dot * nx;
            vy_ -= (1.f + WALL_BOUNCE) * dot * ny;
        }
    }
}

float GyroPiGame::dist(float ax, float ay, float bx, float by) const {
    return std::sqrt((ax - bx) * (ax - bx) + (ay - by) * (ay - by));
}
