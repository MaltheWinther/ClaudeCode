# Hvordan man saetter sig ind i RPPi-projektet

## Laeseraekkefolge

1. **architecture.md** — Systemoverblik: de 3 binaries, filstruktur, message-protokol, roller
2. **Lobby Flow** (sequence-lobby.png) — Flowet fra opstart til spilstart
3. **Class Overview** (class-overview.png) — Alle klasser og deres relationer
4. **NotePi / GyroPi Game Flow** (sequence-notepi.png / sequence-gyropi.png) — Spil-flowet trin for trin
5. **Client Thread Model** (sequence-client-threads.png) — Hvordan traadene kommunikerer
6. **Dybdegaaende docs** — websocket-pattern.md, qt-gui.md, pitch-detection.md, gyropi-physics.md
7. **Koden** — Nu ved du praecis hvilken fil du skal aabne og hvad den goer

## Tips

- Alle bor laese trin 1-5. Fordel trin 6-7 i gruppen efter interesse.
- Laes Messages.hpp tidligt — den definerer kontrakten mellem server og client.
- Naar du laeser kode: hav det relevante sekvensdiagram ved siden af, saa du kan se hvor i flowet funktionen hoerer til.
