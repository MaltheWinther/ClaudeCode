#include "UsernameScreen.hpp"

#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QApplication>

UsernameScreen::UsernameScreen(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(20);

    auto* title = new QLabel("Ready Player Pi", this);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size: 36px; font-weight: bold; color: #2196F3;");

    auto* subtitle = new QLabel("Enter your username to continue", this);
    subtitle->setAlignment(Qt::AlignCenter);
    subtitle->setStyleSheet("font-size: 18px; color: #666;");

    input_ = new QLineEdit(this);
    input_->setPlaceholderText("Username (1-12 alphanumeric)");
    input_->setMaxLength(12);
    input_->setFixedWidth(320);
    input_->setFixedHeight(52);
    input_->setAlignment(Qt::AlignCenter);
    input_->setStyleSheet(
        "font-size: 20px; padding: 8px;"
        "border: 2px solid #ccc; border-radius: 8px;"
    );

    errorLabel_ = new QLabel(this);
    errorLabel_->setAlignment(Qt::AlignCenter);
    errorLabel_->setStyleSheet("color: #F44336; font-size: 14px;");
    errorLabel_->hide();

    auto* saveBtn = new QPushButton("SAVE", this);
    saveBtn->setFixedWidth(320);
    saveBtn->setFixedHeight(52);
    saveBtn->setStyleSheet(
        "font-size: 20px; background: #2196F3; color: white;"
        "border: none; border-radius: 8px;"
    );

    auto* exitBtn = new QPushButton("EXIT", this);
    exitBtn->setFixedWidth(320);
    exitBtn->setFixedHeight(42);
    exitBtn->setStyleSheet(
        "font-size: 16px; background: #757575; color: white;"
        "border: none; border-radius: 8px;"
    );

    layout->addStretch();
    layout->addWidget(title);
    layout->addWidget(subtitle);
    layout->addSpacing(20);
    layout->addWidget(input_,     0, Qt::AlignCenter);
    layout->addWidget(errorLabel_);
    layout->addWidget(saveBtn,    0, Qt::AlignCenter);
    layout->addWidget(exitBtn,    0, Qt::AlignCenter);
    layout->addStretch();

    connect(saveBtn, &QPushButton::clicked, [this]() {
        emit submitted(input_->text().trimmed());
    });
    connect(input_, &QLineEdit::returnPressed, [this]() {
        emit submitted(input_->text().trimmed());
    });
    connect(exitBtn, &QPushButton::clicked, []() {
        QApplication::quit();
    });
}

void UsernameScreen::setError(const QString& msg) {
    errorLabel_->setText(msg);
    errorLabel_->show();
}

void UsernameScreen::clearError() {
    errorLabel_->clear();
    errorLabel_->hide();
}
