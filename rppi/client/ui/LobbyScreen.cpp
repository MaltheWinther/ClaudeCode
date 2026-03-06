#include "LobbyScreen.hpp"

#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>

LobbyScreen::LobbyScreen(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(20);

    auto* title = new QLabel("Game Lobby", this);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size: 32px; font-weight: bold; color: #2196F3;");

    auto* codeTitle = new QLabel("Room Code", this);
    codeTitle->setAlignment(Qt::AlignCenter);
    codeTitle->setStyleSheet("font-size: 18px; color: #666;");

    codeLabel_ = new QLabel(this);
    codeLabel_->setAlignment(Qt::AlignCenter);
    codeLabel_->setStyleSheet(
        "font-size: 64px; font-weight: bold; letter-spacing: 12px;"
        "color: #2196F3; background: #E3F2FD;"
        "padding: 16px 32px; border-radius: 12px;"
    );

    playerCountLabel_ = new QLabel(this);
    playerCountLabel_->setAlignment(Qt::AlignCenter);
    playerCountLabel_->setStyleSheet("font-size: 24px; color: #444;");

    statusLabel_ = new QLabel(this);
    statusLabel_->setAlignment(Qt::AlignCenter);
    statusLabel_->setStyleSheet("font-size: 16px; color: #888;");

    playBtn_ = new QPushButton("PLAY", this);
    playBtn_->setFixedWidth(300);
    playBtn_->setFixedHeight(70);
    playBtn_->hide();

    layout->addStretch();
    layout->addWidget(title);
    layout->addWidget(codeTitle);
    layout->addWidget(codeLabel_,       0, Qt::AlignCenter);
    layout->addWidget(playerCountLabel_);
    layout->addWidget(statusLabel_);
    auto* exitBtn = new QPushButton("BACK TO MAIN MENU", this);
    exitBtn->setFixedWidth(220);
    exitBtn->setFixedHeight(44);
    exitBtn->setStyleSheet(
        "font-size: 16px; background: #F44336; color: white;"
        "border: none; border-radius: 8px;");

    layout->addWidget(playBtn_,         0, Qt::AlignCenter);
    layout->addSpacing(10);
    layout->addWidget(exitBtn,          0, Qt::AlignCenter);
    layout->addStretch();

    connect(playBtn_, &QPushButton::clicked, this, &LobbyScreen::startClicked);
    connect(exitBtn,  &QPushButton::clicked, this, &LobbyScreen::exitLobby);
}

void LobbyScreen::setupHost(const QString& code, int playerCount) {
    isHost_ = true;
    codeLabel_->setText(code);
    playBtn_->show();
    setPlayerCount(playerCount);
}

void LobbyScreen::setupGuest(const QString& code) {
    isHost_ = false;
    codeLabel_->setText(code);
    playerCountLabel_->setText("1 / 2 players");
    statusLabel_->setText("Waiting for host to start the game...");
    playBtn_->hide();
}

void LobbyScreen::setPlayerCount(int count) {
    playerCountLabel_->setText(QString("%1 / 2 players").arg(count));
    if (!isHost_) return;

    if (count >= 2) {
        statusLabel_->setText("Both players connected! Press PLAY to start.");
        playBtn_->setEnabled(true);
        playBtn_->setStyleSheet(
            "font-size: 28px; background: #4CAF50; color: white;"
            "border: none; border-radius: 12px;"
        );
    } else {
        statusLabel_->setText("Waiting for another player to join...");
        playBtn_->setEnabled(false);
        playBtn_->setStyleSheet(
            "font-size: 28px; background: #9E9E9E; color: white;"
            "border: none; border-radius: 12px;"
        );
    }
}

void LobbyScreen::setPlayers(int count, const QString& hostName, const QString& guestName) {
    if (count >= 2) {
        playerCountLabel_->setText(
            QString("%1 & %2 (%3/2)").arg(hostName, guestName).arg(count));
    } else {
        playerCountLabel_->setText(
            QString("%1 (1/2)").arg(hostName));
    }

    if (!isHost_) return;

    if (count >= 2) {
        statusLabel_->setText("Both players connected! Press PLAY to start.");
        playBtn_->setEnabled(true);
        playBtn_->setStyleSheet(
            "font-size: 28px; background: #4CAF50; color: white;"
            "border: none; border-radius: 12px;"
        );
    } else {
        statusLabel_->setText("Waiting for another player to join...");
        playBtn_->setEnabled(false);
        playBtn_->setStyleSheet(
            "font-size: 28px; background: #9E9E9E; color: white;"
            "border: none; border-radius: 12px;"
        );
    }
}
