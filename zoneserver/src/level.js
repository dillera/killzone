/**
 * Level Management
 * 
 * Loads and manages level data including:
 * - Wall positions from text files
 * - Collision detection
 * - Level validation
 */

const fs = require('fs');
const path = require('path');

class Level {
  constructor(levelName = 'level1', width = 40, height = 20) {
    this.levelName = levelName;
    this.width = width;
    this.height = height;
    this.walls = new Set();  // Set of "x,y" strings for wall positions
    this.grid = [];  // 2D array for quick lookup
    
    // Initialize empty grid
    for (let y = 0; y < height; y++) {
      this.grid[y] = [];
      for (let x = 0; x < width; x++) {
        this.grid[y][x] = false;  // false = empty, true = wall
      }
    }
  }

  /**
   * Load level from file
   * @param {string} levelName - Name of level file (without .txt)
   * @returns {boolean} - Success status
   */
  loadFromFile(levelName) {
    try {
      const levelPath = path.join(__dirname, '..', 'levels', `${levelName}.txt`);
      
      if (!fs.existsSync(levelPath)) {
        console.error(`Level file not found: ${levelPath}`);
        return false;
      }

      const content = fs.readFileSync(levelPath, 'utf8');
      const lines = content.split('\n');

      // Clear existing walls
      this.walls.clear();
      for (let y = 0; y < this.height; y++) {
        for (let x = 0; x < this.width; x++) {
          this.grid[y][x] = false;
        }
      }

      // Parse level file
      for (let y = 0; y < Math.min(lines.length, this.height); y++) {
        const line = lines[y];
        for (let x = 0; x < Math.min(line.length, this.width); x++) {
          const char = line[x];
          if (char === '#') {
            this.walls.add(`${x},${y}`);
            this.grid[y][x] = true;
          }
        }
      }

      console.log(`✅ Loaded level "${levelName}" with ${this.walls.size} walls`);
      return true;
    } catch (error) {
      console.error(`Error loading level: ${error.message}`);
      return false;
    }
  }

  /**
   * Check if a position is blocked by a wall
   * @param {number} x - X coordinate
   * @param {number} y - Y coordinate
   * @returns {boolean} - True if wall exists at position
   */
  isWall(x, y) {
    if (x < 0 || x >= this.width || y < 0 || y >= this.height) {
      return false;  // Out of bounds is not a wall
    }
    return this.grid[y][x] === true;
  }

  /**
   * Check if a position is walkable
   * @param {number} x - X coordinate
   * @param {number} y - Y coordinate
   * @returns {boolean} - True if position is empty and in bounds
   */
  isWalkable(x, y) {
    if (x < 0 || x >= this.width || y < 0 || y >= this.height) {
      return false;  // Out of bounds is not walkable
    }
    return !this.isWall(x, y);
  }

  /**
   * Get all wall positions
   * @returns {Array} - Array of {x, y} objects
   */
  getWalls() {
    const wallArray = [];
    this.walls.forEach(wallStr => {
      const [x, y] = wallStr.split(',').map(Number);
      wallArray.push({ x, y });
    });
    return wallArray;
  }

  /**
   * Get level as JSON for client
   * @returns {Object} - Level data object
   */
  toJSON() {
    return {
      name: this.levelName,
      width: this.width,
      height: this.height,
      walls: this.getWalls()
    };
  }
}

module.exports = Level;
