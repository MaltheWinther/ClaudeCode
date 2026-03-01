#include "GameScreen.hpp"

#include <QPainter>
#include <QPaintEvent>
#include <QRadialGradient>
#include <QFont>
#include <vector>
#include <cmath>
#include <algorithm>

// ── Maze data matching GyroPiGame.cpp ───────────────────────────────────────
// The server computes physics; the client only needs the static layout for drawing.

struct WallRect   { float x, y, w, h; };
struct HoleCircle { float x, y, r;    };
struct GoalCircle { float x, y, r;    };

struct LevelData {
    std::vector<WallRect>   walls;
    std::vector<HoleCircle> holes;
    GoalCircle              goal;
};

static const std::vector<LevelData> s_levels = {
    // ── Level 1 ──────────────────────────────────────────────────────────
    {
        {   // walls
            {0,   0,   600, 15}, {0,  485, 600, 15},
            {0,   0,   15,  500},{585, 0,  15,  500},
            {15,  165, 400, 15}, {200,330, 385, 15},
            {180, 80,  160, 15}, {360,220, 15,  110},
        },
        {   // holes
            {280, 122, 18}, {470, 255, 18}, {100, 410, 18},
        },
        {545, 435, 25}  // goal
    },
    // ── Level 2 ──────────────────────────────────────────────────────────
    {
        {   // walls
            {0,   0,   600, 15}, {0,  485, 600, 15},
            {0,   0,   15,  500},{585, 0,  15,  500},
            {15,  121, 405, 15}, {170,242, 415, 15},
            {15,  363, 405, 15}, {200, 80, 180, 15},
            {80,  196, 200, 15}, {100,308, 260, 15},
            {200, 415, 185, 15},
        },
        {   // holes
            {300, 100, 16}, {490, 198, 16}, {235, 338, 16}, {300, 452, 16},
        },
        {55, 438, 20}   // goal
    },
};

// ── GameScreen ───────────────────────────────────────────────────────────────

GameScreen::GameScreen(QWidget* parent) : QWidget(parent) {
    setStyleSheet("background-color: #111;");
}

void GameScreen::setRole(const QString& role) {
    role_ = role;
    update();
}

void GameScreen::updateGameState(float ballX, float ballY,
                                 float pitch, float roll, int level) {
    ballX_ = ballX;
    ballY_ = ballY;
    pitch_ = pitch;
    roll_  = roll;
    level_ = level;
    update();
}

void GameScreen::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    if (role_ == "communicator")
        drawCommunicatorView(p);
    else
        drawPerformerView(p);
}

void GameScreen::drawCommunicatorView(QPainter& p) {
    // Scale the 600×500 maze canvas to fit the widget, centred
    const float scaleX = float(width())  / MAZE_W;
    const float scaleY = float(height()) / MAZE_H;
    const float scale  = std::min(scaleX, scaleY) * 0.95f;
    const float offX   = (width()  - MAZE_W * scale) / 2.f;
    const float offY   = (height() - MAZE_H * scale) / 2.f;

    auto toScreen = [&](float x, float y) -> QPointF {
        return { offX + x * scale, offY + y * scale };
    };

    // Background
    p.fillRect(rect(), QColor(28, 28, 40));

    // Level indicator
    p.setPen(QColor(200, 200, 220));
    p.setFont(QFont("Arial", 16, QFont::Bold));
    p.drawText(QRect(0, 6, width(), 30), Qt::AlignCenter,
               QString("Level %1 / %2").arg(level_).arg(int(s_levels.size())));

    // Clamp level index
    const int lvlIdx     = std::clamp(level_ - 1, 0, int(s_levels.size()) - 1);
    const LevelData& lvl = s_levels[lvlIdx];

    // Draw walls
    p.setPen(Qt::NoPen);
    for (const auto& w : lvl.walls) {
        QPointF tl = toScreen(w.x, w.y);
        p.fillRect(QRectF(tl, QSizeF(w.w * scale, w.h * scale)), QColor(70, 100, 150));
    }

    // Draw holes (dark circles with inner glow)
    for (const auto& h : lvl.holes) {
        QPointF c  = toScreen(h.x, h.y);
        double  r  = double(h.r * scale);
        QRadialGradient hg(c, r);
        hg.setColorAt(0.0, QColor(5,   5,   5));
        hg.setColorAt(0.6, QColor(20,  20,  20));
        hg.setColorAt(1.0, QColor(50,  50,  80));
        p.setBrush(hg);
        p.drawEllipse(c, r, r);
    }

    // Draw goal (gold pulsing circle)
    {
        QPointF c = toScreen(lvl.goal.x, lvl.goal.y);
        double  r = double(lvl.goal.r * scale);
        QRadialGradient gg(c, r);
        gg.setColorAt(0.0, QColor(255, 240, 100));
        gg.setColorAt(0.7, QColor(255, 180,   0));
        gg.setColorAt(1.0, QColor(200, 120,   0));
        p.setBrush(gg);
        p.setPen(QPen(QColor(255, 200, 50), 2));
        p.drawEllipse(c, r, r);
        p.setPen(QColor(80, 40, 0));
        p.setFont(QFont("Arial", int(r * 1.1)));
        p.drawText(QRectF(c.x() - r, c.y() - r, r * 2, r * 2), Qt::AlignCenter, "★");
    }

    // Draw ball
    {
        QPointF  bc = toScreen(ballX_, ballY_);
        double   r  = double(BALL_R * scale);
        QRadialGradient bg(bc - QPointF(r * 0.3, r * 0.3), r);
        bg.setColorAt(0.0, QColor(255, 160, 120));
        bg.setColorAt(0.6, QColor(230,  80,  30));
        bg.setColorAt(1.0, QColor(160,  30,   5));
        p.setPen(Qt::NoPen);
        p.setBrush(bg);
        p.drawEllipse(bc, r, r);
    }
}

void GameScreen::drawPerformerView(QPainter& p) {
    p.fillRect(rect(), QColor(18, 30, 18));

    const int cx = width()  / 2;
    const int cy = height() / 2;
    const int r  = std::min(width(), height()) / 3;

    // Title
    p.setPen(QColor(160, 230, 140));
    p.setFont(QFont("Arial", 20, QFont::Bold));
    p.drawText(QRect(0, 0, width(), 60), Qt::AlignCenter, "TILT CONTROLLER");

    // Outer reference ring
    p.setPen(QPen(QColor(50, 110, 50), 3));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(QPoint(cx, cy), r, r);

    // Crosshair
    p.setPen(QPen(QColor(50, 110, 50), 1, Qt::DashLine));
    p.drawLine(cx - r, cy, cx + r, cy);
    p.drawLine(cx, cy - r, cx, cy + r);

    // Tilt indicator dot: roll → X, pitch → Y (clamped to ±45°)
    constexpr float MAX_TILT = 45.f;
    const int dotX = cx + int(std::clamp(roll_,  -MAX_TILT, MAX_TILT) / MAX_TILT * r);
    const int dotY = cy + int(std::clamp(pitch_, -MAX_TILT, MAX_TILT) / MAX_TILT * r);

    QRadialGradient dg(QPointF(dotX, dotY), 22);
    dg.setColorAt(0.0, QColor(160, 255, 100));
    dg.setColorAt(1.0, QColor( 40, 180,  20));
    p.setPen(Qt::NoPen);
    p.setBrush(dg);
    p.drawEllipse(QPoint(dotX, dotY), 22, 22);

    // Angle readout
    p.setPen(QColor(140, 210, 120));
    p.setFont(QFont("Arial", 16));
    p.drawText(QRect(0, height() - 80, width(), 40), Qt::AlignCenter,
               QString("Pitch: %1°   Roll: %2°")
                   .arg(double(pitch_), 0, 'f', 1)
                   .arg(double(roll_),  0, 'f', 1));

    p.setFont(QFont("Arial", 14));
    p.drawText(QRect(0, height() - 40, width(), 40), Qt::AlignCenter,
               QString("Level %1").arg(level_));
}
