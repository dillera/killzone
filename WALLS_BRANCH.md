# Walls Branch - Level System Implementation

## Overview

This branch introduces a text-based level system that allows defining game levels with walls. Each level is a simple 40x20 grid where characters represent different elements:

- `.` = empty walkable space
- `#` = wall (blocks movement)

## Architecture

### Level Class (`zoneserver/src/level.js`)

The `Level` class manages all level data:

```javascript
const level = new Level('level1', 40, 20);
level.loadFromFile('level1');

// Check if position is a wall
if (level.isWall(x, y)) { /* blocked */ }

// Check if position is walkable
if (level.isWalkable(x, y)) { /* can move */ }

// Get all walls
const walls = level.getWalls();  // Returns [{x, y}, ...]
```

**Key Methods:**
- `loadFromFile(levelName)` - Load level from text file
- `isWall(x, y)` - Check if position has a wall
- `isWalkable(x, y)` - Check if position is empty and in bounds
- `getWalls()` - Get array of all wall positions
- `toJSON()` - Export level data for client

### World Integration

The `World` class now:
1. Loads a level on initialization
2. Validates positions against walls
3. Includes walls in world state responses

```javascript
// Server initialization
const world = new World(40, 20, 'level1');

// Position validation now checks walls
if (world.isValidPosition(x, y)) {
  // Position is in bounds AND not a wall
}

// World state includes walls
const state = world.getState();
// state.walls = [{x, y}, ...]
```

## Level Files

### levels/test.txt
- Empty 40x20 grid
- Used for testing (no walls to interfere)
- Loaded when `NODE_ENV=test`

### levels/level1.txt
- Simple bordered level
- Walls around perimeter
- 116 total walls
- Good for basic gameplay

### levels/level2.txt
- Complex level with interior rooms
- Multiple chambers connected by corridors
- Walls create maze-like structure
- Good for advanced gameplay

## File Format

Each level file is exactly 40 characters wide and 20 lines tall:

```
########################################
#......................................#
#.###########.....###########.........#
#.#.........#.....#.........#.........#
#.#.........#.....#.........#.........#
#.#.........#.....#.........#.........#
#.#.........#.....#.........#.........#
#.###########.....###########.........#
#......................................#
#......................................#
#......................................#
#.....#########.....#########.........#
#.....#.......#.....#.......#.........#
#.....#.......#.....#.......#.........#
#.....#########.....#########.........#
#......................................#
#......................................#
#......................................#
#......................................#
########################################
```

**Rules:**
- Exactly 40 characters per line
- Exactly 20 lines total
- Use `#` for walls, `.` for empty space
- Any other character is treated as empty space

## Collision Detection

### Player Movement
- Players cannot move into walls
- Move requests to wall positions return error
- Server validates all moves against level

### Mob Movement
- Mobs cannot move into walls
- Random movement avoids walls
- Hunter mob pathfinding respects walls

## Server Behavior

### Logging
```
✅ Loaded level "level1" with 116 walls
✅ Loaded level "test" with 0 walls
```

### World State Response
```json
{
  "width": 40,
  "height": 20,
  "players": [...],
  "walls": [
    {"x": 0, "y": 0},
    {"x": 1, "y": 0},
    ...
  ],
  "ticks": 123,
  ...
}
```

## Testing

- Tests use `test` level (no walls)
- Production uses `level1` by default
- Environment variable `NODE_ENV` controls level selection
- All 103 tests passing ✅

## Next Steps

### Client-Side Wall Rendering
- Parse walls from world state
- Render walls on game display
- Update collision detection on client

### Level Management
- Level switching/progression
- Dynamic level loading
- Level editor tool

### Advanced Features
- Destructible walls
- Moving platforms
- Teleporters
- Level-specific spawn points

## Creating New Levels

1. Create a new file in `zoneserver/levels/` (e.g., `level3.txt`)
2. Use exactly 40 characters wide, 20 lines tall
3. Use `#` for walls, `.` for empty space
4. Load in server: `new World(40, 20, 'level3')`
5. Test with: `npm test`

## Usage

### Development
```bash
# Run with test level (no walls)
NODE_ENV=test npm start

# Run with level1 (default)
npm start
```

### In Code
```javascript
// Load a specific level
const world = new World(40, 20, 'level2');

// Check if position is walkable
if (world.isValidPosition(x, y)) {
  player.move(x, y);
}

// Get walls for rendering
const state = world.getState();
const walls = state.walls;
```

## Files Modified

- `zoneserver/src/level.js` - New Level class
- `zoneserver/src/world.js` - Integrated level loading and validation
- `zoneserver/src/server.js` - Initialize world with level
- `zoneserver/tests/world.test.js` - Use test level
- `zoneserver/levels/test.txt` - New test level
- `zoneserver/levels/level1.txt` - New bordered level
- `zoneserver/levels/level2.txt` - New complex level

## Branch Status

✅ Server-side level system complete
✅ Wall collision detection working
✅ Level loading from text files
✅ All tests passing (103/103)

⏳ Pending: Client-side wall rendering
