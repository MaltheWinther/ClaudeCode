#include "GameSelectScreen.hpp"

#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>

GameSelectScreen::GameSelectScreen(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(30);

    auto* title = new QLabel("Select Game", this);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size: 36px; font-weight: bold; color: #2196F3;");

    auto* subtitle = new QLabel("Host picks the game", this);
    subtitle->setAlignment(Qt::AlignCenter);
    subtitle->setStyleSheet("font-size: 16px; color: #888;");

    auto btnStyle = [](const QString& color) {
        return QString(
            "font-size: 24px; font-weight: bold; color: white;"
            "background: %1; border: none; border-radius: 12px;"
            "padding: 20px;"
        ).arg(color);
    };

    gyroPiBtn_ = new QPushButton("GAME 1 - GyroPi", this);
    gyroPiBtn_->setFixedSize(350, 80);
    gyroPiBtn_->setStyleSheet(btnStyle("#4CAF50"));

    notePiBtn_ = new QPushButton("GAME 2 - NotePi", this);
    notePiBtn_->setFixedSize(350, 80);
    notePiBtn_->setStyleSheet(btnStyle("#FF9800"));

    layout->addStretch();
    layout->addWidget(title);
    layout->addWidget(subtitle);
    layout->addSpacing(20);
    layout->addWidget(gyroPiBtn_, 0, Qt::AlignCenter);
    layout->addWidget(notePiBtn_, 0, Qt::AlignCenter);
    layout->addStretch();

    connect(gyroPiBtn_, &QPushButton::clicked, this, [this]() {
        emit gameSelected("gyropi");
    });
    connect(notePiBtn_, &QPushButton::clicked, this, [this]() {
        emit gameSelected("notepi");
    });
}

void GameSelectScreen::setIsHost(bool isHost) {
    isHost_ = isHost;
    gyroPiBtn_->setEnabled(isHost);
    notePiBtn_->setEnabled(isHost);

    auto disabledStyle =
        "font-size: 24px; font-weight: bold; color: white;"
        "background: #9E9E9E; border: none; border-radius: 12px;"
        "padding: 20px;";

    if (!isHost) {
        gyroPiBtn_->setStyleSheet(disabledStyle);
        notePiBtn_->setStyleSheet(disabledStyle);
    }
}
