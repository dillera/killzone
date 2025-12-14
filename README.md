# KillZone Quick Start Guide

## 30-Second Setup

### Terminal 1: Start Server
```bash
cd zoneserver
npm install
npm start
```
Server runs on `http://localhost:3000`

### Terminal 2: Build & Run Client
```bash
cd client
make
~/atari800 build/client.bin
```

### Terminal 3: Run Tests (optional)
```bash
cd zoneserver
npm test
```

---

## What Was Built

A complete multiplayer game system with:

- **Server** (Node.js): Manages shared 40x20 world, player positions, collisions, combat
- **Client** (Atari 8-bit C): Game client that connects to server via HTTP
- **95 Tests**: All passing, covering all major functionality
- **Full Documentation**: Architecture, API contract, development workflow

---

## Key Features

✅ **World Management** - 40x20 grid with player tracking
✅ **Player Lifecycle** - Join, move, combat, leave
✅ **Collision Detection** - Automatic detection when players meet
✅ **Combat System** - 50/50 random winner determination
✅ **REST API** - 7 endpoints for client communication
✅ **Error Handling** - Comprehensive validation and error responses
✅ **Modular Design** - Clean black-box interfaces between components
✅ **Comprehensive Tests** - 95 tests covering all functionality

---

## Project Structure

```
killzone/
├── zoneserver/          # Node.js server
│   ├── src/            # Core modules
│   ├── tests/          # 95 tests
│   └── README.md       # Server docs
│
├── client/             # Atari 8-bit client
│   ├── *.c/h          # Game code
│   ├── Makefile       # Build system
│   └── README.md      # Client docs
│
├── README.md          # Main project docs
├── claude.md          # Detailed spec
└── PROJECT_STATUS.md  # Completion status
```

---

## API Endpoints

| Method | Endpoint | Purpose |
|--------|----------|---------|
| GET | `/api/health` | Server health check |
| GET | `/api/world/state` | Get current world |
| POST | `/api/player/join` | Register player |
| GET | `/api/player/:id/status` | Get player status |
| POST | `/api/player/:id/move` | Move player |
| POST | `/api/player/leave` | Unregister player |
| POST | `/api/player/:id/attack` | Attack (optional) |

---

## Testing

```bash
# Run all tests
npm test

# Watch mode
npm run test:watch

# Coverage report
npm test -- --coverage

# Integration tests only
npm run test:integration
```

**Result**: 95 tests passing ✅

---

## Development Workflow

### For Server Development
```bash
cd zoneserver
npm run test:watch    # Auto-run tests on changes
npm start             # In another terminal
```

### For Client Development
```bash
cd client
make watch            # Auto-rebuild on changes
~/atari800 build/client.bin  # Test in emulator
```

---

## Game Rules

- **World**: 40x20 grid (x: 0-39, y: 0-19)
- **Movement**: One cell per direction (up/down/left/right)
- **Collision**: Automatic when players occupy same position
- **Combat**: Random 50/50 winner, loser removed
- **Respawn**: Loser can rejoin with new player ID

---

## Architecture Highlights

### Black Box Design
- Clean interfaces between modules
- Implementation details hidden
- Easy to replace components
- Clear separation of concerns

### Modular Components
- **World**: Independent state management
- **Player**: Simple entity class
- **Collision**: Separate detection logic
- **Combat**: Independent resolution
- **API**: Clean endpoint definitions

### Testability
- All modules unit tested
- Integration tests for workflows
- Mock-friendly interfaces
- No hard-coded dependencies

---

## Next Steps

### Phase 2: Player Management
- Enhanced join/leave flow
- Player name validation
- Multi-player display

### Phase 3: Movement & Sync
- Movement validation
- Real-time synchronization
- Joystick input handling

### Phase 4: Combat
- Combat feedback
- Respawn handling
- Multiple concurrent combats

### Phase 5: Polish & Scale
- Performance optimization
- Concurrent client handling
- Network resilience

---

## Troubleshooting

### Server won't start
```bash
# Check if port 3000 is in use
lsof -i :3000

# Kill process if needed
kill -9 <PID>
```

### Tests failing
```bash
# Make sure NODE_ENV is set
NODE_ENV=test npm test

# Check dependencies installed
npm install
```

### Client won't compile
```bash
# Verify cc65 is installed
cl65 --version

# Check Makefile
cat client/Makefile
```

---

## Key Files

| File | Purpose |
|------|---------|
| `zoneserver/src/server.js` | Express server entry point |
| `zoneserver/src/world.js` | World state management |
| `zoneserver/src/routes/api.js` | API endpoint definitions |
| `client/main.c` | Game loop and state machine |
| `client/network.c` | HTTP communication |
| `client/Makefile` | Build system |

---

## Documentation

- **README.md** - Main project overview
- **claude.md** - Detailed 798-line specification
- **PROJECT_STATUS.md** - Completion status and checklist
- **zoneserver/README.md** - Server architecture and testing
- **client/README.md** - Client setup and usage

---

## Technology Stack

**Server**
- Node.js 18+
- Express.js 4.18+
- Jest 29.7+ (testing)

**Client**
- C language
- cc65 toolchain
- Atari 800 emulator
- FujiNet HTTP library

---

## Performance

- **World Size**: 40x20 (800 cells)
- **Collision Detection**: O(n²)
- **Suitable for**: 10-20 concurrent players
- **Memory**: ~40KB on Atari
- **Network**: HTTP over FujiNet

---

## Success Criteria Met ✅

- [x] Server compiles and runs
- [x] All 95 tests pass
- [x] API endpoints working
- [x] Client compiles with cc65
- [x] Makefile builds successfully
- [x] Documentation complete
- [x] Code quality standards met
- [x] Error handling comprehensive
- [x] Input validation thorough
- [x] Project structure clean

---

**Status**: Phase 1 Complete ✅
**Ready for**: Phase 2 Development
**Last Updated**: November 9, 2025
