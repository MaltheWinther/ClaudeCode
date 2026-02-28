#!/usr/bin/env python3
"""
ws_bridge.py

Reads JSON tilt data from stdin (piped from gyro_stream) and broadcasts
it to all connected WebSocket clients.

Also serves game.html via a simple HTTP server on port 8080.

Usage:
    ./gyro_stream | python3 ws_bridge.py

Then open:  http://<rpi-ip>:8080/game.html
"""

import asyncio
import sys
import json
import threading
import http.server
import os

try:
    import websockets
except ImportError:
    print("Install websockets:  pip3 install websockets", file=sys.stderr)
    sys.exit(1)

# ── Shared state ──────────────────────────────────────────────────────────────

connected: set = set()

# ── WebSocket handler ─────────────────────────────────────────────────────────

async def ws_handler(websocket):
    connected.add(websocket)
    try:
        await websocket.wait_closed()
    finally:
        connected.discard(websocket)

# ── Stdin → broadcast loop ────────────────────────────────────────────────────

async def stdin_broadcast():
    loop = asyncio.get_event_loop()
    reader = asyncio.StreamReader()
    proto  = asyncio.StreamReaderProtocol(reader)
    await loop.connect_read_pipe(lambda: proto, sys.stdin.buffer)

    async for raw in reader:
        try:
            data = json.loads(raw.decode())
        except (json.JSONDecodeError, UnicodeDecodeError):
            continue  # skip non-JSON lines (e.g. BMI160 init messages)

        msg  = json.dumps(data)
        dead = set()
        for ws in connected.copy():
            try:
                await ws.send(msg)
            except Exception:
                dead.add(ws)
        connected.difference_update(dead)

# ── HTTP server (serves game.html) ────────────────────────────────────────────

def serve_http():
    os.chdir(os.path.dirname(os.path.abspath(__file__)))
    handler = http.server.SimpleHTTPRequestHandler
    handler.log_message = lambda *a: None  # suppress request logs
    with http.server.HTTPServer(("", 8080), handler) as httpd:
        print("HTTP  → http://0.0.0.0:8080/game.html", flush=True)
        httpd.serve_forever()

# ── Main ──────────────────────────────────────────────────────────────────────

async def main():
    threading.Thread(target=serve_http, daemon=True).start()
    async with websockets.serve(ws_handler, "0.0.0.0", 8765):
        print("WS    → ws://0.0.0.0:8765", flush=True)
        print("Pipe  → reading gyro JSON from stdin...", flush=True)
        await stdin_broadcast()

asyncio.run(main())
