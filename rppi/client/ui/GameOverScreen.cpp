#include "GameOverScreen.hpp"

#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QApplication>

GameOverScreen::GameOverScreen(QWidget* parent) : QWidget(parent) {
    auto* outerLayout = new QVBoxLayout(this);
    outerLayout->setAlignment(Qt::AlignCenter);
    outerLayout->setSpacing(16);

    resultLabel_ = new QLabel(this);
    resultLabel_->setAlignment(Qt::AlignCenter);

    subtitleLabel_ = new QLabel(this);
    subtitleLabel_->setAlignment(Qt::AlignCenter);
    subtitleLabel_->setStyleSheet("font-size: 24px; color: #666;");

    // Leaderboard container
    leaderboardWidget_ = new QWidget(this);
    leaderboardLayout_ = new QVBoxLayout(leaderboardWidget_);
    leaderboardLayout_->setSpacing(4);
    leaderboardWidget_->hide();

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

    outerLayout->addStretch();
    outerLayout->addWidget(resultLabel_);
    outerLayout->addWidget(subtitleLabel_);
    outerLayout->addSpacing(16);
    outerLayout->addWidget(leaderboardWidget_, 0, Qt::AlignCenter);
    outerLayout->addSpacing(16);
    outerLayout->addWidget(playAgainBtn, 0, Qt::AlignCenter);
    outerLayout->addWidget(exitBtn,      0, Qt::AlignCenter);
    outerLayout->addStretch();

    connect(playAgainBtn, &QPushButton::clicked, this, &GameOverScreen::playAgain);
    connect(exitBtn,      &QPushButton::clicked, []() { QApplication::quit(); });
}

void GameOverScreen::clearLeaderboard() {
    QLayoutItem* item;
    while ((item = leaderboardLayout_->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }
}

void GameOverScreen::setResult(bool win, int elapsedSecs, int levelsReached,
                               const std::vector<LeaderboardEntry>& leaderboard) {
    setStyleSheet("");  // reset disconnect styling

    if (win) {
        resultLabel_->setText("YOU WIN!");
        resultLabel_->setStyleSheet(
            "font-size: 72px; font-weight: bold; color: #4CAF50;");
    } else {
        resultLabel_->setText("GAME OVER");
        resultLabel_->setStyleSheet(
            "font-size: 72px; font-weight: bold; color: #F44336;");
    }

    subtitleLabel_->setStyleSheet("font-size: 24px; color: #666;");
    subtitleLabel_->setText(
        QString("Time: %1s  |  Level %2/2").arg(elapsedSecs).arg(levelsReached));

    // Build leaderboard
    clearLeaderboard();
    if (!leaderboard.empty()) {
        auto* title = new QLabel("── Top 5 ──", leaderboardWidget_);
        title->setAlignment(Qt::AlignCenter);
        title->setStyleSheet("font-size: 18px; color: #888; margin-bottom: 4px;");
        leaderboardLayout_->addWidget(title);

        int rank = 1;
        for (const auto& e : leaderboard) {
            QString line = QString("%1. %2  —  %3s, L%4 %5")
                .arg(rank++)
                .arg(QString::fromStdString(e.players))
                .arg(e.elapsedSecs)
                .arg(e.levelsReached)
                .arg(e.won ? "✓" : "✗");

            auto* lbl = new QLabel(line, leaderboardWidget_);
            lbl->setAlignment(Qt::AlignCenter);
            lbl->setStyleSheet("font-size: 16px; color: #ccc;");
            leaderboardLayout_->addWidget(lbl);
        }
        leaderboardWidget_->show();
    } else {
        leaderboardWidget_->hide();
    }
}

void GameOverScreen::setDisconnect(const QString& msg) {
    setStyleSheet("background-color: #1a1a00;");

    resultLabel_->setText("DISCONNECTED");
    resultLabel_->setStyleSheet(
        "font-size: 64px; font-weight: bold; color: #FF9800;");

    subtitleLabel_->setStyleSheet("font-size: 20px; color: #aaa;");
    subtitleLabel_->setText(msg);

    clearLeaderboard();
    leaderboardWidget_->hide();
}
