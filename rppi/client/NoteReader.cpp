#include "NoteReader.hpp"

#include <aubio/aubio.h>
#include <chrono>
#include <cmath>
#include <iostream>

static double nowMs() {
    using namespace std::chrono;
    return duration<double, std::milli>(
        steady_clock::now().time_since_epoch()).count();
}

// ── Common methods (all platforms) ───────────────────────────────────────────

NoteReader::NoteReader() {}
NoteReader::~NoteReader() { stop(); }

void NoteReader::start() {
    running_ = true;
    thread_ = std::thread(&NoteReader::loop, this);
}

void NoteReader::stop() {
    running_ = false;
    if (thread_.joinable()) thread_.join();
}

void NoteReader::setOnNoteDetected(std::function<void(NoteEvent)> cb) {
    std::lock_guard<std::mutex> lk(cbMutex_);
    onNote_ = std::move(cb);
}

std::string NoteReader::frequencyToNote(float freq) const {
    // C2 (65 Hz) to C8 (4186 Hz) — covers nearly all piano keys
    if (freq < 65.0f || freq > 4200.0f) return "";

    static const char* noteNames[] = {
        "C", "C#", "D", "D#", "E", "F",
        "F#", "G", "G#", "A", "A#", "B"
    };

    float midi = 12.0f * std::log2(freq / 440.0f) + 69.0f;
    int midiNote = (int)std::round(midi);

    int noteIdx = ((midiNote % 12) + 12) % 12;

    return std::string(noteNames[noteIdx]);
}

// ── Platform-specific loop() ─────────────────────────────────────────────────

#ifdef __APPLE__
// ── macOS: CoreAudio (AudioQueue) + aubio ────────────────────────────────────
#include <AudioToolbox/AudioToolbox.h>
#include <vector>
#include <condition_variable>

static constexpr unsigned int SAMPLE_RATE = 44100;
static constexpr unsigned int HOP_SIZE    = 512;
static constexpr unsigned int BUF_SIZE    = 2048;

// Shared ring buffer: callback writes samples, loop() reads them
struct AudioRingBuffer {
    std::mutex mtx;
    std::condition_variable cv;
    std::vector<float> data;
    std::atomic<bool>* running;
};

static void noteReaderAudioCallback(
    void* userData,
    AudioQueueRef queue,
    AudioQueueBufferRef buffer,
    const AudioTimeStamp*,
    UInt32,
    const AudioStreamPacketDescription*)
{
    auto* ring = static_cast<AudioRingBuffer*>(userData);
    auto* samples = static_cast<float*>(buffer->mAudioData);
    UInt32 sampleCount = buffer->mAudioDataByteSize / sizeof(float);

    {
        std::lock_guard<std::mutex> lk(ring->mtx);
        ring->data.insert(ring->data.end(), samples, samples + sampleCount);
    }
    ring->cv.notify_one();

    if (ring->running->load())
        AudioQueueEnqueueBuffer(queue, buffer, 0, nullptr);
}

void NoteReader::loop() {
    aubio_pitch_t* pitch = new_aubio_pitch("yin", BUF_SIZE, HOP_SIZE, SAMPLE_RATE);
    aubio_pitch_set_unit(pitch, "Hz");
    aubio_pitch_set_tolerance(pitch, 0.7f);

    fvec_t* inputBuf  = new_fvec(HOP_SIZE);
    fvec_t* outputBuf = new_fvec(1);

    AudioRingBuffer ring;
    ring.running = &running_;

    // Set up AudioQueue for mic input
    AudioStreamBasicDescription fmt = {};
    fmt.mSampleRate       = SAMPLE_RATE;
    fmt.mFormatID         = kAudioFormatLinearPCM;
    fmt.mFormatFlags      = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked;
    fmt.mBitsPerChannel   = 32;
    fmt.mChannelsPerFrame = 1;
    fmt.mBytesPerFrame    = sizeof(float);
    fmt.mFramesPerPacket  = 1;
    fmt.mBytesPerPacket   = sizeof(float);

    AudioQueueRef queue;
    OSStatus err = AudioQueueNewInput(&fmt, noteReaderAudioCallback, &ring,
                                       nullptr, nullptr, 0, &queue);
    if (err != noErr) {
        std::cerr << "[NoteReader] AudioQueueNewInput failed: " << err << "\n";
        del_fvec(inputBuf); del_fvec(outputBuf); del_aubio_pitch(pitch);
        return;
    }

    const int NUM_BUFFERS = 3;
    const UInt32 bufferBytes = HOP_SIZE * 4 * sizeof(float);
    for (int i = 0; i < NUM_BUFFERS; i++) {
        AudioQueueBufferRef buf;
        AudioQueueAllocateBuffer(queue, bufferBytes, &buf);
        AudioQueueEnqueueBuffer(queue, buf, 0, nullptr);
    }

    AudioQueueStart(queue, nullptr);
    std::cout << "[NoteReader] Listening on microphone (CoreAudio)...\n";

    // Process audio from ring buffer
    std::vector<float> local;
    while (running_) {
        {
            std::unique_lock<std::mutex> lk(ring.mtx);
            ring.cv.wait_for(lk, std::chrono::milliseconds(100),
                             [&]{ return ring.data.size() >= HOP_SIZE || !running_; });
            if (!running_) break;
            if (ring.data.size() < HOP_SIZE) continue;
            local.swap(ring.data);
        }

        // Process all available HOP_SIZE chunks
        size_t offset = 0;
        while (offset + HOP_SIZE <= local.size() && running_) {
            for (unsigned int i = 0; i < HOP_SIZE; i++)
                inputBuf->data[i] = local[offset + i];
            offset += HOP_SIZE;

            // RMS volume check — skip quiet frames (background noise)
            float sumSq = 0.0f;
            for (unsigned int i = 0; i < HOP_SIZE; i++)
                sumSq += inputBuf->data[i] * inputBuf->data[i];
            float rms = std::sqrt(sumSq / HOP_SIZE);
            if (rms < MIN_RMS) continue;

            aubio_pitch_do(pitch, inputBuf, outputBuf);
            float freq = fvec_get_sample(outputBuf, 0);
            float conf = aubio_pitch_get_confidence(pitch);

            std::cerr << "[NoteReader] rms=" << rms
                      << " freq=" << freq
                      << " conf=" << conf << "\n";

            if (conf < MIN_CONFIDENCE) continue;

            std::string noteName = frequencyToNote(freq);
            if (noteName.empty()) {
                std::cerr << "[NoteReader] freq " << freq << " out of range\n";
                continue;
            }

            double now = nowMs();

            // Cooldown after accepting a note — let old tone fade
            if (now < cooldownUntilMs_) continue;

            // If note changed, start a new candidate
            if (noteName != candidateNote_) {
                candidateNote_ = noteName;
                candidateStartMs_ = now;
                candidateFired_ = false;
                continue;
            }

            // Same note — check if held long enough
            if (!candidateFired_ && (now - candidateStartMs_) >= SUSTAIN_MS) {
                candidateFired_ = true;
                cooldownUntilMs_ = now + COOLDOWN_MS;
                std::lock_guard<std::mutex> lk(cbMutex_);
                if (onNote_) onNote_({noteName, conf});
            }
        }
        local.clear();
    }

    AudioQueueStop(queue, true);
    AudioQueueDispose(queue, true);
    del_fvec(inputBuf);
    del_fvec(outputBuf);
    del_aubio_pitch(pitch);
}

#elif defined(__linux__)
// ── Linux / RPi: ALSA + aubio ────────────────────────────────────────────────
#include <alsa/asoundlib.h>
#include <vector>

void NoteReader::loop() {
    snd_pcm_t* capture = nullptr;
    int err = snd_pcm_open(&capture, "default", SND_PCM_STREAM_CAPTURE, 0);
    if (err < 0) {
        std::cerr << "[NoteReader] ALSA open failed: " << snd_strerror(err) << "\n";
        return;
    }

    const unsigned int sampleRate = 44100;
    const unsigned int hopSize = 512;
    const unsigned int bufSize = 2048;

    snd_pcm_set_params(capture,
        SND_PCM_FORMAT_FLOAT, SND_PCM_ACCESS_RW_INTERLEAVED,
        1, sampleRate, 1, 50000);

    aubio_pitch_t* pitch = new_aubio_pitch("yin", bufSize, hopSize, sampleRate);
    aubio_pitch_set_unit(pitch, "Hz");
    aubio_pitch_set_tolerance(pitch, 0.7f);

    fvec_t* inputBuf = new_fvec(hopSize);
    fvec_t* outputBuf = new_fvec(1);

    std::vector<float> pcmBuf(hopSize);

    while (running_) {
        snd_pcm_sframes_t frames = snd_pcm_readi(capture, pcmBuf.data(), hopSize);
        if (frames < 0) {
            snd_pcm_recover(capture, (int)frames, 0);
            continue;
        }

        for (unsigned int i = 0; i < hopSize; ++i)
            inputBuf->data[i] = pcmBuf[i];

        // RMS volume check — skip quiet frames (background noise)
        float sumSq = 0.0f;
        for (unsigned int i = 0; i < hopSize; i++)
            sumSq += inputBuf->data[i] * inputBuf->data[i];
        float rms = std::sqrt(sumSq / hopSize);
        if (rms < MIN_RMS) continue;

        aubio_pitch_do(pitch, inputBuf, outputBuf);
        float freq = fvec_get_sample(outputBuf, 0);
        float conf = aubio_pitch_get_confidence(pitch);

        if (conf < MIN_CONFIDENCE) continue;

        std::string noteName = frequencyToNote(freq);
        if (noteName.empty()) continue;

        double now = nowMs();

        // Cooldown after accepting a note — let old tone fade
        if (now < cooldownUntilMs_) continue;

        // If note changed, start a new candidate
        if (noteName != candidateNote_) {
            candidateNote_ = noteName;
            candidateStartMs_ = now;
            candidateFired_ = false;
            continue;
        }

        // Same note — check if held long enough
        if (!candidateFired_ && (now - candidateStartMs_) >= SUSTAIN_MS) {
            candidateFired_ = true;
            cooldownUntilMs_ = now + COOLDOWN_MS;
            std::lock_guard<std::mutex> lk(cbMutex_);
            if (onNote_) onNote_({noteName, conf});
        }
    }

    del_fvec(inputBuf);
    del_fvec(outputBuf);
    del_aubio_pitch(pitch);
    snd_pcm_close(capture);
}

#else
// ── Unsupported platform stub ────────────────────────────────────────────────
void NoteReader::loop() {}
#endif
