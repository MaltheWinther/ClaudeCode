#include "RoleScreen.hpp"

#include <QVBoxLayout>
#include <QLabel>
#include <QTimer>

RoleScreen::RoleScreen(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(16);

    auto* title = new QLabel("Get Ready!", this);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size: 36px; font-weight: bold; color: #2196F3;");

    gameLabel_ = new QLabel(this);
    gameLabel_->setAlignment(Qt::AlignCenter);
    gameLabel_->setStyleSheet("font-size: 16px; color: #888;");

    auto* yourRoleLabel = new QLabel("Your role for this game:", this);
    yourRoleLabel->setAlignment(Qt::AlignCenter);
    yourRoleLabel->setStyleSheet("font-size: 18px; color: #666;");

    roleLabel_ = new QLabel(this);
    roleLabel_->setAlignment(Qt::AlignCenter);
    roleLabel_->setStyleSheet(
        "font-size: 48px; font-weight: bold; color: #2196F3;"
        "background: #E3F2FD; border-radius: 12px; padding: 16px 40px;");

    descLabel_ = new QLabel(this);
    descLabel_->setAlignment(Qt::AlignCenter);
    descLabel_->setWordWrap(true);
    descLabel_->setStyleSheet("font-size: 18px; color: #444;");

    countdownLabel_ = new QLabel(this);
    countdownLabel_->setAlignment(Qt::AlignCenter);
    countdownLabel_->setStyleSheet(
        "font-size: 96px; font-weight: bold; color: #FF9800;");
    countdownLabel_->hide();

    layout->addStretch();
    layout->addWidget(title);
    layout->addWidget(gameLabel_);
    layout->addSpacing(16);
    layout->addWidget(yourRoleLabel);
    layout->addWidget(roleLabel_, 0, Qt::AlignCenter);
    layout->addSpacing(8);
    layout->addWidget(descLabel_);
    layout->addSpacing(24);
    layout->addWidget(countdownLabel_);
    layout->addStretch();
}

void RoleScreen::setRole(const QString& role, const QString& game) {
    gameLabel_->setText(game);
    roleLabel_->setText(role.toUpper());

    if (role == "communicator") {
        descLabel_->setText("You see the maze on screen.\nGuide your partner by voice!");
    } else {
        descLabel_->setText("You control the ball by tilting\nthe Raspberry Pi.");
    }

    // 3-2-1 countdown then emit done()
    countdown_ = 3;
    countdownLabel_->setText("3");
    countdownLabel_->show();

    if (countdownTimer_) {
        countdownTimer_->stop();
        countdownTimer_->deleteLater();
    }
    countdownTimer_ = new QTimer(this);
    countdownTimer_->setInterval(1000);
    connect(countdownTimer_, &QTimer::timeout, this, [this]() {
        --countdown_;
        if (countdown_ > 0) {
            countdownLabel_->setText(QString::number(countdown_));
        } else {
            countdownTimer_->stop();
            countdownLabel_->hide();
            emit done();
        }
    });
    countdownTimer_->start();
}
