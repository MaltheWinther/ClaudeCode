#pragma once

#include <QWidget>
#include <vector>
#include <string>
#include "../../common/Messages.hpp"

class QLabel;
class QPushButton;
class QVBoxLayout;

class NotePiScreen : public QWidget {
    Q_OBJECT
public:
    explicit NotePiScreen(QWidget* parent = nullptr);

    void setRole(const QString& role);
    void addDetectedNote(const QString& note);
    void updateNoteState(const MsgNoteState& state);

signals:
    void notesSubmitted(const std::vector<std::string>& notes);
    void exitGame();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void rebuildUI();
    void drawWordleGrid(QPainter& p);

    QString role_;
    std::vector<std::string> currentNotes_;  // notes being composed (performer)
    std::vector<NoteAttempt> history_;
    bool waitingForPerformer_ = true;

    // Performer controls
    QPushButton* recordBtn_ = nullptr;
    QPushButton* submitBtn_ = nullptr;
    QPushButton* clearBtn_  = nullptr;

    static constexpr int SEQ_LEN = 5;
    static constexpr int CELL_W = 80;
    static constexpr int CELL_H = 50;
    static constexpr int CELL_GAP = 6;
};
