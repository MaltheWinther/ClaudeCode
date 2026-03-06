#pragma once

#include <string>
#include <vector>
#include "../common/Messages.hpp"

class NotePiGame {
public:
    NotePiGame();

    struct GuessResult {
        std::vector<std::string> colors;  // "green", "yellow", "red"
        bool correct;
    };

    GuessResult submitGuess(const std::vector<std::string>& guess);

    int  attemptCount()  const { return attemptCount_; }
    bool isFinished()    const { return finished_; }
    const std::vector<NoteAttempt>& history() const { return history_; }
    const std::vector<std::string>& target() const { return target_; }

    static const std::vector<std::string>& allNotes();

private:
    std::vector<std::string> target_;
    int  attemptCount_ = 0;
    bool finished_     = false;
    std::vector<NoteAttempt> history_;

    static constexpr int SEQ_LEN = 5;
};
