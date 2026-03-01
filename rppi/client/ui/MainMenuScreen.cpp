#include "MainMenuScreen.hpp"

#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

MainMenuScreen::MainMenuScreen(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(16);

    auto* title = new QLabel("Ready Player Pi", this);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size: 36px; font-weight: bold; color: #2196F3;");

    usernameLabel_ = new QLabel(this);
    usernameLabel_->setAlignment(Qt::AlignCenter);
    usernameLabel_->setStyleSheet("font-size: 20px; color: #444;");

    errorLabel_ = new QLabel(this);
    errorLabel_->setAlignment(Qt::AlignCenter);
    errorLabel_->setStyleSheet("color: #F44336; font-size: 14px;");
    errorLabel_->hide();

    auto* hostBtn = new QPushButton("HOST ROOM", this);
    hostBtn->setFixedWidth(300);
    hostBtn->setFixedHeight(60);
    hostBtn->setStyleSheet(
        "font-size: 22px; background: #4CAF50; color: white;"
        "border: none; border-radius: 10px;"
    );

    auto* joinBtn = new QPushButton("JOIN ROOM", this);
    joinBtn->setFixedWidth(300);
    joinBtn->setFixedHeight(60);
    joinBtn->setStyleSheet(
        "font-size: 22px; background: #FF9800; color: white;"
        "border: none; border-radius: 10px;"
    );

    // Join panel — shown when JOIN ROOM is clicked
    joinPanel_ = new QWidget(this);
    auto* joinLayout = new QVBoxLayout(joinPanel_);
    joinLayout->setContentsMargins(0, 0, 0, 0);
    joinLayout->setSpacing(8);

    codeInput_ = new QLineEdit(joinPanel_);
    codeInput_->setPlaceholderText("Room code (e.g. AB3K)");
    codeInput_->setMaxLength(4);
    codeInput_->setFixedWidth(300);
    codeInput_->setFixedHeight(50);
    codeInput_->setAlignment(Qt::AlignCenter);
    codeInput_->setStyleSheet(
        "font-size: 26px; letter-spacing: 6px; padding: 8px;"
        "border: 2px solid #FF9800; border-radius: 8px;"
    );

    joinConfirmBtn_ = new QPushButton("JOIN", joinPanel_);
    joinConfirmBtn_->setFixedWidth(300);
    joinConfirmBtn_->setFixedHeight(50);
    joinConfirmBtn_->setStyleSheet(
        "font-size: 20px; background: #FF9800; color: white;"
        "border: none; border-radius: 8px;"
    );

    joinLayout->addWidget(codeInput_,      0, Qt::AlignCenter);
    joinLayout->addWidget(joinConfirmBtn_, 0, Qt::AlignCenter);
    joinPanel_->hide();

    layout->addStretch();
    layout->addWidget(title);
    layout->addWidget(usernameLabel_);
    layout->addWidget(errorLabel_);
    layout->addSpacing(24);
    layout->addWidget(hostBtn,   0, Qt::AlignCenter);
    layout->addWidget(joinBtn,   0, Qt::AlignCenter);
    layout->addWidget(joinPanel_, 0, Qt::AlignCenter);
    layout->addStretch();

    connect(hostBtn, &QPushButton::clicked, [this]() {
        joinPanel_->hide();
        clearError();
        emit hostClicked();
    });

    connect(joinBtn, &QPushButton::clicked, [this]() {
        joinPanel_->setVisible(!joinPanel_->isVisible());
        if (joinPanel_->isVisible()) codeInput_->setFocus();
    });

    auto confirmJoin = [this]() {
        QString code = codeInput_->text().trimmed().toUpper();
        if (code.isEmpty()) return;
        joinPanel_->hide();
        codeInput_->clear();
        emit joinClicked(code);
    };

    connect(joinConfirmBtn_, &QPushButton::clicked, confirmJoin);
    connect(codeInput_, &QLineEdit::returnPressed, confirmJoin);
}

void MainMenuScreen::setUsername(const QString& username) {
    usernameLabel_->setText("Welcome, " + username + "!");
}

void MainMenuScreen::setError(const QString& msg) {
    errorLabel_->setText(msg);
    errorLabel_->show();
}

void MainMenuScreen::clearError() {
    errorLabel_->clear();
    errorLabel_->hide();
}
