# KillZone: Bringing Real-Time Multiplayer Gaming to the Atari 8-bit

## Introduction

KillZone represents a remarkable achievement in retro computing: a fully functional real-time multiplayer game running on the Atari 8-bit computer platform, connecting to modern cloud infrastructure through the FujiNet network adapter. This project demonstrates that vintage hardware from the early 1980s can participate in contemporary networked gaming experiences, bridging four decades of computing history.

## The Technical Challenge

The Atari 8-bit computers, with their 6502 processors running at 1.79 MHz and limited RAM, were never designed for network communication. Traditional approaches to networked gaming on these platforms have relied on HTTP and JSON—protocols that, while ubiquitous, carry significant overhead that strains the limited resources of vintage hardware. Parsing JSON strings character by character on a 6502 processor introduces noticeable latency that degrades the gaming experience.

KillZone addresses this challenge through the implementation of a custom binary TCP protocol specifically designed for the constraints of 8-bit systems. By transmitting compact binary packets rather than verbose JSON structures, the game achieves response times that feel immediate to players, despite the underlying hardware limitations.

## Evolution of the Architecture

The project began with a conventional HTTP/JSON client-server architecture. The Node.js server exposed RESTful API endpoints, and the Atari client used the FujiNet network library to make HTTP requests and parse JSON responses. While functional, this approach suffered from performance issues: the combination of HTTP overhead, JSON parsing, and the inherent latency of the protocol stack resulted in gameplay that felt sluggish.

The breakthrough came with the transition to a raw TCP binary protocol. The new architecture maintains persistent TCP connections between clients and the server, eliminating the overhead of establishing new HTTP connections for each interaction. Messages are encoded as compact binary packets, with single-byte command identifiers and fixed-width numeric fields. A typical movement command now requires only two bytes, compared to potentially hundreds of bytes in the original JSON format.

## Key Technical Achievements

### Incremental Rendering

One of the significant optimizations involves the display subsystem. Rather than redrawing the entire 40×20 character game grid on each frame, the client now tracks entity positions and updates only the characters that have changed. This incremental rendering approach reduces the display update time from nearly a second to virtually instantaneous, making the game feel responsive despite the limited processing power available.

### Non-Blocking Message System

Combat notifications and game events are now handled through a non-blocking message system. When combat occurs, the message is stored in the client's state and displayed during the normal status bar update cycle. A frame counter automatically clears messages after a configurable duration, ensuring players have time to read notifications without interrupting gameplay.

### Balanced Combat System

The combat system employs a weighted probability model that provides fair but engaging gameplay. Players enjoy a 70% win rate against standard enemies, making exploration rewarding while still maintaining challenge. The Hunter enemy type provides additional difficulty, actively seeking out players and engaging them in combat.

## The FujiNet Connection

None of this would be possible without the FujiNet project, a modern network adapter that provides vintage computers with contemporary networking capabilities. FujiNet exposes a unified API for network operations, abstracting away the complexity of TCP/IP networking from the vintage system. The Atari communicates with the FujiNet device using the standard SIO (Serial I/O) interface, while FujiNet handles the actual network communication over WiFi.

## Looking Forward

KillZone demonstrates that vintage computing platforms remain capable of participating in modern networked experiences when approached with appropriate optimization techniques. The project serves as both a proof of concept for binary protocol optimization on resource-constrained systems and an entertaining game that brings the Atari 8-bit into the modern multiplayer era.

The source code is available for those interested in exploring the implementation details or adapting the techniques for their own retro computing projects. We welcome contributions from the retro computing community and look forward to seeing how these optimization strategies might be applied to other vintage platforms.

## Deep Dive: The Binary Protocol

The heart of KillZone's performance improvements lies in its compact binary protocol. This section provides a detailed examination of each message type, showing the exact byte layout and demonstrating how both the server (Node.js) and client (C/cc65) parse these messages efficiently.

### Design Principles

The protocol follows several key principles:

1. **Single-byte command identifiers** - The first byte of every message identifies its type
2. **Length-prefixed strings** - Variable-length data includes a length byte prefix
3. **Fixed-width numerics** - Coordinates and counts use single bytes (0-255 range)
4. **No delimiters or escaping** - Binary data is transmitted directly without encoding overhead

### Message Type 0x01: Join Request/Response

When a player joins the game, the client sends a minimal request and receives the server's response with their spawn position and the server version.

**Client Request:**
```
Byte 0:    0x01          Command identifier (Join)
Byte 1:    0x04          Name length (4 bytes)
Bytes 2-5: "andy"        Player name (ASCII)
```

**Server Response:**
```
Byte 0:     0x01         Command identifier (Join response)
Byte 1:     0x2B         Player ID length (43 bytes)
Bytes 2-44: "player_..."  Player ID string
Byte 45:    0x19         X position (25)
Byte 46:    0x07         Y position (7)
Byte 47:    0x64         Health (100)
Byte 48:    0x05         Server version length (5 bytes)
Bytes 49-53: "1.2.0"     Server version string
```

**Server-side encoding (Node.js):**
```javascript
const idBuf = Buffer.from(player.id);
const verBuf = Buffer.from(pkg.version);
const resp = Buffer.alloc(1 + 1 + idBuf.length + 1 + 1 + 1 + 1 + verBuf.length);
let offset = 0;
resp.writeUInt8(0x01, offset++);              // Command
resp.writeUInt8(idBuf.length, offset++);      // ID length
idBuf.copy(resp, offset); offset += idBuf.length;
resp.writeUInt8(player.x, offset++);          // X
resp.writeUInt8(player.y, offset++);          // Y
resp.writeUInt8(player.health, offset++);     // Health
resp.writeUInt8(verBuf.length, offset++);     // Version length
verBuf.copy(resp, offset);
socket.write(resp);
```

**Client-side parsing (C):**
```c
len = network_read(tcp_device_spec, buf, 2);  // Read command + ID length
idLen = buf[1];
len = network_read(tcp_device_spec, buf, idLen);  // Read ID
memcpy(player->id, buf, idLen);
player->id[idLen] = '\0';

len = network_read(tcp_device_spec, buf, 3);  // Read X, Y, Health
player->x = buf[0];
player->y = buf[1];
player->health = buf[2];

len = network_read(tcp_device_spec, buf, 1);  // Read version length
verLen = buf[0];
len = network_read(tcp_device_spec, buf, verLen);
buf[verLen] = '\0';
state_set_server_version((char*)buf);
```

### Message Type 0x02: Move Request/Response

Movement commands are the most frequent messages. The protocol minimizes these to just 2 bytes for the request.

**Client Request:**
```
Byte 0: 0x02    Command identifier (Move)
Byte 1: 0x77    Direction character ('w' = up, ASCII 119)
```

Direction characters: `w` (up), `s` (down), `a` (left), `d` (right)

**Server Response (no combat):**
```
Byte 0: 0x02    Command identifier (Move response)
Byte 1: 0x19    New X position (25)
Byte 2: 0x06    New Y position (6)
Byte 3: 0x64    Current health (100)
Byte 4: 0x00    Collision flag (0 = no collision)
Byte 5: 0x00    Message length (0 = no message)
```

**Server Response (combat occurred):**
```
Byte 0:     0x02          Command identifier
Byte 1:     0x19          X position (25)
Byte 2:     0x07          Y position (7) - didn't move, fought instead
Byte 3:     0x64          Health (100)
Byte 4:     0x01          Collision flag (1 = combat)
Byte 5:     0x14          Message length (20 bytes)
Bytes 6-25: "andy defeats Goblin1!"  Combat result message
```

**Server-side encoding (Node.js):**
```javascript
let battleMsg = hadCollision ? 
    `${winner.name} defeats ${loser.name}!` : '';
if (battleMsg.length > 39) battleMsg = battleMsg.substring(0, 39);
const msgBuf = Buffer.from(battleMsg);

const resp = Buffer.alloc(6 + msgBuf.length);
resp.writeUInt8(0x02, 0);
resp.writeUInt8(player.x, 1);
resp.writeUInt8(player.y, 2);
resp.writeUInt8(player.health, 3);
resp.writeUInt8(hadCollision ? 1 : 0, 4);
resp.writeUInt8(msgBuf.length, 5);
msgBuf.copy(resp, 6);
socket.write(resp);
```

**Client-side parsing (C):**
```c
len = network_read(tcp_device_spec, buf, 6);
result->x = buf[1];
result->y = buf[2];
player->health = buf[3];
result->collision = buf[4];

uint8_t msgLen = buf[5];
if (msgLen > 0 && msgLen < 40) {
    len = network_read(tcp_device_spec, buf, msgLen);
    buf[msgLen] = '\0';
    state_set_combat_message((char*)buf);  // Display on status line
}
```

### Message Type 0x03: World State Request/Response

The world state message transmits positions of all entities (players and mobs) for the client to render.

**Client Request:**
```
Byte 0: 0x03    Command identifier (Get State)
```

**Server Response:**
```
Byte 0:     0x03    Command identifier
Byte 1:     0x05    Entity count (5 entities)
Byte 2:     0x2A    World ticks low byte (42)
Byte 3:     0x00    World ticks high byte (0) -> ticks = 42
Byte 4:     0x12    Message length (18 bytes)
Bytes 5-22: "Hunter killed andy"  Event message

-- Entity 1 --
Byte 23:    0x4D    Type 'M' (Me/local player)
Byte 24:    0x19    X position (25)
Byte 25:    0x07    Y position (7)

-- Entity 2 --
Byte 26:    0x45    Type 'E' (Enemy/mob)
Byte 27:    0x0A    X position (10)
Byte 28:    0x0C    Y position (12)

-- Entity 3 --
Byte 29:    0x48    Type 'H' (Hunter)
Byte 30:    0x14    X position (20)
Byte 31:    0x05    Y position (5)

... (additional entities follow same pattern)
```

Entity type characters:
- `M` (0x4D) - "Me" - The requesting player
- `P` (0x50) - Other players
- `E` (0x45) - Enemy (standard mob)
- `H` (0x48) - Hunter (aggressive mob)

**Server-side encoding (Node.js):**
```javascript
const worldState = this.world.getState();
const ticks = worldState.ticks % 65536;
let combatMsg = worldState.lastKillMessage || '';
const msgBuf = Buffer.from(combatMsg);
const all = [...players, ...mobs];

const buf = Buffer.alloc(5 + msgBuf.length + all.length * 3);
let offset = 0;
buf.writeUInt8(0x03, offset++);
buf.writeUInt8(all.length, offset++);
buf.writeUInt8(ticks & 0xFF, offset++);
buf.writeUInt8((ticks >> 8) & 0xFF, offset++);
buf.writeUInt8(msgBuf.length, offset++);
msgBuf.copy(buf, offset); offset += msgBuf.length;

for (const ent of all) {
    let typeChar = ent.type === 'player' ? 
        (ent.id === socket.player.id ? 'M' : 'P') :
        (ent.isHunter ? 'H' : 'E');
    buf.writeUInt8(typeChar.charCodeAt(0), offset++);
    buf.writeUInt8(ent.x, offset++);
    buf.writeUInt8(ent.y, offset++);
}
socket.write(buf);
```

**Client-side parsing (C):**
```c
len = network_read(tcp_device_spec, buf, 5);
count = buf[1];
uint16_t ticks = buf[2] | (buf[3] << 8);
state_set_world_ticks(ticks);

uint8_t msgLen = buf[4];
if (msgLen > 0) {
    network_read(tcp_device_spec, buf, msgLen);
    buf[msgLen] = '\0';
    state_set_combat_message((char*)buf);
}

for (i = 0; i < count; i++) {
    network_read(tcp_device_spec, buf, 3);
    char typeChar = buf[0];
    uint8_t x = buf[1];
    uint8_t y = buf[2];
    
    if (typeChar == 'M') {
        // Update local player position
    } else {
        // Add to other_players array for rendering
    }
}
```

### Protocol Efficiency Analysis

Consider a typical game frame where the player moves and receives updated world state:

**JSON/HTTP Approach (original):**
- HTTP headers: ~200 bytes
- JSON request: `{"player_id":"player_123...","direction":"up"}` ~60 bytes
- JSON response: `{"x":25,"y":6,"health":100,"entities":[...]}` ~400 bytes
- **Total: ~660 bytes per frame**

**Binary TCP Approach (optimized):**
- Move request: 2 bytes
- Move response: 6 bytes (no combat) or ~30 bytes (with message)
- State request: 1 byte
- State response: 5 + (entities × 3) bytes = ~20 bytes for 5 entities
- **Total: ~30 bytes per frame**

This represents a **95% reduction** in bandwidth and eliminates JSON parsing entirely on the Atari side.

### Error Handling

The protocol includes implicit error handling through validation:

1. **Command byte validation**: First byte must match expected command type
2. **Length bounds checking**: String lengths verified before reading
3. **Position clamping**: Coordinates validated against world bounds
4. **Connection state**: Operations fail gracefully if TCP connection is lost

```c
len = network_read(tcp_device_spec, buf, 5);
if (len < 5 || buf[0] != 0x03) return 0;  // Validation
```

This design ensures that malformed packets don't crash the client and that network errors result in clean disconnection rather than undefined behavior.

## Technical Specifications

- **Client Platform**: Atari 8-bit (XL/XE series)
- **Server Platform**: Node.js
- **Network Protocol**: Custom binary TCP
- **Network Hardware**: FujiNet WiFi adapter
- **Display**: 40×20 character grid with incremental updates
- **Compiler**: cc65 cross-compiler
- **Current Version**: 1.2.0

---

*KillZone is an open-source project celebrating the enduring relevance of vintage computing platforms in the modern networked world.*
