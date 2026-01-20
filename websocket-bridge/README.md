# WebSocket Bridge Server

A bridge server that connects WebSocket clients to the TCP-based C++ chat server.

## Purpose

Browsers cannot directly connect to TCP sockets. This bridge server:
1. Accepts WebSocket connections from the frontend
2. Connects to the C++ TCP server on port 5555
3. Forwards messages bidirectionally between WebSocket and TCP

## Setup

```bash
cd websocket-bridge
pnpm install
```

## Running

```bash
pnpm start
```

The bridge server will:
- Listen on WebSocket port 8080
- Connect to TCP server at 127.0.0.1:5555

## Development

For auto-reload during development:

```bash
pnpm dev
```

## Configuration

Edit `index.js` to change:
- `WS_PORT`: WebSocket server port (default: 8080)
- `TCP_HOST`: TCP server host (default: 127.0.0.1)
- `TCP_PORT`: TCP server port (default: 5555)

## Notes

- Make sure the C++ chat server is running before starting the bridge
- The bridge handles reconnection automatically
- Each WebSocket client gets its own TCP connection
