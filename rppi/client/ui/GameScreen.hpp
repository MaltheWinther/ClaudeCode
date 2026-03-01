#pragma once

#include <QWidget>

class GameScreen : public QWidget {
    Q_OBJECT
public:
    explicit GameScreen(QWidget* parent = nullptr);

    void setRole(const QString& role);

public slots:
    void updateGameState(float ballX, float ballY, float pitch, float roll, int level);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void drawCommunicatorView(QPainter& p);
    void drawPerformerView(QPainter& p);

    QString role_;
    float   ballX_ = 45.f;
    float   ballY_ = 85.f;
    float   pitch_ = 0.f;
    float   roll_  = 0.f;
    int     level_ = 1;

    // Maze canvas dimensions matching GyroPiGame
    static constexpr float MAZE_W = 600.f;
    static constexpr float MAZE_H = 500.f;
    static constexpr float BALL_R = 14.f;
};
