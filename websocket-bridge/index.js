import { WebSocketServer } from 'ws';
import net from 'net';

const WS_PORT = 8080;
const TCP_HOST = '127.0.0.1';
const TCP_PORT = 5555;

const wss = new WebSocketServer({ port: WS_PORT });

console.log(`WebSocket bridge server listening on port ${WS_PORT}`);
console.log(`Forwarding connections to TCP server at ${TCP_HOST}:${TCP_PORT}`);

wss.on('connection', (ws) => {
  console.log('New WebSocket client connected');

  let tcpClient = null;
  let isConnected = false;

  // Create TCP connection to C++ server
  const connectToTcpServer = () => {
    tcpClient = new net.Socket();

    tcpClient.connect(TCP_PORT, TCP_HOST, () => {
      console.log(`TCP connection established to ${TCP_HOST}:${TCP_PORT}`);
      isConnected = true;
      ws.send(JSON.stringify({ type: 'connected', message: 'Connected to chat server' }));
    });

    tcpClient.on('data', (data) => {
      // Forward TCP data to WebSocket client
      const message = data.toString();
      if (ws.readyState === ws.OPEN) {
        ws.send(message);
      }
    });

    tcpClient.on('close', () => {
      console.log('TCP connection closed');
      isConnected = false;
      if (ws.readyState === ws.OPEN) {
        ws.send(JSON.stringify({ type: 'disconnected', message: 'Connection to server lost' }));
      }
    });

    tcpClient.on('error', (error) => {
      console.error('TCP connection error:', error.message);
      isConnected = false;
      if (ws.readyState === ws.OPEN) {
        ws.send(JSON.stringify({ type: 'error', message: `Connection error: ${error.message}` }));
      }
    });
  };

  // Connect to TCP server
  connectToTcpServer();

  // Handle messages from WebSocket client
  ws.on('message', (message) => {
    if (!isConnected || !tcpClient) {
      ws.send('Error: Not connected to server\n');
      return;
    }

    try {
      const text = message.toString();
      // Forward message to TCP server
      tcpClient.write(text);
    } catch (error) {
      console.error('Error forwarding message:', error);
      ws.send('Error: Failed to send message\n');
    }
  });

  // Handle WebSocket close
  ws.on('close', () => {
    console.log('WebSocket client disconnected');
    if (tcpClient) {
      tcpClient.destroy();
    }
  });

  // Handle WebSocket errors
  ws.on('error', (error) => {
    console.error('WebSocket error:', error);
    if (tcpClient) {
      tcpClient.destroy();
    }
  });
});

wss.on('error', (error) => {
  console.error('WebSocket server error:', error);
});
