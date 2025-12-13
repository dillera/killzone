const net = require('net');
const Player = require('./player');
const CombatResolver = require('./combat');

/**
 * TCP Server for KillZone
 * Handles binary connections for low-latency gameplay
 */
class TcpServer {
    constructor(world, port) {
        this.world = world;
        this.port = port;
        this.server = net.createServer(this.handleConnection.bind(this));
        this.clients = new Set();
    }

    start() {
        this.server.listen(this.port, () => {
            console.log(`KillZone TCP Server running on port ${this.port}`);
        });
    }

    handleConnection(socket) {
        console.log(`TCP Client connected: ${socket.remoteAddress}`);
        this.clients.add(socket);

        socket.player = null; // Associated player object

        socket.on('data', (data) => this.handleData(socket, data));
        socket.on('close', () => this.handleClose(socket));
        socket.on('error', (err) => console.error(`Socket error: ${err.message}`));
    }

    handleData(socket, data) {
        const packetType = data[0];

        try {
            switch (packetType) {
                case 0x01: // Join
                    this.handleJoin(socket, data.slice(1));
                    break;
                case 0x02: // Move
                    this.handleMove(socket, data.slice(1));
                    break;
                case 0x03: // Get State
                    this.handleGetState(socket);
                    break;
                default:
                    console.log(`Unknown packet type: ${packetType}`);
            }
        } catch (e) {
            console.error(`Error handling TCP data: ${e.message}`);
        }
    }

    handleJoin(socket, data) {
        if (data.length < 1) return;
        const nameLen = data[0];
        if (data.length < 1 + nameLen) return;

        const name = data.slice(1, 1 + nameLen).toString();
        console.log(`TCP Join Request: ${name}`);

        // Check if previously disconnected
        const disconnectedPlayer = this.world.getDisconnectedPlayer(name);
        let player;

        if (disconnectedPlayer) {
            player = disconnectedPlayer;
            player.status = 'alive';
            player.health = 100;

            // New spawn pos
            let x, y;
            let attempts = 0;
            do {
                x = Math.floor(Math.random() * this.world.width);
                y = Math.floor(Math.random() * this.world.height);
                attempts++;
            } while (this.world.getPlayerAtPosition(x, y) !== null && attempts < 10);

            player.setPosition(x, y);
            this.world.removeDisconnectedPlayer(name);
            this.world.addPlayer(player);
            this.world.setRejoinMessage(name);
            console.log(`  🔄 TCP Rejoin: ${name}`);
        } else {
            const playerId = `player_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`;
            let x, y;
            let attempts = 0;
            do {
                x = Math.floor(Math.random() * this.world.width);
                y = Math.floor(Math.random() * this.world.height);
                attempts++;
            } while (this.world.getPlayerAtPosition(x, y) !== null && attempts < 10);

            player = new Player(playerId, name, x, y);
            this.world.addPlayer(player);
            this.world.setJoinMessage(name);
            console.log(`  👤 TCP Join: ${name}`);
        }

        socket.player = player;

        // Response: 0x01 [ID_LEN] [ID] [X] [Y] [Health] [VER_LEN] [VERSION]
        const pkg = require('../package.json');
        const verBuf = Buffer.from(pkg.version);
        const idBuf = Buffer.from(player.id);
        const resp = Buffer.alloc(1 + 1 + idBuf.length + 1 + 1 + 1 + 1 + verBuf.length);
        let offset = 0;
        resp.writeUInt8(0x01, offset++);
        resp.writeUInt8(idBuf.length, offset++);
        idBuf.copy(resp, offset); offset += idBuf.length;
        resp.writeUInt8(Math.floor(player.x), offset++);
        resp.writeUInt8(Math.floor(player.y), offset++);
        resp.writeUInt8(player.health, offset++);
        resp.writeUInt8(verBuf.length, offset++);
        verBuf.copy(resp, offset); offset += verBuf.length;

        socket.write(resp);
    }

    handleMove(socket, data) {
        if (!socket.player) return;
        if (data.length < 1) return;

        const dirChar = String.fromCharCode(data[0]);
        let direction = null; // 'up', 'down', 'left', 'right'

        // Simple char mapping
        if (dirChar === 'u') direction = 'up';
        if (dirChar === 'd') direction = 'down';
        if (dirChar === 'l') direction = 'left';
        if (dirChar === 'r') direction = 'right';

        if (direction) {
            this.world.updatePlayerActivity(socket.player.id);

            let newX = socket.player.x;
            let newY = socket.player.y;

            switch (direction) {
                case 'up': newY = Math.max(0, newY - 1); break;
                case 'down': newY = Math.min(this.world.height - 1, newY + 1); break;
                case 'left': newX = Math.max(0, newX - 1); break;
                case 'right': newX = Math.min(this.world.width - 1, newX + 1); break;
            }

            if (!this.world.isValidPosition(newX, newY)) {
                // Out of bounds, ignore
                // Send current pos back to sync
            } else {
                // Check collisions
                const collidingPlayer = this.world.getPlayerAtPosition(newX, newY, socket.player.id);
                const collidingMob = this.world.getMobAtPosition(newX, newY);
                let hadCollision = false;

                if (collidingPlayer) {
                    hadCollision = true;
                    const result = CombatResolver.resolveBattle(socket.player, collidingPlayer);
                    console.log(`  ⚔️  TCP Combat: "${socket.player.name}" vs "${collidingPlayer.name}" - Winner: "${result.finalWinnerName}"`);
                    if (result.finalLoserId === socket.player.id) {
                        this.world.removePlayer(socket.player.id);
                        this.world.setKillMessage(result.finalWinnerName, result.finalLoserName, 'player');
                    } else {
                        this.world.removePlayer(collidingPlayer.id);
                        this.world.setKillMessage(result.finalWinnerName, result.finalLoserName, 'player');
                    }
                    this.world.setLastCombat(result);
                    // We don't move if we fought
                } else if (collidingMob) {
                    hadCollision = true;
                    const result = CombatResolver.resolveBattle(socket.player, collidingMob);
                    console.log(`  ⚔️  TCP Combat: "${socket.player.name}" vs "${collidingMob.name}" - Winner: "${result.finalWinnerName}"`);
                    if (result.finalLoserId === socket.player.id) {
                        this.world.removePlayer(socket.player.id);
                        this.world.setKillMessage(result.finalWinnerName, result.finalLoserName, 'player');
                    } else {
                        this.world.removeMob(collidingMob.id);
                        this.world.setKillMessage(result.finalWinnerName, result.finalLoserName, 'mob');
                    }
                    this.world.setLastCombat(result);
                } else {
                    // Move
                    socket.player.setPosition(newX, newY);
                    console.log(`  🎮 TCP Move: ${socket.player.name} to (${newX}, ${newY})`);
                }

                // Send State Update back to client: 0x02 [X] [Y] [Health] [Collision]
                const resp = Buffer.alloc(5);
                resp.writeUInt8(0x02, 0); // Type
                resp.writeUInt8(Math.floor(socket.player.x), 1);
                resp.writeUInt8(Math.floor(socket.player.y), 2);
                resp.writeUInt8(socket.player.health, 3);
                resp.writeUInt8(hadCollision ? 1 : 0, 4); // Collision flag
                socket.write(resp);
            }
        }
    }

    handleGetState(socket) {
        // Trigger world update (tick, mob movement, etc)
        const worldState = this.world.getState();
        const ticks = worldState.ticks % 65536; // Limit to 16-bit

        const players = Array.from(this.world.players.values());
        const mobs = Array.from(this.world.mobs.values());
        const all = [...players, ...mobs];

        const count = Math.min(all.length, 255);

        // Format: 0x03 [Count] [TicksLow] [TicksHigh] [Entity1: Type X Y] [Entity2: ...]
        const buf = Buffer.alloc(4 + count * 3);
        let offset = 0;
        buf.writeUInt8(0x03, offset++);
        buf.writeUInt8(count, offset++);
        buf.writeUInt8(ticks & 0xFF, offset++);        // Ticks low byte
        buf.writeUInt8((ticks >> 8) & 0xFF, offset++); // Ticks high byte

        for (let i = 0; i < count; i++) {
            const ent = all[i];
            let typeChar = 'O';
            if (ent.type === 'player') {
                typeChar = (socket.player && ent.id === socket.player.id) ? 'M' : 'P';
            } else if (ent.isHunter) {
                typeChar = 'H';
            } else {
                typeChar = 'E';
            }

            buf.writeUInt8(typeChar.charCodeAt(0), offset++);
            buf.writeUInt8(Math.floor(ent.x), offset++);
            buf.writeUInt8(Math.floor(ent.y), offset++);
        }

        socket.write(buf);
    }

    handleClose(socket) {
        this.clients.delete(socket);
        if (socket.player) {
            console.log(`TCP Client Disconnected: ${socket.player.name}`);
            this.world.removePlayer(socket.player.id);
        }
    }
}

module.exports = TcpServer;
