/**
 * World State Management
 * 
 * Manages the shared game world state including:
 * - World dimensions (40x20 grid)
 * - Player entity tracking
 * - Position validation
 * - World persistence across client connections
 */

class World {
  constructor(width = 40, height = 20) {
    this.width = width;
    this.height = height;
    this.players = new Map(); // playerId -> Player object
    this.mobs = new Map(); // mobId -> Mob object
    this.timestamp = Date.now();
    this.ticks = 0;
    this.lastCombatLog = '';
    this.lastCombatTimestamp = 0;
    this.lastCombatWinner = '';
    this.lastCombatLoser = '';
    this.lastCombatScore = '';
    this.lastCombatMessages = [];
  }

  /**
   * Add a player to the world
   * @param {Player} player - Player object to add
   * @returns {boolean} - Success status
   */
  addPlayer(player) {
    if (!player || !player.id) {
      return false;
    }
    this.players.set(player.id, player);
    this.timestamp = Date.now();
    return true;
  }

  /**
   * Remove a player from the world
   * @param {string} playerId - ID of player to remove
   * @returns {boolean} - Success status
   */
  removePlayer(playerId) {
    const removed = this.players.delete(playerId);
    if (removed) {
      this.timestamp = Date.now();
    }
    return removed;
  }

  /**
   * Get a player by ID
   * @param {string} playerId - ID of player to retrieve
   * @returns {Player|null} - Player object or null if not found
   */
  getPlayer(playerId) {
    return this.players.get(playerId) || null;
  }

  /**
   * Get all players in the world
   * @returns {Array} - Array of all player objects
   */
  getAllPlayers() {
    return Array.from(this.players.values());
  }

  /**
   * Get player count
   * @returns {number} - Number of players in world
   */
  getPlayerCount() {
    return this.players.size;
  }

  /**
   * Check if a position is valid (within bounds)
   * @param {number} x - X coordinate
   * @param {number} y - Y coordinate
   * @returns {boolean} - True if position is valid
   */
  isValidPosition(x, y) {
    return x >= 0 && x < this.width && y >= 0 && y < this.height;
  }

  /**
   * Check if a position is occupied by another player
   * @param {number} x - X coordinate
   * @param {number} y - Y coordinate
   * @param {string} excludePlayerId - Player ID to exclude from check (optional)
   * @returns {Player|null} - Player at position or null if empty
   */
  getPlayerAtPosition(x, y, excludePlayerId = null) {
    for (const player of this.players.values()) {
      if (player.x === x && player.y === y) {
        if (excludePlayerId && player.id === excludePlayerId) {
          continue;
        }
        return player;
      }
    }
    return null;
  }

  getMob(mobId) {
    return this.mobs.get(mobId) || null;
  }

  getMobAtPosition(x, y) {
    for (const mob of this.mobs.values()) {
      if (mob.x === x && mob.y === y) {
        return mob;
      }
    }
    return null;
  }

  /**
   * Add a mob to the world
   * @param {Mob} mob - Mob object to add
   * @returns {boolean} - Success status
   */
  addMob(mob) {
    if (!mob || !mob.id) {
      return false;
    }
    this.mobs.set(mob.id, mob);
    this.timestamp = Date.now();
    return true;
  }

  /**
   * Remove a mob from the world
   * @param {string} mobId - ID of mob to remove
   * @returns {boolean} - Success status
   */
  removeMob(mobId) {
    const removed = this.mobs.delete(mobId);
    if (removed) {
      this.timestamp = Date.now();
    }
    return removed;
  }

  setLastCombat(result) {
    if (!result) {
      return;
    }
    this.lastCombatLog = result.combatLog || '';
    this.lastCombatTimestamp = result.timestamp || Date.now();
    this.lastCombatWinner = result.finalWinnerName || '';
    this.lastCombatLoser = result.finalLoserName || '';
    this.lastCombatScore = result.finalScore || '';
    this.lastCombatMessages = result.messages || [];
  }

  /**
   * Get all mobs
   * @returns {Array} - Array of all mobs
   */
  getAllMobs() {
    return Array.from(this.mobs.values());
  }

  /**
   * Update all mobs (move them randomly)
   */
  updateMobs() {
    for (const mob of this.mobs.values()) {
      mob.moveRandom(this.width, this.height);
    }
  }

  /**
   * Get world state snapshot for API responses
   * @returns {Object} - World state object
   */
  getState() {
    this.ticks++;  /* Increment world ticks on each state query */
    
    /* Update mobs every tick */
    this.updateMobs();
    
    /* Combine players and mobs for the response */
    const allEntities = [
      ...this.getAllPlayers().map(p => ({
        id: p.id,
        x: p.x,
        y: p.y,
        health: p.health,
        status: p.status,
        type: 'player'
      })),
      ...this.getAllMobs().map(m => ({
        id: m.id,
        x: m.x,
        y: m.y,
        health: m.health,
        status: m.status,
        type: 'mob'
      }))
    ];
    
    return {
      width: this.width,
      height: this.height,
      players: allEntities,
      ticks: this.ticks,
      timestamp: this.timestamp,
      lastCombatTimestamp: this.lastCombatTimestamp,
      lastCombatLog: this.lastCombatLog,
      lastCombatWinner: this.lastCombatWinner,
      lastCombatLoser: this.lastCombatLoser,
      lastCombatScore: this.lastCombatScore,
      lastCombatMessages: this.lastCombatMessages
    };
  }

  /**
   * Reset world to initial state
   */
  reset() {
    this.players.clear();
    this.mobs.clear();
    this.timestamp = Date.now();
    this.lastCombatLog = '';
    this.lastCombatTimestamp = 0;
    this.lastCombatWinner = '';
    this.lastCombatLoser = '';
    this.lastCombatScore = '';
    this.lastCombatMessages = [];
  }
}

module.exports = World;
