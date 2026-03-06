#pragma once

#include <string>
#include <functional>
#include <atomic>
#include <mutex>
#include <thread>

class NoteReader {
public:
    struct NoteEvent {
        std::string note;
        float confidence;
    };

    NoteReader();
    ~NoteReader();

    void start();
    void stop();
    void setOnNoteDetected(std::function<void(NoteEvent)> cb);

private:
    void loop();
    std::string frequencyToNote(float freq) const;

    std::thread thread_;
    std::atomic<bool> running_{ false };

    std::function<void(NoteEvent)> onNote_;
    std::mutex cbMutex_;

    static constexpr float  MIN_CONFIDENCE = 0.8f;
    static constexpr float  MIN_RMS = 0.005f;  // low threshold to allow phone speakers

    // Sustained-note detection: note must be held for SUSTAIN_MS before accepted
    std::string candidateNote_;
    double candidateStartMs_ = 0.0;
    bool candidateFired_ = false;
    double cooldownUntilMs_ = 0.0;
    static constexpr double SUSTAIN_MS = 350.0;
    static constexpr double COOLDOWN_MS = 400.0;  // ignore input after accepting a note
};
