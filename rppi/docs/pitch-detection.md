# NoteReader: Pitch Detection Pipeline

## Overblik

NoteReader lytter pa mikrofonen og detekterer hvilken musikalsk note der spilles.
Den bruger `aubio`-biblioteket med YIN-algoritmen til pitch detection.

## Pipeline (for hvert audio-frame a 512 samples)

```
Mikrofon (44100 Hz, mono, float32)
    |
    v
[1] RMS Volume Check
    Beregn gennemsnitslydstyrke: RMS = sqrt(sum(sample^2) / N)
    Hvis RMS < 0.008 -> SKIP (stilhed/baggrundsstoj)
    |
    v
[2] Aubio Pitch Detection (YIN-algoritmen)
    Input: 512 samples
    Output: frekvens (Hz) + confidence (0.0 - 1.0)
    Hvis confidence < 0.6 -> SKIP (usikker detektion)
    |
    v
[3] Frekvens -> Notenavn
    MIDI = 12 * log2(freq / 440) + 69
    Note = ["C","C#","D",...,"B"][MIDI % 12]
    Kun range 131-2000 Hz (C3-B6, telefonhojttaler-range)
    Ingen oktav -- kun notenavn (fx "A", ikke "A4")
    |
    v
[4] Sustained Note Detection
    Noten skal vaere den SAMME i mindst 350ms for den accepteres.
    Dette forhindrer at korte stoj-spikes registreres som noter.

    Logik:
    - Ny note != candidateNote_ -> start ny candidate, reset timer
    - Samme note i 350ms -> ACCEPTER, fire callback
    |
    v
[5] Cooldown (500ms)
    Efter en note accepteres, ignoreres AL input i 500ms.
    Dette forhindrer:
    - Sustain/reverb fra forrige tone registreres igen
    - Samme tone detekteres to gange
    |
    v
[6] Callback -> Qt Main Thread
    NoteEvent {note: "A", confidence: 0.85}
    -> QMetaObject::invokeMethod -> NotePiScreen viser noten
```

## Platform-forskelle

### macOS (CoreAudio)
- AudioQueue API til mikrofon-input
- Audio callback skriver til ring buffer (mutex-beskyttet)
- NoteReader-traad laeser fra ring buffer via condition variable
- Tre audio-buffers roterer (triple buffering)

### Linux/RPi (ALSA)
- `snd_pcm_readi()` blokerer indtil samples er klar
- Simplere model: laeser direkte, ingen ring buffer
- USB-mikrofon konfigureres som "default" ALSA device

## Parametre

| Parameter | Vaerdi | Formaal |
|-----------|--------|---------|
| SAMPLE_RATE | 44100 Hz | Standard audio sample rate |
| HOP_SIZE | 512 samples | ~11.6ms per frame |
| BUF_SIZE | 2048 samples | Intern aubio buffer (bedre freq resolution) |
| MIN_RMS | 0.008 | Minimum lydstyrke for at processere |
| MIN_CONFIDENCE | 0.6 | Minimum aubio confidence |
| SUSTAIN_MS | 350ms | Tone skal holdes sa laenge |
| COOLDOWN_MS | 500ms | Pause efter accepteret tone |
| Freq range | 131-2000 Hz | C3 til B6 (telefon-speaker range) |

## YIN-algoritmen (kort)

YIN er en pitch-detection algoritme designet til monophonisk lyd (en tone ad gangen).
Den finder periodicitet i signalet ved at beregne difference function og finde minimum.

Fordele: hurtig, praecis for rene toner
Ulemper: kan forvirres af harmonisk-rige signaler (fx telefonhojttaler)

aubio's implementation: `new_aubio_pitch("yin", bufSize, hopSize, sampleRate)`
