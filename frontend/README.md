# Chat Terminal Frontend

A Bloomberg Terminal-style web frontend for the C++ chat server.

## Features
- **Real-time Chat**: WebSocket connection to chat server
- **User Authentication**: Login and registration
- **Active Users List**: View all connected users
- **Server Statistics**: Monitor server metrics
- **Private Messaging**: Send direct messages to users
- **Command Support**: All server commands supported

## Setup

### Prerequisites

- Node.js 18+ and pnpm installed
- C++ chat server running on port 5555
- WebSocket bridge server running (see websocket-bridge directory)

### Installation

```bash
cd frontend
pnpm install
```

### Development

```bash
# Start the WebSocket bridge server first (in one terminal)
cd ../websocket-bridge
pnpm install
pnpm start

# Then start the frontend (in another terminal)
cd frontend
pnpm dev
```

The frontend will be available at `http://localhost:3000`

### Production Build

```bash
pnpm build
pnpm start
```

## Architecture

- **Next.js 14**: React framework with App Router
- **TypeScript**: Type-safe development
- **Tailwind CSS**: Utility-first styling
- **WebSocket Client**: Custom WebSocket client for server communication
- **Bloomberg Terminal Theme**: Custom color scheme and styling

## Components

- `ChatTerminal`: Main application component
- `MessageList`: Displays chat messages
- `InputArea`: Message input with command support
- `Sidebar`: Shows active users and server stats
- `AuthModal`: Login/registration modal

## Commands

- `/stats` - Show server statistics
- `/list` - List active users
- `/msg <username> <message>` - Send private message
- `/register <username> <password>` - Register new user
- `/login <username> <password>` - Login

## Styling

The application uses a custom Bloomberg Terminal color scheme:
- Dark backgrounds (#0d1117, #161b22, #21262d)
- Accent colors (blue #58a6ff)
- Success/Error indicators (green/red)
- Monospace fonts for terminal aesthetic
****
