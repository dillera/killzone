#!/bin/bash
#
# run-server.sh - Kill any process on port 3000 and start the zoneserver
#

set -e

PORT=3000
SERVER_DIR="zoneserver"

echo "========================================"
echo "Starting KillZone Server"
echo "========================================"

# Check if something is running on port 3000
echo ""
echo "Checking for processes on port $PORT..."
PID=$(lsof -ti:$PORT || true)

if [ -n "$PID" ]; then
    echo "Found process(es) running on port $PORT: $PID"
    echo "Killing process(es)..."
    kill -9 $PID
    echo "✓ Process(es) killed"
    sleep 1
else
    echo "No process found on port $PORT"
fi

# Navigate to server directory
cd "$SERVER_DIR"

# Install dependencies if needed
if [ ! -d "node_modules" ]; then
    echo ""
    echo "Installing dependencies..."
    npm install
fi

# Start the server
echo ""
echo "========================================"
echo "Starting server on port $PORT..."
echo "========================================"
echo ""

# Use npm start or node src/server.js
npm start
