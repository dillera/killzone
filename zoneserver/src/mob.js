/**
 * Mob (NPC) Entity
 * 
 * Server-controlled entities that move randomly around the world.
 * Used for testing multi-player rendering and combat.
 */

class Mob {
  constructor(id, name, x, y, isHunter = false) {
    this.id = id;
    this.name = name;
    this.x = x;
    this.y = y;
    this.health = 50;
    this.status = 'alive';
    this.type = 'mob';
    this.isHunter = isHunter;  // Special hunter mob with AI
    this.moveCounter = 0;
    this.moveInterval = Math.floor(Math.random() * 3) + 2; // Move every 2-4 ticks
  }

  /**
   * Move mob randomly
   * @param {number} worldWidth - World width boundary
   * @param {number} worldHeight - World height boundary
   */
  /**
   * Move toward target using Manhattan distance
   * @param {number} targetX - Target X coordinate
   * @param {number} targetY - Target Y coordinate
   * @param {number} worldWidth - World width boundary
   * @param {number} worldHeight - World height boundary
   */
  moveToward(targetX, targetY, worldWidth, worldHeight) {
    const dx = targetX - this.x;
    const dy = targetY - this.y;
    
    let newX = this.x;
    let newY = this.y;
    
    // Prefer moving in the direction with larger distance
    if (Math.abs(dx) > Math.abs(dy)) {
      // Move horizontally
      newX = this.x + (dx > 0 ? 1 : -1);
    } else {
      // Move vertically
      newY = this.y + (dy > 0 ? 1 : -1);
    }
    
    // Clamp to world boundaries
    newX = Math.max(0, Math.min(worldWidth - 1, newX));
    newY = Math.max(0, Math.min(worldHeight - 1, newY));
    
    this.x = newX;
    this.y = newY;
  }

  moveRandom(worldWidth, worldHeight) {
    this.moveCounter++;
    if (this.moveCounter < this.moveInterval) {
      return; // Not time to move yet
    }
    
    this.moveCounter = 0;
    this.moveInterval = Math.floor(Math.random() * 3) + 2; // Randomize next interval
    
    const directions = ['up', 'down', 'left', 'right'];
    const direction = directions[Math.floor(Math.random() * directions.length)];
    
    let newX = this.x;
    let newY = this.y;
    
    switch (direction) {
      case 'up':
        newY = Math.max(0, newY - 1);
        break;
      case 'down':
        newY = Math.min(worldHeight - 1, newY + 1);
        break;
      case 'left':
        newX = Math.max(0, newX - 1);
        break;
      case 'right':
        newX = Math.min(worldWidth - 1, newX + 1);
        break;
    }
    
    this.x = newX;
    this.y = newY;
  }

  /**
   * Set position
   * @param {number} x - X coordinate
   * @param {number} y - Y coordinate
   */
  setPosition(x, y) {
    this.x = x;
    this.y = y;
  }

  /**
   * Set health
   * @param {number} health - Health value
   */
  setHealth(health) {
    this.health = Math.max(0, health);
    if (this.health === 0) {
      this.status = 'dead';
    }
  }

  /**
   * Set status
   * @param {string} status - Status string (alive, dead, etc)
   */
  setStatus(status) {
    this.status = status;
  }

  /**
   * Check if mob is adjacent to target (for attack)
   * @param {number} targetX - Target X coordinate
   * @param {number} targetY - Target Y coordinate
   * @returns {boolean} - True if adjacent (within 1 move)
   */
  isAdjacentTo(targetX, targetY) {
    const dx = Math.abs(this.x - targetX);
    const dy = Math.abs(this.y - targetY);
    return dx <= 1 && dy <= 1 && (dx + dy > 0);  // Adjacent but not same position
  }
}

module.exports = Mob;
