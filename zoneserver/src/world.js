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
    this.disconnectedPlayers = new Map(); // playerName -> Player object (for reconnection)
    this.timestamp = Date.now();
    this.ticks = 0;
    this.lastCombatLog = '';
    this.lastCombatTimestamp = 0;
    this.lastCombatWinner = '';
    this.lastCombatLoser = '';
    this.lastCombatScore = '';
    this.lastCombatMessages = [];
    this.lastKillMessage = '';
    this.lastKillTimestamp = 0;
    this.previousPlayerNames = new Set(); // Track player names for rejoin detection
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
    player.lastActivity = Date.now();  // Track activity for disconnect cleanup
    this.players.set(player.id, player);
    // Track player name for rejoin detection
    if (player.name) {
      this.previousPlayerNames.add(player.name);
    }
    this.timestamp = Date.now();
    return true;
  }

  updatePlayerActivity(playerId) {
    const player = this.players.get(playerId);
    if (player) {
      player.lastActivity = Date.now();
    }
  }

  cleanupInactivePlayers(timeoutMs = 120000) {
    // 120000ms = 2 minutes
    const now = Date.now();
    const inactivePlayers = [];

    for (const [playerId, player] of this.players.entries()) {
      if (now - player.lastActivity > timeoutMs) {
        inactivePlayers.push({ id: playerId, name: player.name });
        this.removePlayer(playerId);
      }
    }

    return inactivePlayers;
  }

  isRejoiningPlayer(playerName) {
    return this.previousPlayerNames.has(playerName);
  }

  /**
   * Remove a player from the world (move to disconnected for later reconnection)
   * @param {string} playerId - ID of player to remove
   * @returns {boolean} - Success status
   */
  removePlayer(playerId) {
    const player = this.players.get(playerId);
    if (player) {
      // Store in disconnected players by name for reconnection
      this.disconnectedPlayers.set(player.name, player);
      this.players.delete(playerId);
      this.timestamp = Date.now();
      return true;
    }
    return false;
  }

  /**
   * Get a disconnected player by name
   * @param {string} playerName - Name of player to retrieve
   * @returns {Player|null} - Player object or null if not found
   */
  getDisconnectedPlayer(playerName) {
    return this.disconnectedPlayers.get(playerName) || null;
  }

  /**
   * Remove a player from disconnected list (when they reconnect)
   * @param {string} playerName - Name of player to remove
   * @returns {boolean} - Success status
   */
  removeDisconnectedPlayer(playerName) {
    return this.disconnectedPlayers.delete(playerName);
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

  setKillMessage(winnerName, loserName, loserType) {
    const now = Date.now();
    if (loserType === 'player') {
      this.lastKillMessage = `${winnerName} killed ${loserName}!`;
    } else {
      this.lastKillMessage = `${winnerName} killed ${loserName}`;
    }
    this.lastKillTimestamp = now;
  }

  setRejoinMessage(playerName) {
    this.lastKillMessage = `${playerName} has rejoined the game!`;
    this.lastKillTimestamp = Date.now();
  }

  setJoinMessage(playerName) {
    this.lastKillMessage = `${playerName} joined the game!`;
    this.lastKillTimestamp = Date.now();
  }

  clearKillMessage() {
    this.lastKillMessage = '';
    this.lastKillTimestamp = 0;
  }

  /**
   * Get all mobs
   * @returns {Array} - Array of all mobs
   */
  getAllMobs() {
    return Array.from(this.mobs.values());
  }

  /**
   * Update all mobs (move them randomly or toward players, and attack if adjacent)
   */
  updateMobs() {
    const CombatResolver = require('./combat');
    
    for (const mob of this.mobs.values()) {
      if (mob.isHunter) {
        // Hunter mob: look for nearby players
        let nearestPlayer = null;
        let nearestDistance = Infinity;
        
        for (const player of this.players.values()) {
          const distance = Math.abs(mob.x - player.x) + Math.abs(mob.y - player.y);
          if (distance <= 10 && distance < nearestDistance) {
            nearestPlayer = player;
            nearestDistance = distance;
          }
        }
        
        if (nearestPlayer) {
          // Check if adjacent - if so, attack!
          if (mob.isAdjacentTo(nearestPlayer.x, nearestPlayer.y)) {
            // Combat between hunter and player
            const combatResult = CombatResolver.resolveBattle(mob, nearestPlayer);
            
            // Remove loser
            if (combatResult.finalLoserId === nearestPlayer.id) {
              this.removePlayer(nearestPlayer.id);
              this.setKillMessage(combatResult.finalWinnerName, combatResult.finalLoserName, 'player');
              console.log(`  ⚔️  Hunter Combat: "${mob.name}" vs "${nearestPlayer.name}" - Winner: "${combatResult.finalWinnerName}" (${combatResult.finalScore})`);
            } else {
              this.removeMob(mob.id);
              console.log(`  ⚔️  Hunter Combat: "${mob.name}" vs "${nearestPlayer.name}" - Winner: "${combatResult.finalWinnerName}" (${combatResult.finalScore})`);
            }
            this.setLastCombat(combatResult);
          } else {
            // Not adjacent, move toward player
            mob.moveToward(nearestPlayer.x, nearestPlayer.y, this.width, this.height);
          }
        } else {
          // No player in range, move randomly
          mob.moveRandom(this.width, this.height);
        }
      } else {
        // Regular mob: move randomly
        mob.moveRandom(this.width, this.height);
      }
    }
  }

  /**
   * Ensure minimum mob count, spawn new ones if needed
   * @param {number} minMobs - Minimum number of mobs to maintain
   * @returns {Array} - Array of newly spawned mobs
   */
  respawnMobs(minMobs = 3) {
    const spawnedMobs = [];
    const currentCount = this.mobs.size;
    
    if (currentCount < minMobs) {
      const toSpawn = minMobs - currentCount;
      
      for (let i = 0; i < toSpawn; i++) {
        // Generate random position
        let x, y;
        let attempts = 0;
        do {
          x = Math.floor(Math.random() * this.width);
          y = Math.floor(Math.random() * this.height);
          attempts++;
        } while (this.getPlayerAtPosition(x, y) !== null && attempts < 10);
        
        // Determine if this should be a hunter mob
        // Only one hunter at a time - check if one exists
        let hasHunter = false;
        for (const mob of this.mobs.values()) {
          if (mob.isHunter) {
            hasHunter = true;
            break;
          }
        }
        
        const isHunter = !hasHunter && i === 0;  // First spawn is hunter if none exists
        const mobId = `mob_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`;
        const mobName = isHunter ? 'Hunter' : `Goblin${currentCount + i + 1}`;
        
        const mob = new (require('./mob'))(mobId, mobName, x, y, isHunter);
        this.addMob(mob);
        spawnedMobs.push(mob);
      }
    }
    
    return spawnedMobs;
  }

  /**
   * Get world state snapshot for API responses
   * @returns {Object} - World state object
   */
  getState() {
    this.ticks++;  /* Increment world ticks on each state query */
    
    /* Update mobs every tick */
    this.updateMobs();
    
    /* Auto-clear kill message after 5 seconds */
    if (this.lastKillMessage && this.lastKillTimestamp) {
      const elapsed = Date.now() - this.lastKillTimestamp;
      if (elapsed > 5000) {
        this.clearKillMessage();
      }
    }
    
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
        type: 'mob',
        isHunter: m.isHunter || false
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
      lastCombatMessages: this.lastCombatMessages,
      lastKillMessage: this.lastKillMessage,
      lastKillTimestamp: this.lastKillTimestamp
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
    this.lastKillMessage = '';
    this.lastKillTimestamp = 0;
  }
}

module.exports = World;
