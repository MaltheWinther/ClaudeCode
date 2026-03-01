#pragma once

#include <vector>
#include <QWidget>
#include "../../common/Messages.hpp"

class QLabel;
class QVBoxLayout;

class GameOverScreen : public QWidget {
    Q_OBJECT
public:
    explicit GameOverScreen(QWidget* parent = nullptr);

    void setResult(bool win, int elapsedSecs, int levelsReached,
                   const std::vector<LeaderboardEntry>& leaderboard);
    void setDisconnect(const QString& msg);

signals:
    void playAgain();
    void exitGame();

private:
    void clearLeaderboard();

    QLabel*      resultLabel_;
    QLabel*      subtitleLabel_;
    QVBoxLayout* leaderboardLayout_;
    QWidget*     leaderboardWidget_;
};
