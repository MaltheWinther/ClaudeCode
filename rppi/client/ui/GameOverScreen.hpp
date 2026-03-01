#pragma once

#include <QWidget>

class QLabel;

class GameOverScreen : public QWidget {
    Q_OBJECT
public:
    explicit GameOverScreen(QWidget* parent = nullptr);

    void setResult(bool win);

signals:
    void playAgain();
    void exitGame();

private:
    QLabel* resultLabel_;
    QLabel* subtitleLabel_;
};
