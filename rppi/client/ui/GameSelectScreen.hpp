#pragma once

#include <QWidget>

class QPushButton;

class GameSelectScreen : public QWidget {
    Q_OBJECT
public:
    explicit GameSelectScreen(QWidget* parent = nullptr);

    void setIsHost(bool isHost);

signals:
    void gameSelected(const QString& gameType);

private:
    QPushButton* gyroPiBtn_;
    QPushButton* notePiBtn_;
    bool isHost_ = false;
};
