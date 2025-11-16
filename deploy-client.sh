#!/bin/bash

# KillZone Client Build & Deploy Script
# Builds the Atari client binary and deploys to remote server

set -e

echo "=========================================="
echo "KillZone Client Build & Deploy"
echo "=========================================="
echo ""

# Build the client
echo "🔨 Building client..."
make clean && make disk

if [ ! -f dist/killzone.atr ]; then
    echo "❌ Build failed - killzone.atr not found"
    exit 1
fi

echo "✅ Build successful"
echo ""

# Deploy to remote server
DEPLOY_HOST="${DEPLOY_HOST:-actual.diller.org}"
DEPLOY_PATH="${DEPLOY_PATH:-_services/tnfs/server_root/ATARI/TESTING}"

echo "📤 Deploying to $DEPLOY_HOST:$DEPLOY_PATH"
scp dist/killzone.atr "$DEPLOY_HOST:$DEPLOY_PATH/"

if [ $? -eq 0 ]; then
    echo "✅ Deployment successful!"
    echo ""
    echo "Client available at:"
    echo "  $DEPLOY_HOST:$DEPLOY_PATH/killzone.atr"
else
    echo "❌ Deployment failed"
    exit 1
fi
