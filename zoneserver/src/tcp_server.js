const net = require('net');
const Player = require('./player');
const CombatResolver = require('./combat');
const logger = require('./logger');

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
            logger.info(`KillZone TCP Server running on port ${this.port}`);
        });
    }

    handleConnection(socket) {
        logger.info(`TCP connect from ${socket.remoteAddress}`);
        this.clients.add(socket);

        socket.player = null; // Associated player object
        socket.rxBuffer = Buffer.alloc(0); // TCP stream reassembly buffer
        socket.clientVersion = 'legacy'; // Overwritten by a 0x04 hello packet, if sent
        socket.connectedAt = Date.now();

        socket.on('data', (data) => this.handleData(socket, data));
        socket.on('close', () => this.handleClose(socket));
        socket.on('error', (err) => logger.error(`TCP socket error from ${socket.remoteAddress}: ${err.message}`));
    }

    handleData(socket, data) {
        if (!data || data.length === 0) {
            return;
        }

        socket.rxBuffer = Buffer.concat([socket.rxBuffer, data]);

        while (socket.rxBuffer.length > 0) {
            const packetType = socket.rxBuffer[0];
            let packetLen = 0;

            // Packet formats:
            // 0x01 [NameLen] [Name...]
            // 0x02 [DirChar]
            // 0x03
            // 0x04 [VerLen] [Version...]
            if (packetType === 0x01) {
                if (socket.rxBuffer.length < 2) {
                    return;
                }
                // Protocol guardrails: 1..31 byte names only.
                if (socket.rxBuffer[1] === 0 || socket.rxBuffer[1] > 31) {
                    logger.warn(`TCP invalid join name length: ${socket.rxBuffer[1]} from ${socket.remoteAddress}`);
                    socket.destroy();
                    return;
                }
                packetLen = 2 + socket.rxBuffer[1];
            } else if (packetType === 0x02) {
                packetLen = 2;
            } else if (packetType === 0x03) {
                packetLen = 1;
            } else if (packetType === 0x04) {
                if (socket.rxBuffer.length < 2) {
                    return;
                }
                // Protocol guardrails: 1..15 byte version strings only.
                if (socket.rxBuffer[1] === 0 || socket.rxBuffer[1] > 15) {
                    logger.warn(`TCP invalid hello version length: ${socket.rxBuffer[1]} from ${socket.remoteAddress}`);
                    socket.destroy();
                    return;
                }
                packetLen = 2 + socket.rxBuffer[1];
            } else {
                logger.warn(`TCP unknown packet type: 0x${packetType.toString(16)} from ${socket.remoteAddress}`);
                socket.rxBuffer = socket.rxBuffer.slice(1);
                continue;
            }

            if (socket.rxBuffer.length < packetLen) {
                return;
            }

            const packet = socket.rxBuffer.slice(0, packetLen);
            socket.rxBuffer = socket.rxBuffer.slice(packetLen);

            try {
                switch (packetType) {
                    case 0x01: // Join
                        this.handleJoin(socket, packet.slice(1));
                        break;
                    case 0x02: // Move
                        this.handleMove(socket, packet.slice(1));
                        break;
                    case 0x03: // Get State
                        this.handleGetState(socket);
                        break;
                    case 0x04: // Hello (client version)
                        this.handleHello(socket, packet.slice(1));
                        break;
                    default:
                        break;
                }
            } catch (e) {
                logger.error(`Error handling TCP data: ${e.message}`);
            }
        }
    }

    sendMoveResponse(socket, player, hadCollision, battleMsg, loserId = '') {
        const safeMsg = (battleMsg || '').substring(0, 39);
        const safeLoserId = (loserId || '').substring(0, 31);
        const msgBuf = Buffer.from(safeMsg);
        const loserBuf = Buffer.from(safeLoserId);

        // 0x02 [X] [Y] [Health] [Collision] [MsgLen] [Msg...] [LoserIdLen] [LoserId...]
        const resp = Buffer.alloc(6 + msgBuf.length + 1 + loserBuf.length);
        let offset = 0;
        resp.writeUInt8(0x02, offset++);
        resp.writeUInt8(Math.floor(player.x), offset++);
        resp.writeUInt8(Math.floor(player.y), offset++);
        resp.writeUInt8(player.health, offset++);
        resp.writeUInt8(hadCollision ? 1 : 0, offset++);
        resp.writeUInt8(msgBuf.length, offset++);
        msgBuf.copy(resp, offset);
        offset += msgBuf.length;
        resp.writeUInt8(loserBuf.length, offset++);
        loserBuf.copy(resp, offset);

        socket.write(resp);
    }

    handleHello(socket, data) {
        if (data.length < 1) return;
        const verLen = data[0];
        if (data.length < 1 + verLen) return;

        const version = data.slice(1, 1 + verLen).toString();
        socket.clientVersion = version;
        logger.info(`TCP hello: client v${version} from ${socket.remoteAddress}`);
    }

    handleJoin(socket, data) {
        if (data.length < 1) return;
        const nameLen = data[0];
        if (data.length < 1 + nameLen) return;

        const name = data.slice(1, 1 + nameLen).toString();

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
            logger.info(`TCP join: "${name}" (client v${socket.clientVersion}) REJOIN at (${player.x},${player.y}) -- players now ${this.world.players.size}`);
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
            logger.info(`TCP join: "${name}" (client v${socket.clientVersion}) NEW at (${player.x},${player.y}) -- players now ${this.world.players.size}`);
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

        const activePlayer = socket.player;
        const dirChar = String.fromCharCode(data[0]);
        let direction = null; // 'up', 'down', 'left', 'right'

        // Simple char mapping
        if (dirChar === 'u') direction = 'up';
        if (dirChar === 'd') direction = 'down';
        if (dirChar === 'l') direction = 'left';
        if (dirChar === 'r') direction = 'right';

        if (!direction) {
            // Keep protocol in sync even for malformed packets.
            this.sendMoveResponse(socket, activePlayer, false, '', '');
            return;
        }

        this.world.updatePlayerActivity(activePlayer.id);

        let newX = activePlayer.x;
        let newY = activePlayer.y;
        let hadCollision = false;
        let battleMsg = '';
        let loserId = '';

        switch (direction) {
            case 'up': newY = Math.max(0, newY - 1); break;
            case 'down': newY = Math.min(this.world.height - 1, newY + 1); break;
            case 'left': newX = Math.max(0, newX - 1); break;
            case 'right': newX = Math.min(this.world.width - 1, newX + 1); break;
            default:
                break;
        }

        if (!this.world.isValidPosition(newX, newY)) {
            this.sendMoveResponse(socket, activePlayer, false, '', '');
            return;
        }

        // Check collisions
        const collidingPlayer = this.world.getPlayerAtPosition(newX, newY, activePlayer.id);
        const collidingMob = this.world.getMobAtPosition(newX, newY);

        if (collidingPlayer) {
            hadCollision = true;
            const result = CombatResolver.resolveBattle(activePlayer, collidingPlayer);
            logger.info(`TCP combat: "${activePlayer.name}" vs "${collidingPlayer.name}" -> winner "${result.finalWinnerName}"`);
            battleMsg = `${result.finalWinnerName} defeats ${result.finalLoserName}!`;
            loserId = result.finalLoserId || '';
            if (result.finalLoserId === activePlayer.id) {
                this.world.removePlayer(activePlayer.id);
                this.world.setKillMessage(result.finalWinnerName, result.finalLoserName, 'player');
            } else {
                this.world.removePlayer(collidingPlayer.id);
                this.world.setKillMessage(result.finalWinnerName, result.finalLoserName, 'player');
            }
            this.world.setLastCombat(result);
            // We don't move if we fought
        } else if (collidingMob) {
            hadCollision = true;
            const result = CombatResolver.resolveBattle(activePlayer, collidingMob);
            logger.info(`TCP combat: "${activePlayer.name}" vs "${collidingMob.name}" -> winner "${result.finalWinnerName}"`);
            battleMsg = `${result.finalWinnerName} defeats ${result.finalLoserName}!`;
            loserId = result.finalLoserId || '';
            if (result.finalLoserId === activePlayer.id) {
                this.world.removePlayer(activePlayer.id);
                this.world.setKillMessage(result.finalWinnerName, result.finalLoserName, 'player');
            } else {
                this.world.removeMob(collidingMob.id);
                this.world.setKillMessage(result.finalWinnerName, result.finalLoserName, 'mob');
            }
            this.world.setLastCombat(result);
        } else {
            // Move
            activePlayer.setPosition(newX, newY);
            logger.debug(`TCP move: "${activePlayer.name}" to (${newX}, ${newY})`);
        }

        this.sendMoveResponse(socket, activePlayer, hadCollision, battleMsg, loserId);

        // Prevent dead sockets from continuing to move as ghost clients.
        if (loserId && loserId === activePlayer.id) {
            socket.player = null;
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

        // Get any pending combat message (e.g., from hunter attacks)
        let combatMsg = worldState.lastKillMessage || '';
        if (combatMsg.length > 39) {
            combatMsg = combatMsg.substring(0, 39);
        }
        const msgBuf = Buffer.from(combatMsg);

        // Format: 0x03 [Count] [TicksLow] [TicksHigh] [MsgLen] [Msg...] [Entity1: Type X Y] [Entity2: ...]
        const buf = Buffer.alloc(5 + msgBuf.length + count * 3);
        let offset = 0;
        buf.writeUInt8(0x03, offset++);
        buf.writeUInt8(count, offset++);
        buf.writeUInt8(ticks & 0xFF, offset++);        // Ticks low byte
        buf.writeUInt8((ticks >> 8) & 0xFF, offset++); // Ticks high byte
        buf.writeUInt8(msgBuf.length, offset++);       // Message length
        msgBuf.copy(buf, offset); offset += msgBuf.length;

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

        logger.debug(`TCP state -> ${socket.player ? `"${socket.player.name}"` : '?'}: count=${count} tick=${ticks}`);

        socket.write(buf);
    }

    handleClose(socket) {
        this.clients.delete(socket);
        const durationSec = socket.connectedAt ? ((Date.now() - socket.connectedAt) / 1000).toFixed(1) : '?';
        const name = socket.player ? `"${socket.player.name}"` : '(unjoined)';
        logger.info(`TCP disconnect: ${name} after ${durationSec}s`);
        if (socket.player) {
            this.world.removePlayer(socket.player.id);
        }
    }
}

module.exports = TcpServer;
