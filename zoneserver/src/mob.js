/**
 * Mob (NPC) Entity
 * 
 * Server-controlled entities that move randomly around the world.
 * Used for testing multi-player rendering and combat.
 */

class Mob {
  constructor(id, x, y, name = 'Mob') {
    this.id = id;
    this.x = x;
    this.y = y;
    this.name = name;
    this.health = 50;
    this.status = 'alive';
    this.moveCounter = 0;
    this.moveInterval = Math.floor(Math.random() * 3) + 2; // Move every 2-4 ticks
    this.type = 'mob';
  }

  /**
   * Move mob randomly
   * @param {number} worldWidth - World width boundary
   * @param {number} worldHeight - World height boundary
   */
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
}

module.exports = Mob;
