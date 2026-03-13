# RPPi - Arkitekturoverblik

## 1. Systemoversigt

Systemet bestar af 3 uafhaengige programmer (binaries) der kommunikerer over TCP med length-prefix framing:

```
+-------------------+         +-------------------+         +-------------------+
|   HOST RPi        |         |   SERVER RPi      |         |   GUEST RPi       |
|                   |         |                   |         |                   |
|  bin/client       | <-----> |  bin/server        | <-----> |  bin/client       |
|  (Qt6 GUI)        | TCP:9000|  (LobbyServer)    |TCP:9000 |  (Qt6 GUI)        |
|                   |         |                   |         |                   |
|  GyroReader       |         |  bin/game_room    |         |  GyroReader       |
|  NoteReader       | <-----> |  (GameRoom)       | <-----> |  NoteReader       |
|                   | TCP:900x|  (GyroPi/NotePi)  |TCP:900x |                   |
+-------------------+         +-------------------+         +-------------------+
```

- `bin/server` korer permanent pa Server RPi, lytter pa port 9000
- `bin/game_room` startes automatisk (fork+exec) af serveren for hvert nyt spil, pa port 9001, 9002, ...
- `bin/client` korer pa bade Host og Guest RPi -- de er identiske programmer

Al kommunikation er JSON over TCP via Qt Network (QTcpServer/QTcpSocket) med 4-byte length-prefix framing.


## 2. De 3 binaries

### bin/server (server/main.cpp + LobbyServer)
- Starter QCoreApplication + LobbyServer pa port 9000
- Handterer SIGINT (ctrl+c) og SIGCHLD (reaper zombie game_room processer)
- Korer Qt's event loop (`app.exec()`)
- Roller: oprette rum, matche spillere, starte spil

### bin/game_room (server/game_room/main.cpp + GameRoom)
- Startes af serveren via `fork() + execl()` med argumenter: roomCode, port, hostName, guestName, gameType
- Selvstaendig process -- korer uafhaengigt af serveren efter start
- Ejer spillogikken (GyroPiGame eller NotePiGame)
- QTimer (5ms) poller game state og broadcaster til spillere
- Lukker sig selv nar spillet er faerdigt

### bin/client (client/main.cpp + Qt6 GUI)
- Qt6 applikation med QStackedWidget (en skaerm ad gangen)
- Opretter LobbyClient (forbinder til server:9000 via QTcpSocket)
- Efter rolle-tildeling oprettes GameClient (forbinder til game_room:900x)
- Netvaerk korer i Qt's event loop -- ingen separate netvaerkstraade
- Ejer hardware-laesere: GyroReader (BMI160 via SPI) og NoteReader (mikrofon via aubio)


## 3. Filstruktur og ansvar

### common/ (delt mellem server og client)
```
Messages.hpp        -- Alle message-typer (JSON serialisering/deserialisering)
                       DETTE ER DEN VIGTIGSTE FIL AT FORSTAA FOERST
                       Den definerer "kontrakten" mellem server og client
TcpFraming.hpp      -- Length-prefix framing til TCP: frameMessage() + TcpFrameReader
                       Erstatter WebSocket framing med 4-byte BE length + JSON payload
```

### server/
```
main.cpp            -- Entry point: QCoreApplication + LobbyServer, handterer signals
LobbyServer.hpp/cpp -- QObject: lobby via QTcpServer, opretter rum, matcher spillere, forker game_room
GameRoom.hpp/cpp    -- QObject: game session via QTcpServer, modtager input, driver spillogik, broadcaster state
IGame.hpp           -- Interface for spil (update, getState, isFinished, isWon)
GyroPiGame.hpp/cpp  -- GyroPi spillogik: fysik-simulation, vaegge, huller, maal
NotePiGame.hpp/cpp  -- NotePi spillogik: Wordle-algoritme for noter
Leaderboard.hpp/cpp -- Persistent leaderboard i ~/.rppi_leaderboard.json
game_room/main.cpp  -- Entry point for game_room processen
```

### client/
```
main.cpp            -- Entry point: Qt app, opretter alle forbindelser mellem
                       UI signals og netvaerk/hardware. DEN CENTRALE ORCHESTRATOR.
LobbyClient.hpp/cpp -- QObject: TCP client til LobbyServer (rum oprettelse/join)
GameClient.hpp/cpp  -- QObject: TCP client til GameRoom (spil-kommunikation)
GyroReader.hpp/cpp  -- Laeser BMI160 gyroscope via SPI (kun Linux/RPi)
NoteReader.hpp/cpp  -- Laeser mikrofon, pitch detection via aubio (ALSA/CoreAudio)
Credentials.hpp/cpp -- Gemmer/indlaeser brugernavn fra ~/.rppi_credentials
spi_interface.hpp   -- SPI hardware abstraktion til BMI160
```

### client/ui/
```
AppWindow.hpp/cpp       -- QMainWindow med QStackedWidget, koordinerer alle skaerme
UsernameScreen.hpp/cpp  -- Brugernavn-input
MainMenuScreen.hpp/cpp  -- HOST ROOM / JOIN ROOM knapper
LobbyScreen.hpp/cpp     -- Rum-kode, spiller-liste, PLAY knap (host), EXIT knap
GameSelectScreen.hpp/cpp-- Vaelg GyroPi eller NotePi (kun host)
RoleScreen.hpp/cpp      -- Viser rolle (Performer/Communicator) + 3-2-1 countdown
GameScreen.hpp/cpp      -- GyroPi skaerm: labyrint (communicator) / tilt-indikator (performer)
NotePiScreen.hpp/cpp    -- NotePi skaerm: note-grid, Wordle-farver, mikrofon-input
GameOverScreen.hpp/cpp  -- Resultat, leaderboard, play again / exit
```


## 4. Kommunikationsprotokol (Messages.hpp)

Alle beskeder er JSON med et "type" felt. Her er alle 10 message-typer:

### Client -> Server (Lobby)
| Type | Sender | Formaal | Vigtige felter |
|------|--------|---------|----------------|
| `create_room` | Host | Opret nyt rum | `username` |
| `join_room` | Guest | Join eksisterende rum | `code`, `username` |
| `start_game` | Host | Start spillet | `game` ("gyropi"/"notepi") |

### Server -> Client (Lobby)
| Type | Modtager | Formaal | Vigtige felter |
|------|----------|---------|----------------|
| `room_created` | Host | Rum oprettet | `code` (fx "X7K3") |
| `room_joined` | Guest | Bekraeft join | `code` |
| `player_joined` | Begge | Begge spillere er der | `host_name`, `guest_name`, `count` |
| `role_assigned` | Begge | Roller tildelt, spil starter | `role`, `game_port`, `game` |
| `error` | Begge | Fejlbesked | `message` |

### Client -> Server (Game)
| Type | Sender | Formaal | Vigtige felter |
|------|--------|---------|----------------|
| `identify` | Begge | Fortael GameRoom sin rolle | `role` |
| `gyro_data` | Performer (GyroPi) | Gyroscope data | `pitch`, `roll` |
| `note_submit` | Performer (NotePi) | 5-note gaet | `notes` (array) |

### Server -> Client (Game)
| Type | Modtager | Formaal | Vigtige felter |
|------|----------|---------|----------------|
| `game_state` | Begge (GyroPi) | Bold-position + level | `ball_x`, `ball_y`, `pitch`, `roll`, `level` |
| `note_state` | Begge (NotePi) | Wordle-feedback | `notes`, `colors`, `correct`, `history` |
| `game_over` | Begge | Spil faerdigt | `win`, `elapsed_secs`, `levels_reached`, `leaderboard` |


## 5. Roller

Hver spilrunde tildeles rollerne **tilfaeldigt** (50/50 chance):

- **Performer**: Den der udforer handlingen
  - GyroPi: Vipper gyroscopet for at styre bolden
  - NotePi: Spiller noter pa telefon-klaver via mikrofon
  - Ser IKKE labyrinten/det rigtige svar

- **Communicator**: Den der ser skærmen og guider
  - GyroPi: Ser labyrinten med bolden, huller og maal -- skal guide Performer mundtligt
  - NotePi: Ser Wordle-farverne (groen/gul/roed) -- skal fortaelle Performer hvilke noter der er rigtige
  - Kan IKKE interagere med spillet direkte


## 6. Traadmodel

Client-applikationen bruger 1-2 traade:

```
Qt Main Thread (traad 0)
  |-- Korer Qt event loop (app.exec())
  |-- Al UI opdatering sker her
  |-- Netvaerks-I/O (LobbyClient + GameClient) korer OGSAA her
  |     via QTcpSocket signals (readyRead, connected, disconnected)
  |-- GameClient's gyro-timer (QTimer, 25ms) korer her

GyroReader Thread (std::thread, kun performer i GyroPi)
  |-- Laeser BMI160 via SPI hvert 25ms (~40Hz)
  |-- Opdaterer angles_ via mutex

NoteReader Thread (std::thread, kun performer i NotePi)
  |-- CoreAudio/ALSA ring buffer -> aubio pitch detection
  |-- Sender detekterede noter til Qt via QMetaObject::invokeMethod
```

**Vigtig forskel fra tidligere**: Netvaerks-callbacks korer nu direkte i Qt main thread
(via QTcpSocket signals), sa der er IKKE behov for `QMetaObject::invokeMethod`
til netvaerksdata. Kun NoteReader (som korer i sin egen traad) bruger stadig
`invokeMethod` for at poste noter til UI-traaden.


## 7. Server-side traadmodel

```
LobbyServer (bin/server)
  |-- Qt event loop (QCoreApplication::exec())
  |-- QTcpServer lytter pa port 9000
  |-- Modtager/sender via QTcpSocket signals
  |-- Single-threaded — alt sker i event loopet

GameRoom (bin/game_room)
  |-- Qt event loop (QCoreApplication::exec())
  |-- QTcpServer lytter pa port 900x
  |-- QTimer (5ms) checker stateDirty_ og broadcaster
  |-- For NotePi: evaluerer gaet direkte i event loopet

GameRoom game loop thread (kun GyroPi)
  |-- Korer fysik-simulation ved ~40Hz
  |-- Laeser gyro-data via mutex
  |-- Skriver pendingState_ via mutex
  |-- Saetter stateDirty_ flag (atomic)
  |-- QTimer i main thread checker flaget og broadcaster
```

NotePi har IKKE en game loop thread -- det er event-drevet:
Performer sender `note_submit` -> server evaluerer -> sender `note_state` til begge.


## 8. Noegle-designbeslutninger

### Hvorfor fork+exec for game_room?
- Isolering: et crash i game_room pavirker ikke lobbyen
- Skalering: flere spil kan kore samtidigt pa forskellige porte
- Oprydning: OS rydder op naar processen lukker

### Hvorfor Qt Network (QTcpSocket/QTcpServer)?
- Ensartet Qt-baseret arkitektur pa bade server og client
- Non-blocking I/O via Qt's event loop — ingen separate netvaerkstraade nødvendige
- Signals/slots integrerer direkte med Qt's traadmodel
- TCP med length-prefix framing (TcpFraming.hpp) erstatter WebSocket framing

### Hvorfor QStackedWidget?
- Simpel navigation: kun en skaerm vises ad gangen
- Alle skaerme oprettes ved startup og genbruges
- AppWindow koordinerer via signals/slots -- ingen skaerm kender til andre skaerme

### Hvorfor Messages.hpp som delt header?
- En enkelt "kontrakt" mellem server og client
- Compile-time garanti: begge sider parser/builder det samme format
- Nemt at tilfoeje nye message-typer: tilfoej struct, tilfoej handler
