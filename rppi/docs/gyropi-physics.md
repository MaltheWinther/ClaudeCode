# GyroPi: Fysik-simulation og Hardware

## Overblik

GyroPi er et labyrint-spil hvor en bold styres ved at vippe en fysisk gyroscope (BMI160).
Performer vipper RPi'en, Communicator ser labyrinten og guider mundtligt.

## Hardware: BMI160 Gyroscope

### Hvad er BMI160?
En 6-akset IMU (Inertial Measurement Unit) der maler:
- **Accelerometer**: tyngdekraftens retning (statisk tilt)
- **Gyroskop**: rotationshastighed (dynamisk bevaegelse)

### Kommunikation: SPI
BMI160 sidder pa ECE SYS HAT og kommunikerer via SPI (Serial Peripheral Interface):
- 4 ledninger: MOSI, MISO, SCLK, CS
- RPi er master, BMI160 er slave
- `spi_interface.hpp` wrapper hvert SPI read/write
- `bmi160_wrapper` (eksternt bibliotek) laeser raa sensor-data

### Complementary Filter
Raa sensor-data er stojende. Vi kombinerer accelerometer og gyroskop:

```
pitch = 0.98 * (pitch + gyro_y * dt) + 0.02 * accel_pitch
roll  = 0.98 * (roll  + gyro_x * dt) + 0.02 * accel_roll
```

- **98% gyroskop**: hurtig, praecis pa kort sigt, men drifter over tid
- **2% accelerometer**: langsom, stojende, men drifter IKKE
- Resultatet: stabil tilt-vinkel i grader uden drift

GyroReader laeser ved 40Hz og eksponerer `read()` med mutex.

## Fysik-simulation (GyroPiGame)

### Game Loop (40Hz)
Koerer i separat traad. Hvert 25ms:

```
1. Acceleration fra tilt:
   vx += sin(roll  * pi/180) * G * dt    (G = 1350)
   vy += sin(pitch * pi/180) * G * dt

2. Friktion (rullefriktion):
   vx *= e^(-2.0 * dt)
   vy *= e^(-2.0 * dt)

3. Flyt bold:
   bx += vx * dt
   by += vy * dt

4. Vaeg-kollision (AABB vs cirkel):
   - Find naermeste punkt pa rektangel til boldcenter
   - Hvis afstand < boldradius: skub bold ud + reflekter hastighed
   - Energitab ved bounce: 25% (WALL_BOUNCE = 0.25)

5. Sort hul check:
   - Hvis afstand(bold, hul) < hul.radius + bold.radius
   - Reset: bold til start, hastighed = 0

6. Maal check:
   - Hvis afstand(bold, maal) < maal.radius
   - Naeste level eller WIN
```

### Level-design
Hvert level er defineret som:
- `walls[]`: rektangler (vaegge + kanter)
- `holes[]`: cirkler (sorte huller -- faelderne)
- `goal`: cirkel (maalomraade)
- `startX, startY`: bold-startposition

Level 1: 3 korridorer, maal nederst til hojre, 3 huller
Level 2: 4 korridorer, maal nederst til venstre, 4 huller, mindre maal

### Dataflow: Gyro -> Server -> Skaerm

```
BMI160 (SPI) -> GyroReader (40Hz traad) -> GameClient (hvert 25ms)
    -> WebSocket -> GameRoom -> GyroPiGame.update()
    -> GameState {ballX, ballY, level}
    -> WebSocket -> begge clients
    -> GameScreen tegner labyrint (communicator) / tilt-pil (performer)
```

## Konstanter

| Konstant | Vaerdi | Betydning |
|----------|--------|-----------|
| BALL_R | 14 px | Boldens radius |
| G | 1350 | Tyngdekraft-styrke (acceleration) |
| ROLL_FRICTION | 2.0 | Friktionskoefficient |
| WALL_BOUNCE | 0.25 | Energi bevaret ved vaeg-bounce |
| MAX_DT | 0.05s | Cap pa delta-time (undgar lag-spikes) |
| ALPHA | 0.98 | Complementary filter vaegt |
| GYRO_SCALE | 2000/32767 | LSB til grader/sekund |
