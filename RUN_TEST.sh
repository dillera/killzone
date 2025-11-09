#\!/bin/bash

echo "=========================================="
echo "KillZone Phase 2 - Integration Test"
echo "=========================================="
echo ""

# Kill any existing server on port 3000
if lsof -Pi :3000 -sTCP:LISTEN -t >/dev/null 2>&1 ; then
    echo "Killing existing server on port 3000..."
    lsof -Pi :3000 -sTCP:LISTEN -t | xargs kill -9 2>/dev/null
    sleep 1
fi

# Start fresh server
echo "Starting server in background..."
cd /Users/dillera/code/killzone/zoneserver
npm start > /tmp/killzone-server.log 2>&1 &
SERVER_PID=$!
echo "Server PID: $SERVER_PID"
sleep 2

# Check if server started
if lsof -Pi :3000 -sTCP:LISTEN -t >/dev/null 2>&1 ; then
    echo "✅ Server started successfully"
else
    echo "❌ Server failed to start"
    cat /tmp/killzone-server.log
    exit 1
fi

echo ""
echo "Starting Atari client..."
echo "Watch the server logs in another terminal:"
echo "  tail -f /tmp/killzone-server.log"
echo ""

cd /Users/dillera/code/killzone
~/atari800SIO -netsio -win-width 1024 -win-height 768build/killzone.atari

echo ""
echo "Test complete\!"
