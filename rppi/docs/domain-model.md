# Domain Model for RPPi (Ready Player Pi)

## Context
The domain model identifies the central concepts in the RPPi system and their relationships. It is derived through noun analysis of the requirements specification (UC-1 through UC-4, M1-M20, S1-S10) and the system sketch.   

## Domain Model (PlantUML)

The PlantUML code below generates the domain model. Render it at plantuml.com, in Draw.io, or via VS Code PlantUML plugin.

```plantuml
@startuml
skinparam classAttributeIconSize 0
skinparam classFontSize 14
skinparam class {
  BackgroundColor #FEFECE
  BorderColor #A80036
}
hide methods

title Domain Model — Ready Player Pi (RPPi)

class Player {
  username
}

class "Host Player" as HP {
}

class "Guest Player" as GP {
}

Player <|-- HP
Player <|-- GP

class "Game Controller" as GC {
  Raspberry Pi
  touchscreen
}

class "ECE SYS HAT" as HAT {
}

class Gyroscope {
  pitch
  roll
}

class Microphone {
}

class Server {
  ipAddress
}

class Lobby {
}

class "Game Room" as Room {
  roomCode
  state
}

class Role <<enumeration>> {
  Communicator
  Performer
}

class Game {
  score
  elapsedTime
}

class GyroPi {
}

class NotePi {
}

Game <|-- GyroPi
Game <|-- NotePi

class Maze {
  level
}

class Ball {
  x, y
  velocity
}

class Wall {
  x, y, w, h
}

class "Black Hole" as BH {
  x, y, radius
}

class Goal {
  x, y, radius
}

class "Tilt Data" as Tilt {
  pitch
  roll
}

class "Note Sequence" as NoteSeq {
  targetNotes [5]
}

class Note {
  pitch (C4–B4)
  position
}

class "Wordle Feedback" as Wordle {
  green / yellow / red
}

class Leaderboard {
  top-50
}

class "Leaderboard Entry" as LBEntry {
  players
  score
  date
  won
}

class Credentials {
  username
}

class GUI {
}

' --- Relationships ---

Player "1" -- "1" GC : uses >
GC "1" *-- "1" HAT : contains >
HAT "1" *-- "1" Gyroscope : contains >
GC "1" *-- "1" Microphone : contains >

Player "1" -- "1" Credentials : has >
Player "1" -- "1" GUI : interacts with >

HP "1" -- "1" Room : creates >
GP "1" -- "1" Room : joins >

Server "1" *-- "1" Lobby : runs >
Lobby "1" *-- "0..*" Room : manages >

Room "1" -- "2" Player : contains >
Room "1" -- "1" Game : starts >

Server "1" -- "2" Role : assigns >
Player "1" -- "1" Role : is assigned >

GyroPi "1" *-- "1" Maze : contains >
Maze "1" *-- "1" Ball : contains >
Maze "1" *-- "1..*" Wall : has >
Maze "1" *-- "0..*" BH : has >
Maze "1" *-- "1" Goal : has >

Gyroscope "1" -- "1" Tilt : produces >
Tilt "1" -- "1" Ball : controls >

NotePi "1" *-- "1" NoteSeq : generates >
NoteSeq "1" *-- "5" Note : consists of >
Microphone "1" -- "1" Note : records >
NotePi "1" -- "1" Wordle : displays >

Server "1" *-- "1" Leaderboard : maintains >
Leaderboard "1" *-- "0..*" LBEntry : contains >
Game "1" -- "1" LBEntry : produces >

GUI "1" -- "1" Maze : shows (Communicator) >
GUI "1" -- "1" Tilt : shows (Performer) >
GUI "1" -- "1" Wordle : shows (Communicator) >
GUI "1" -- "1" Leaderboard : shows >

@enduml
```

## Domain Objects and Descriptions

| Domain Object | Description | Source |
|---|---|---|
| **Player** | A person using the application. Has a username (M1). | UC-1, UC-2 |
| **Host Player** | The player who creates a Game Room and shares the Room Code. | UC-2 |
| **Guest Player** | The player who joins an existing Room via Room Code. | UC-2 |
| **Game Controller** | A Raspberry Pi with attached hardware (touchscreen, ECE HAT, microphone). | System Description |
| **ECE SYS HAT** | Hardware HAT for RPi containing the BMI160 gyroscope. | System Description |
| **Gyroscope** | Sensor measuring pitch and roll via tilting the RPi (NF10: <50ms). | UC-3, M11 |
| **Microphone** | Sensor recording audio for note classification in NotePi. | UC-4, M14 |
| **Server** | Central Raspberry Pi managing lobby, game rooms, data routing, and leaderboard. | System Description, M19 |
| **Lobby** | The server's waiting area where rooms are created and joins are handled. | UC-2 |
| **Game Room** | A session with 2 players, identified by a Room Code. | UC-2, M4-M6 |
| **Role** | Communicator (sees screen, guides verbally) or Performer (physical input, no screen). Randomly assigned. | M8, M9 |
| **Game** | An active game session with score and elapsed time. | UC-3, UC-4 |
| **GyroPi** | Concrete game: steer a ball through a maze by tilting the RPi. | UC-3 |
| **NotePi** | Concrete game: find the correct 5-note sequence with Wordle-like feedback. | UC-4 |
| **Maze** | Course in GyroPi with walls, black holes, and a goal. Has levels. | M10 |
| **Ball** | The ball controlled by the Performer's tilt data in GyroPi. | M12 |
| **Wall** | Obstacle in the maze that the ball collides with. | M10 |
| **Black Hole** | Trap in the maze — ball falls in → reset to start (M13). | UC-3 Ext.1 |
| **Goal** | Target zone — ball reaches here → game is won. | UC-3 |
| **Tilt Data** | Pitch/roll data from gyroscope sent from Performer to Server. | M11, M12 |
| **Note Sequence** | The randomly generated 5-note sequence the Performer must find. | M15 |
| **Note** | A single classified tone (C4–B4 must, A0–C8 should). | M14, M18, S10 |
| **Wordle Feedback** | Color-coded feedback per note: green (correct note+position), yellow (correct note, wrong position), red (not in sequence). | M16 |
| **Leaderboard** | Global top-50 ranking per game, sorted and saved as JSON. | S8, S9, NF15 |
| **Leaderboard Entry** | Single result: player names, score, date, won/lost. | M20 |
| **Credentials** | Locally stored username file on the player's RPi. | M1, M2 |
| **GUI** | Graphical user interface on the player's RPi (Qt6). Shows the role-specific perspective. | System Description |

## Key Relationships

- **Player** uses a **Game Controller** (RPi + hardware)
- **Host Player** creates a **Game Room**, **Guest Player** joins via Room Code
- **Server** runs a **Lobby** that manages 0..* **Game Rooms**
- A **Game Room** contains exactly 2 **Players** and starts 1 **Game**
- **Server** randomly assigns **Roles** to the 2 players
- **GyroPi** contains a **Maze** with **Ball**, **Walls**, **Black Holes**, and **Goal**
- **Gyroscope** produces **Tilt Data** which controls **Ball**
- **NotePi** generates a **Note Sequence** of 5 **Notes**; **Microphone** records Notes
- **Game** produces a **Leaderboard Entry** stored in **Leaderboard**
- **GUI** shows role-specific view: Communicator sees Maze/Wordle, Performer sees Tilt/recording

## Verification
- Render the PlantUML code at plantuml.com or in Draw.io and verify all boxes and relationships are visible
- Compare with the SON domain model style (Figure 6, page 16): box notation, named relationships, hierarchical layout
- Check that all nouns from UC-1 through UC-4 and M1-M20 are represented
