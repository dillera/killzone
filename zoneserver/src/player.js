/**
 * Player Entity Class
 * 
 * Represents a single player in the game world with:
 * - Position tracking (x, y)
 * - Health and status
 * - Metadata (join time, name)
 */

class Player {
  constructor(id, name, x, y) {
    this.id = id;
    this.name = name;
    this.x = x;
    this.y = y;
    this.health = 100;
    this.status = 'alive'; // alive, dead, waiting
    this.joinedAt = Date.now();
    this.type = 'player';
  }

  /**
   * Update player position
   * @param {number} x - New X coordinate
   * @param {number} y - New Y coordinate
   */
  setPosition(x, y) {
    this.x = x;
    this.y = y;
  }

  /**
   * Set player health
   * @param {number} health - New health value
   */
  setHealth(health) {
    this.health = Math.max(0, Math.min(100, health));
    if (this.health === 0) {
      this.status = 'dead';
    }
  }

  /**
   * Set player status
   * @param {string} status - New status (alive, dead, waiting)
   */
  setStatus(status) {
    if (['alive', 'dead', 'waiting'].includes(status)) {
      this.status = status;
    }
  }

  /**
   * Get player as JSON object
   * @returns {Object} - Player data object
   */
  toJSON() {
    return {
      id: this.id,
      name: this.name,
      x: this.x,
      y: this.y,
      health: this.health,
      status: this.status,
      joinedAt: this.joinedAt
    };
  }
}

module.exports = Player;
