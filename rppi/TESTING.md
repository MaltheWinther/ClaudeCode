# RPPi — Testing Guide

Testing GyroPi with 3 Raspberry Pis: one server, two players.

---

## Hardware needed
- 3 Raspberry Pis with ECE SYS HATs (gyroscope)
- All connected to the same Wi-Fi network
- A screen for each player RPi (the 7-inch touch display works)

---

## Step 1 — Install dependencies (first time only, on each RPi)

**On the server RPi:**
```bash
sudo apt update
sudo apt install libwebsockets-dev nlohmann-json3-dev
```

**On each player RPi:**
```bash
sudo apt update
sudo apt install libwebsockets-dev nlohmann-json3-dev qt6-base-dev
```

The `ecesyshat` library (BMI160 gyroscope driver) should already be pre-installed on the RPis from the course setup.

---

## Step 2 — Get the code (first time only, on each RPi)

```bash
cd ~
git clone https://github.com/MaltheWinther/ClaudeCode.git
cd ClaudeCode/rppi
```

If the repo is already there, just pull the latest:

```bash
cd ~/ClaudeCode && git pull
cd rppi
```

---

## Step 3 — Build

**On the server RPi** — build server and game_room:
```bash
cd ~/ClaudeCode/rppi
make server game_room
```

**On each player RPi** — build client:
```bash
cd ~/ClaudeCode/rppi
make client
```

---

## Step 4 — Find the server RPi's IP address

On the server RPi:
```bash
hostname -I
```

Note the IP address (e.g. `192.168.1.42`). The players will need this.

---

## Step 5 — Start the server

On the server RPi:
```bash
cd ~/ClaudeCode/rppi
./bin/server
```

You should see:
```
[Lobby] Listening on port 9000
```

Leave this terminal open.

---

## Step 6 — Start the clients

On **each player RPi**, open a terminal and run:
```bash
cd ~/ClaudeCode/rppi
./bin/client <server_ip>
```

Replace `<server_ip>` with the IP from Step 4. Example:
```bash
./bin/client 192.168.1.42
```

---

## Step 7 — Play

1. **First launch**: each player enters a username (letters and numbers, max 12 characters). It is saved for future sessions.

2. **Host player**: tap **HOST ROOM** — a 4-letter room code appears on screen.

3. **Guest player**: tap **JOIN ROOM**, type in the room code, tap **JOIN**.

4. **Host player**: once the lobby shows both player names, tap **PLAY**.

5. Both screens show a 3-2-1 countdown and the assigned role:
   - **Communicator** — sees the maze with the ball, black holes, and goal
   - **Performer** — sees a tilt indicator; physically tilts the RPi to move the ball

6. The Communicator guides the Performer verbally. The Performer cannot see the maze.

7. **Black hole**: if the ball falls in, the Communicator sees a red flash message and the ball resets to start.

8. **Win**: reach the gold star goal on both levels → game over screen with time, levels reached, and top-5 leaderboard.

---

## Troubleshooting

| Problem | Fix |
|---|---|
| `./bin/server` not found | Run `make server game_room` first |
| Client can't connect | Double-check the server IP; make sure all RPis are on the same network |
| Port 9000 already in use | `pkill -f bin/server` then start again |
| Gyroscope not working | Make sure the ECE SYS HAT is properly seated; reboot if needed |
| Screen is blank / no display | Check `DISPLAY` env var; try `export DISPLAY=:0` before running the client |

---

## Leaderboard

Scores are saved permanently on the server RPi at:
```
~/.rppi_leaderboard.json
```

You can view it anytime:
```bash
cat ~/.rppi_leaderboard.json
```

---

## Two clients on one machine (quick Mac test, no RPi needed)

If you want to test the lobby flow without hardware, you can run two clients on a Mac using the username override argument to avoid the shared credentials file:

```bash
./bin/client <server_ip> Alice   # terminal 1
./bin/client <server_ip> Bob     # terminal 2
```

Note: the gyroscope won't work on Mac — the ball won't move. This is only useful for testing the lobby, role assignment, and UI screens.
