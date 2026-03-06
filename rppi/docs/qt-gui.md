# Qt6 GUI: Arkitektur og Moenstre

## Overblik

Clientens GUI er bygget med Qt6 Widgets. Ingen QML, ingen HTML -- ren C++ med Qt.

## QStackedWidget-moensteret

AppWindow indeholder en `QStackedWidget` -- en container der viser
praecis EN child widget ad gangen (som en stak kort):

```
AppWindow (QMainWindow)
  |
  +-- QStackedWidget (central widget)
       |-- [0] UsernameScreen    "Indtast brugernavn"
       |-- [1] MainMenuScreen    "HOST ROOM / JOIN ROOM"
       |-- [2] LobbyScreen       "Rum X7K3, venter pa spiller..."
       |-- [3] GameSelectScreen  "Vaelg GyroPi eller NotePi"
       |-- [4] RoleScreen        "Du er PERFORMER! 3... 2... 1..."
       |-- [5] GameScreen        "GyroPi labyrint / tilt-indikator"
       |-- [6] NotePiScreen      "NotePi note-grid / Wordle-farver"
       |-- [7] GameOverScreen    "YOU WIN! / GAME OVER"
```

Skift skaerm: `stack_->setCurrentWidget(gameScreen_);`

**Alle skaerme oprettes EN gang** i AppWindow's constructor og genbruges.

## Signals og Slots

Qt's signal/slot system er event-drevet kommunikation:

```
[Knap klikket] --signal--> [Handler koerer]
```

### Hvordan det virker i RPPi

1. **Screen udsender signal** naar bruger interagerer:
   ```cpp
   // I MainMenuScreen, naar "HOST ROOM" trykkes:
   emit hostClicked();
   ```

2. **AppWindow forbinder** signal til sit eget signal:
   ```cpp
   connect(mainMenuScreen_, &MainMenuScreen::hostClicked,
           this, &AppWindow::hostRoomClicked);
   ```

3. **main.cpp forbinder** AppWindow's signal til en lambda:
   ```cpp
   QObject::connect(&window, &AppWindow::hostRoomClicked, [&]() {
       lobby = new LobbyClient(serverIp, 9000);
       lobby->createRoom(username);
       netThread = std::thread([lp = lobby] { lp->run(); });
   });
   ```

**Kaedefoelge**: Screen -> AppWindow -> main.cpp lambda
Hver skaerm kender KUN til sine egne signals -- den ved ikke hvad der sker naar signalet fires.

## Traad-sikkerhed: QMetaObject::invokeMethod

**Problem**: Netvaerkstraaden modtager data, men UI maa kun opdateres fra Qt main thread.

**Loesning**: `QMetaObject::invokeMethod` med `Qt::QueuedConnection`:

```cpp
// I netvaerkstraaden (GameClient callback):
game->setOnGameState([&window](MsgGameState gs) {
    QMetaObject::invokeMethod(&window,
        [&window, gs]() {
            window.updateGameState(gs.ballX, gs.ballY, ...);
        }, Qt::QueuedConnection);
});
```

Hvad sker der:
1. Netvaerkstraaden opretter en lambda med en KOPI af data
2. `invokeMethod` laegger lambdaen i Qt's event queue
3. Naar Qt main thread naar til den, koerer lambdaen
4. UI opdateres sikkert fra den rigtige traad

**Qt::QueuedConnection** = "koer senere i modtagerens traad"
**Qt::DirectConnection** = "koer nu i denne traad" (farligt for UI!)

## Skaerm-oversigt

### UsernameScreen
- QLineEdit til brugernavn
- Validation: 1-12 alfanumeriske tegn
- Signal: `submitted(QString username)`

### MainMenuScreen
- Viser "Hej, [username]!"
- To knapper: HOST ROOM, JOIN ROOM (med kode-input)
- Signals: `hostClicked()`, `joinClicked(QString code)`

### LobbyScreen
- Viser rum-kode (stort, centreret)
- Viser spillernavne naar begge er der
- PLAY knap (kun synlig for host, aktiv naar 2 spillere)
- EXIT knap (roed)
- Signals: `startClicked()`, `exitLobby()`

### GameSelectScreen
- To store knapper: GAME 1 - GyroPi, GAME 2 - NotePi
- Kun host kan vaelge (guest ser disabled knapper)
- Signal: `gameSelected(QString gameType)`

### RoleScreen
- Stor tekst: "You are PERFORMER!" eller "You are COMMUNICATOR!"
- Forklaring af hvad rollen indebaerer
- Automatisk countdown: 3... 2... 1... (QTimer, 1s intervaller)
- Signal: `done()` (naar countdown er faerdig)

### GameScreen (GyroPi)
- **Communicator**: QPainter tegner labyrint med vaegge (graa),
  huller (sort), maal (groen), bold (roed). Live-opdatering ved 40Hz.
- **Performer**: Viser tilt-retning som pil + vinkelvaerdier
- EXIT knap (oevre venstre)
- Signal: `exitGame()`

### NotePiScreen (NotePi)
- 5 celler til noter (klik for at slette en enkelt)
- **Performer**: Graa celler (ingen farve-feedback)
- **Communicator**: Groen/gul/roed Wordle-feedback
- SUBMIT knap + CLEAR knap
- Historik af tidligere gaet
- Mikrofon-detekterede noter vises automatisk
- EXIT knap
- Signal: `notesSubmitted(vector<string>)`, `exitGame()`

### GameOverScreen
- "YOU WIN!" (groen) eller "GAME OVER" (roed)
- NotePi: "Time: Xs | Attempts: Y"
- GyroPi: "Time: Xs | Level Y/2"
- Leaderboard top 10
- "GO BACK TO MAIN MENU" knap, "EXIT PROGRAM" knap
- Signals: `playAgain()`, `exitGame()`

## MOC (Meta-Object Compiler)

Enhver klasse med `Q_OBJECT` makroen skal processeres af Qt's MOC.
MOC genererer ekstra C++ kode der muliggoer signals/slots.

I Makefile:
```makefile
client/ui/%.moc.cpp: client/ui/%.hpp
	$(MOC) $(QT_CFLAGS) $< -o $@
```

Disse `.moc.cpp` filer kompileres og linkes med resten.
