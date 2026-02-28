#!/bin/bash
# Compile and start the GyroPi game.
# Open http://<rpi-ip>:8080/game.html in a browser on your Mac.
set -e
cd "$(dirname "$0")"
make
echo "Starting GyroPi..."
./gyro_stream | python3 ws_bridge.py
