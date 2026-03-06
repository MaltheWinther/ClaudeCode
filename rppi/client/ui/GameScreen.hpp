#pragma once

#include <QWidget>

class QTimer;

class GameScreen : public QWidget {
    Q_OBJECT
public:
    explicit GameScreen(QWidget* parent = nullptr);

    void setRole(const QString& role);

signals:
    void exitGame();

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

    // Black hole flash
    float   prevBallX_    = 45.f;
    float   prevBallY_    = 85.f;
    int     prevLevel_    = 1;
    bool    showHoleFlash_= false;
    QTimer* flashTimer_   = nullptr;

    // Maze canvas dimensions matching GyroPiGame
    static constexpr float MAZE_W = 600.f;
    static constexpr float MAZE_H = 500.f;
    static constexpr float BALL_R = 14.f;
};
