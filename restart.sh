#!/bin/bash

# Kill any existing server process
echo "Cleaning up existing processes..."
pkill -f "./server" 2>/dev/null

# Wait a moment to ensure the port is freed
sleep 1

# Check if port is still in use
if lsof -i :5555 > /dev/null; then
    echo "Port 5555 is still in use. Trying to kill the process..."
    kill -9 $(lsof -t -i:5555) 2>/dev/null
    sleep 1
fi

# Rebuild and start the server
echo "Rebuilding server..."
make server

echo "Starting server..."
./server
