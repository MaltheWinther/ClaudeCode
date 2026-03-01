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

    layout->addStretch();
    layout->addWidget(gameLabel_);
    layout->addSpacing(10);
    layout->addWidget(yourRoleLabel);
    layout->addWidget(roleLabel_);
    layout->addWidget(descLabel_);
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

    // Auto-advance after 2 seconds
    QTimer::singleShot(2000, this, &RoleScreen::done);
}
