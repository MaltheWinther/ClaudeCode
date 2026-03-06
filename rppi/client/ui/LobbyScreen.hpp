#pragma once

#include <QWidget>

class QLabel;
class QPushButton;

class LobbyScreen : public QWidget {
    Q_OBJECT
public:
    explicit LobbyScreen(QWidget* parent = nullptr);

    void setupHost(const QString& code, int playerCount);
    void setupGuest(const QString& code);
    void setPlayerCount(int count);
    void setPlayers(int count, const QString& hostName, const QString& guestName);

signals:
    void startClicked();
    void exitLobby();

private:
    QLabel*      codeLabel_;
    QLabel*      playerCountLabel_;
    QLabel*      statusLabel_;
    QPushButton* playBtn_;
    bool         isHost_ = false;
};
