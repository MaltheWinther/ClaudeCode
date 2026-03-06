#include "NotePiScreen.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QPainter>
#include <QMouseEvent>
#include <QFont>

NotePiScreen::NotePiScreen(QWidget* parent) : QWidget(parent) {}

void NotePiScreen::setRole(const QString& role) {
    role_ = role;
    currentNotes_.clear();
    history_.clear();
    waitingForPerformer_ = true;

    // Remove old layout
    delete layout();
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setAlignment(Qt::AlignCenter);
    mainLayout->setSpacing(10);

    auto* title = new QLabel("NotePi", this);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size: 36px; font-weight: bold; color: #FF9800;");
    mainLayout->addWidget(title);

    auto* roleLabel = new QLabel(
        role_ == "performer" ? "You are the PERFORMER - Play notes!"
                             : "You are the COMMUNICATOR - Watch the results!",
        this);
    roleLabel->setAlignment(Qt::AlignCenter);
    roleLabel->setStyleSheet("font-size: 16px; color: #888;");
    mainLayout->addWidget(roleLabel);

    // Spacer for the paint area (Wordle grid)
    mainLayout->addStretch(1);

    if (role_ == "performer") {
        auto* btnLayout = new QHBoxLayout();
        btnLayout->setAlignment(Qt::AlignCenter);

        clearBtn_ = new QPushButton("CLEAR", this);
        clearBtn_->setFixedSize(120, 50);
        clearBtn_->setStyleSheet(
            "font-size: 18px; background: #F44336; color: white;"
            "border: none; border-radius: 8px;");

        submitBtn_ = new QPushButton("SUBMIT", this);
        submitBtn_->setFixedSize(160, 50);
        submitBtn_->setStyleSheet(
            "font-size: 18px; font-weight: bold; background: #4CAF50; color: white;"
            "border: none; border-radius: 8px;");
        submitBtn_->setEnabled(false);

        btnLayout->addWidget(clearBtn_);
        btnLayout->addSpacing(20);
        btnLayout->addWidget(submitBtn_);
        mainLayout->addLayout(btnLayout);

        connect(clearBtn_, &QPushButton::clicked, this, [this]() {
            currentNotes_.clear();
            submitBtn_->setEnabled(false);
            update();
        });

        connect(submitBtn_, &QPushButton::clicked, this, [this]() {
            if ((int)currentNotes_.size() == SEQ_LEN) {
                emit notesSubmitted(currentNotes_);
                currentNotes_.clear();
                submitBtn_->setEnabled(false);
                update();
            }
        });
    }

    auto* exitBtn = new QPushButton("EXIT", this);
    exitBtn->setFixedSize(80, 36);
    exitBtn->setStyleSheet(
        "font-size: 14px; background: #F44336; color: white;"
        "border: none; border-radius: 6px;");
    mainLayout->addSpacing(10);
    mainLayout->addWidget(exitBtn, 0, Qt::AlignCenter);
    connect(exitBtn, &QPushButton::clicked, this, &NotePiScreen::exitGame);

    update();
}

void NotePiScreen::addDetectedNote(const QString& note) {
    if (role_ != "performer") return;
    if ((int)currentNotes_.size() >= SEQ_LEN) return;

    currentNotes_.push_back(note.toStdString());

    if (submitBtn_ && (int)currentNotes_.size() == SEQ_LEN)
        submitBtn_->setEnabled(true);

    update();
}

void NotePiScreen::updateNoteState(const MsgNoteState& state) {
    history_ = state.history;
    waitingForPerformer_ = false;
    update();
}

void NotePiScreen::mousePressEvent(QMouseEvent* event) {
    if (role_ != "performer") return;

    int gridW = SEQ_LEN * (CELL_W + CELL_GAP) - CELL_GAP;
    int startX = (width() - gridW) / 2;
    int startY = 120;
    int row = (int)history_.size();  // current input row

    int mx = event->pos().x();
    int my = event->pos().y();
    int rowY = startY + row * (CELL_H + CELL_GAP);

    if (my < rowY || my > rowY + CELL_H) return;

    for (int col = 0; col < (int)currentNotes_.size(); ++col) {
        int cellX = startX + col * (CELL_W + CELL_GAP);
        if (mx >= cellX && mx <= cellX + CELL_W) {
            currentNotes_.erase(currentNotes_.begin() + col);
            if (submitBtn_) submitBtn_->setEnabled((int)currentNotes_.size() == SEQ_LEN);
            update();
            return;
        }
    }
}

void NotePiScreen::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    drawWordleGrid(p);
}

void NotePiScreen::drawWordleGrid(QPainter& p) {
    int gridW = SEQ_LEN * (CELL_W + CELL_GAP) - CELL_GAP;
    int startX = (width() - gridW) / 2;
    int startY = 120;

    QFont noteFont("Arial", 16, QFont::Bold);
    p.setFont(noteFont);

    // Draw history rows
    for (int row = 0; row < (int)history_.size(); ++row) {
        const auto& attempt = history_[row];
        for (int col = 0; col < SEQ_LEN && col < (int)attempt.notes.size(); ++col) {
            int x = startX + col * (CELL_W + CELL_GAP);
            int y = startY + row * (CELL_H + CELL_GAP);

            QColor bg;
            if (role_ == "performer") {
                bg = QColor(58, 58, 60);     // no color feedback for performer
            } else if (attempt.colors[col] == "green")
                bg = QColor(83, 141, 78);    // #538d4e
            else if (attempt.colors[col] == "yellow")
                bg = QColor(181, 159, 59);   // #b59f3b
            else
                bg = QColor(58, 58, 60);     // #3a3a3c

            p.setBrush(bg);
            p.setPen(Qt::NoPen);
            p.drawRoundedRect(x, y, CELL_W, CELL_H, 6, 6);

            p.setPen(Qt::white);
            p.drawText(QRect(x, y, CELL_W, CELL_H), Qt::AlignCenter,
                       QString::fromStdString(attempt.notes[col]));
        }
    }

    // Draw current input row (performer only)
    if (role_ == "performer") {
        int row = (int)history_.size();
        for (int col = 0; col < SEQ_LEN; ++col) {
            int x = startX + col * (CELL_W + CELL_GAP);
            int y = startY + row * (CELL_H + CELL_GAP);

            bool filled = col < (int)currentNotes_.size();
            QColor bg = filled ? QColor(120, 120, 120) : QColor(40, 40, 40);
            QColor border = QColor(80, 80, 80);

            p.setBrush(bg);
            p.setPen(QPen(border, 2));
            p.drawRoundedRect(x, y, CELL_W, CELL_H, 6, 6);

            if (filled) {
                p.setPen(Qt::white);
                p.drawText(QRect(x, y, CELL_W, CELL_H), Qt::AlignCenter,
                           QString::fromStdString(currentNotes_[col]));
            }
        }
    }

    // If communicator and waiting
    if (role_ != "performer" && waitingForPerformer_ && history_.empty()) {
        p.setPen(QColor(150, 150, 150));
        QFont waitFont("Arial", 20);
        p.setFont(waitFont);
        p.drawText(QRect(0, startY, width(), 60), Qt::AlignCenter,
                   "Waiting for Performer...");
    }
}
