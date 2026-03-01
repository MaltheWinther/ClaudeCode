#include "RoleScreen.hpp"

#include <QVBoxLayout>
#include <QLabel>
#include <QTimer>

RoleScreen::RoleScreen(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(20);

    gameLabel_ = new QLabel(this);
    gameLabel_->setAlignment(Qt::AlignCenter);
    gameLabel_->setStyleSheet("font-size: 28px; color: #aaa;");

    auto* yourRoleLabel = new QLabel("Your Role:", this);
    yourRoleLabel->setAlignment(Qt::AlignCenter);
    yourRoleLabel->setStyleSheet("font-size: 22px; color: #ccc;");

    roleLabel_ = new QLabel(this);
    roleLabel_->setAlignment(Qt::AlignCenter);
    roleLabel_->setStyleSheet("font-size: 64px; font-weight: bold; color: #FFD700;");

    descLabel_ = new QLabel(this);
    descLabel_->setAlignment(Qt::AlignCenter);
    descLabel_->setWordWrap(true);
    descLabel_->setStyleSheet("font-size: 18px; color: #aaa;");

    countdownLabel_ = new QLabel(this);
    countdownLabel_->setAlignment(Qt::AlignCenter);
    countdownLabel_->setStyleSheet(
        "font-size: 120px; font-weight: bold; color: #FFD700;");
    countdownLabel_->hide();

    layout->addStretch();
    layout->addWidget(gameLabel_);
    layout->addSpacing(10);
    layout->addWidget(yourRoleLabel);
    layout->addWidget(roleLabel_);
    layout->addWidget(descLabel_);
    layout->addSpacing(20);
    layout->addWidget(countdownLabel_);
    layout->addStretch();
}

void RoleScreen::setRole(const QString& role, const QString& game) {
    gameLabel_->setText(game);
    roleLabel_->setText(role.toUpper());

    if (role == "communicator") {
        descLabel_->setText("You see the maze on screen.\nGuide your partner by voice!");
        setStyleSheet("background-color: #0d2137;");
    } else {
        descLabel_->setText("You control the ball by tilting\nthe Raspberry Pi.");
        setStyleSheet("background-color: #1a2a0a;");
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
