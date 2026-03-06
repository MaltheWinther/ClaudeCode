#include "NotePiGame.hpp"
#include <random>
#include <algorithm>
#include <iostream>

const std::vector<std::string>& NotePiGame::allNotes() {
    static const std::vector<std::string> notes = {
        "C", "C#", "D", "D#", "E", "F",
        "F#", "G", "G#", "A", "A#", "B"
    };
    return notes;
}

NotePiGame::NotePiGame() {
    const auto& notes = allNotes();
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(0, (int)notes.size() - 1);

    target_.resize(SEQ_LEN);
    for (int i = 0; i < SEQ_LEN; ++i)
        target_[i] = notes[dist(rng)];

    std::cout << "[NotePi] Target: ";
    for (const auto& n : target_) std::cout << n << " ";
    std::cout << "\n";
}

NotePiGame::GuessResult NotePiGame::submitGuess(const std::vector<std::string>& guess) {
    ++attemptCount_;

    std::vector<std::string> colors(SEQ_LEN, "red");
    std::vector<bool> targetUsed(SEQ_LEN, false);
    std::vector<bool> guessUsed(SEQ_LEN, false);

    // First pass: exact matches (green)
    for (int i = 0; i < SEQ_LEN; ++i) {
        if (guess[i] == target_[i]) {
            colors[i] = "green";
            targetUsed[i] = true;
            guessUsed[i] = true;
        }
    }

    // Second pass: wrong position (yellow)
    for (int i = 0; i < SEQ_LEN; ++i) {
        if (guessUsed[i]) continue;
        for (int j = 0; j < SEQ_LEN; ++j) {
            if (targetUsed[j]) continue;
            if (guess[i] == target_[j]) {
                colors[i] = "yellow";
                targetUsed[j] = true;
                break;
            }
        }
    }

    bool correct = std::all_of(colors.begin(), colors.end(),
                               [](const std::string& c) { return c == "green"; });
    if (correct) finished_ = true;

    history_.push_back({guess, colors});

    return {colors, correct};
}
