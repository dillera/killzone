#!/bin/bash
# KillZone Server Systemd Deployment Script
# This script creates or updates a systemd service for the KillZone game server

# Configuration - Set this to your KillZone server directory
KILLZONE_DIR="/home/$(whoami)/killzone/zoneserver"

# Derived paths
SERVICE_NAME="killzone-server"
SERVICE_FILE="/etc/systemd/system/${SERVICE_NAME}.service"
USER=$(whoami)
NODE_PATH=$(which node)

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "KillZone Server Systemd Deployment"
echo "===================================="
echo ""

# Check if running as root
if [ "$EUID" -ne 0 ]; then 
    echo -e "${RED}Error: This script must be run as root (use sudo)${NC}"
    exit 1
fi

# Verify the server directory exists
if [ ! -d "$KILLZONE_DIR" ]; then
    echo -e "${RED}Error: Server directory not found: $KILLZONE_DIR${NC}"
    echo "Please edit this script and set KILLZONE_DIR to the correct path"
    exit 1
fi

# Verify package.json exists
if [ ! -f "$KILLZONE_DIR/package.json" ]; then
    echo -e "${RED}Error: package.json not found in $KILLZONE_DIR${NC}"
    exit 1
fi

# Verify node is installed
if [ -z "$NODE_PATH" ]; then
    echo -e "${RED}Error: Node.js not found. Please install Node.js first.${NC}"
    exit 1
fi

echo "Configuration:"
echo "  Server Directory: $KILLZONE_DIR"
echo "  Service Name: $SERVICE_NAME"
echo "  User: $USER"
echo "  Node Path: $NODE_PATH"
echo ""

# Check if service already exists
SERVICE_EXISTS=0
if systemctl list-unit-files | grep -q "^${SERVICE_NAME}.service"; then
    SERVICE_EXISTS=1
    echo -e "${YELLOW}Service already exists. Will update and restart.${NC}"
    
    # Stop the existing service
    echo "Stopping existing service..."
    systemctl stop ${SERVICE_NAME}
    
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}✓ Service stopped${NC}"
    else
        echo -e "${YELLOW}⚠ Service was not running${NC}"
    fi
else
    echo -e "${GREEN}Creating new service...${NC}"
fi

# Create the systemd service file
echo "Writing service file to $SERVICE_FILE..."

cat > "$SERVICE_FILE" << EOF
[Unit]
Description=KillZone Multiplayer Game Server
After=network.target
Documentation=https://github.com/dillera/killzone

[Service]
Type=simple
User=$USER
WorkingDirectory=$KILLZONE_DIR
ExecStart=$NODE_PATH src/server.js
Restart=always
RestartSec=10
StandardOutput=journal
StandardError=journal
SyslogIdentifier=killzone-server

# Environment
Environment=NODE_ENV=production

# Security settings
NoNewPrivileges=true
PrivateTmp=true

[Install]
WantedBy=multi-user.target
EOF

if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ Service file created${NC}"
else
    echo -e "${RED}✗ Failed to create service file${NC}"
    exit 1
fi

# Set proper permissions
chmod 644 "$SERVICE_FILE"

# Reload systemd daemon
echo "Reloading systemd daemon..."
systemctl daemon-reload

if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ Systemd daemon reloaded${NC}"
else
    echo -e "${RED}✗ Failed to reload systemd daemon${NC}"
    exit 1
fi

# Enable the service to start on boot
echo "Enabling service to start on boot..."
systemctl enable ${SERVICE_NAME}

if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ Service enabled${NC}"
else
    echo -e "${RED}✗ Failed to enable service${NC}"
    exit 1
fi

# Start the service
echo "Starting service..."
systemctl start ${SERVICE_NAME}

if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ Service started${NC}"
else
    echo -e "${RED}✗ Failed to start service${NC}"
    echo "Check logs with: journalctl -u ${SERVICE_NAME} -f"
    exit 1
fi

# Wait a moment for the service to start
sleep 2

# Check service status
echo ""
echo "Service Status:"
echo "==============="
systemctl status ${SERVICE_NAME} --no-pager -l

echo ""
echo -e "${GREEN}Deployment complete!${NC}"
echo ""
echo "Useful commands:"
echo "  View logs:        journalctl -u ${SERVICE_NAME} -f"
echo "  Stop service:     sudo systemctl stop ${SERVICE_NAME}"
echo "  Start service:    sudo systemctl start ${SERVICE_NAME}"
echo "  Restart service:  sudo systemctl restart ${SERVICE_NAME}"
echo "  Service status:   sudo systemctl status ${SERVICE_NAME}"
echo "  Disable service:  sudo systemctl disable ${SERVICE_NAME}"
echo ""
