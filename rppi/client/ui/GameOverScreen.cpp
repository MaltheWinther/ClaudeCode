#include "GameOverScreen.hpp"

#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QApplication>

GameOverScreen::GameOverScreen(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(24);

    resultLabel_ = new QLabel(this);
    resultLabel_->setAlignment(Qt::AlignCenter);

    subtitleLabel_ = new QLabel(this);
    subtitleLabel_->setAlignment(Qt::AlignCenter);
    subtitleLabel_->setStyleSheet("font-size: 24px; color: #666;");

    auto* playAgainBtn = new QPushButton("PLAY AGAIN", this);
    playAgainBtn->setFixedWidth(300);
    playAgainBtn->setFixedHeight(60);
    playAgainBtn->setStyleSheet(
        "font-size: 22px; background: #2196F3; color: white;"
        "border: none; border-radius: 10px;"
    );

    auto* exitBtn = new QPushButton("EXIT", this);
    exitBtn->setFixedWidth(300);
    exitBtn->setFixedHeight(50);
    exitBtn->setStyleSheet(
        "font-size: 18px; background: #757575; color: white;"
        "border: none; border-radius: 10px;"
    );

    layout->addStretch();
    layout->addWidget(resultLabel_);
    layout->addWidget(subtitleLabel_);
    layout->addSpacing(32);
    layout->addWidget(playAgainBtn, 0, Qt::AlignCenter);
    layout->addWidget(exitBtn,      0, Qt::AlignCenter);
    layout->addStretch();

    connect(playAgainBtn, &QPushButton::clicked, this, &GameOverScreen::playAgain);
    connect(exitBtn,      &QPushButton::clicked, []() { QApplication::quit(); });
}

void GameOverScreen::setResult(bool win) {
    if (win) {
        resultLabel_->setText("YOU WIN!");
        resultLabel_->setStyleSheet(
            "font-size: 72px; font-weight: bold; color: #4CAF50;"
        );
        subtitleLabel_->setText("Congratulations! You completed the maze.");
    } else {
        resultLabel_->setText("GAME OVER");
        resultLabel_->setStyleSheet(
            "font-size: 72px; font-weight: bold; color: #F44336;"
        );
        subtitleLabel_->setText("Better luck next time!");
    }
}
