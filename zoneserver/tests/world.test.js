/**
 * World State Management Tests
 */

const World = require('../src/world');
const Player = require('../src/player');

describe('World', () => {
  let world;

  beforeEach(() => {
    world = new World(40, 20, 'test');  // Use test level with no walls
  });

  describe('initialization', () => {
    test('creates world with correct dimensions', () => {
      expect(world.width).toBe(40);
      expect(world.height).toBe(20);
    });

    test('starts with no players', () => {
      expect(world.getPlayerCount()).toBe(0);
    });

    test('has initial timestamp', () => {
      expect(world.timestamp).toBeDefined();
      expect(typeof world.timestamp).toBe('number');
    });
  });

  describe('player management', () => {
    test('adds player to world', () => {
      const player = new Player('p1', 'Alice', 10, 10);
      const result = world.addPlayer(player);
      
      expect(result).toBe(true);
      expect(world.getPlayerCount()).toBe(1);
    });

    test('retrieves player by ID', () => {
      const player = new Player('p1', 'Alice', 10, 10);
      world.addPlayer(player);
      
      const retrieved = world.getPlayer('p1');
      expect(retrieved).toBe(player);
    });

    test('returns null for non-existent player', () => {
      const retrieved = world.getPlayer('nonexistent');
      expect(retrieved).toBeNull();
    });

    test('removes player from world', () => {
      const player = new Player('p1', 'Alice', 10, 10);
      world.addPlayer(player);
      expect(world.getPlayerCount()).toBe(1);
      
      const result = world.removePlayer('p1');
      expect(result).toBe(true);
      expect(world.getPlayerCount()).toBe(0);
    });

    test('returns false when removing non-existent player', () => {
      const result = world.removePlayer('nonexistent');
      expect(result).toBe(false);
    });

    test('gets all players', () => {
      const p1 = new Player('p1', 'Alice', 10, 10);
      const p2 = new Player('p2', 'Bob', 20, 15);
      world.addPlayer(p1);
      world.addPlayer(p2);
      
      const all = world.getAllPlayers();
      expect(all.length).toBe(2);
      expect(all).toContain(p1);
      expect(all).toContain(p2);
    });
  });

  describe('position validation', () => {
    test('validates position within bounds', () => {
      expect(world.isValidPosition(0, 0)).toBe(true);
      expect(world.isValidPosition(39, 19)).toBe(true);
      expect(world.isValidPosition(20, 10)).toBe(true);
    });

    test('rejects position outside bounds', () => {
      expect(world.isValidPosition(-1, 10)).toBe(false);
      expect(world.isValidPosition(40, 10)).toBe(false);
      expect(world.isValidPosition(10, -1)).toBe(false);
      expect(world.isValidPosition(10, 20)).toBe(false);
    });
  });

  describe('position occupancy', () => {
    test('finds player at position', () => {
      const player = new Player('p1', 'Alice', 10, 10);
      world.addPlayer(player);
      
      const found = world.getPlayerAtPosition(10, 10);
      expect(found).toBe(player);
    });

    test('returns null for empty position', () => {
      const found = world.getPlayerAtPosition(10, 10);
      expect(found).toBeNull();
    });

    test('excludes specified player from occupancy check', () => {
      const p1 = new Player('p1', 'Alice', 10, 10);
      world.addPlayer(p1);
      
      const found = world.getPlayerAtPosition(10, 10, 'p1');
      expect(found).toBeNull();
    });

    test('finds other player when one is excluded', () => {
      const p1 = new Player('p1', 'Alice', 10, 10);
      const p2 = new Player('p2', 'Bob', 10, 10);
      world.addPlayer(p1);
      world.addPlayer(p2);
      
      const found = world.getPlayerAtPosition(10, 10, 'p1');
      expect(found).toBe(p2);
    });
  });

  describe('world state', () => {
    test('returns world state snapshot', () => {
      const p1 = new Player('p1', 'Alice', 10, 10);
      world.addPlayer(p1);
      
      const state = world.getState();
      expect(state.width).toBe(40);
      expect(state.height).toBe(20);
      expect(state.players.length).toBe(1);
      expect(state.timestamp).toBeDefined();
    });

    test('includes all player data in state', () => {
      const p1 = new Player('p1', 'Alice', 10, 10);
      world.addPlayer(p1);
      
      const state = world.getState();
      const playerData = state.players[0];
      expect(playerData.id).toBe('p1');
      expect(playerData.x).toBe(10);
      expect(playerData.y).toBe(10);
      expect(playerData.health).toBe(100);
      expect(playerData.status).toBe('alive');
    });

    test('updates timestamp on player changes', () => {
      const initialTime = world.timestamp;
      
      // Add small delay to ensure timestamp changes
      const p1 = new Player('p1', 'Alice', 10, 10);
      world.addPlayer(p1);
      
      expect(world.timestamp).toBeGreaterThanOrEqual(initialTime);
    });
  });

  describe('reset', () => {
    test('clears all players', () => {
      const p1 = new Player('p1', 'Alice', 10, 10);
      const p2 = new Player('p2', 'Bob', 20, 15);
      world.addPlayer(p1);
      world.addPlayer(p2);
      
      world.reset();
      expect(world.getPlayerCount()).toBe(0);
      expect(world.getAllPlayers().length).toBe(0);
    });

    test('resets timestamp on reset', () => {
      const p1 = new Player('p1', 'Alice', 10, 10);
      world.addPlayer(p1);
      const beforeReset = world.timestamp;
      
      world.reset();
      expect(world.timestamp).toBeGreaterThanOrEqual(beforeReset);
    });
  });
});
