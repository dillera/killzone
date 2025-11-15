/**
 * API Route Definitions
 * 
 * Defines all REST API endpoints for the KillZone server.
 */

const express = require('express');
const Player = require('../player');
const CollisionDetector = require('../collision');
const CombatResolver = require('../combat');

function createApiRoutes(world) {
  const router = express.Router();

  /**
   * GET /api/health
   * Server health check endpoint
   */
  router.get('/health', (req, res) => {
    const playerCount = world.getPlayerCount();
    console.log(`  📊 Health check - Players online: ${playerCount}, Uptime: ${process.uptime().toFixed(1)}s`);
    res.status(200).json({
      status: 'healthy',
      uptime: process.uptime(),
      playerCount: playerCount,
      timestamp: Date.now()
    });
  });

  /**
   * GET /api/world/state
   * Get current world snapshot
   */
  router.get('/world/state', (req, res) => {
    res.status(200).json(world.getState());
  });

  /**
   * POST /api/player/join
   * Register new player and return initial state
   */
  router.post('/player/join', (req, res) => {
    const { name } = req.body;
    console.log(`  📝 Join request received - Name: "${name}", Current players: ${world.getPlayerCount()}`);

    // Validate input
    if (!name || typeof name !== 'string' || name.trim().length === 0) {
      console.log(`  ❌ Join failed - Invalid name`);
      return res.status(400).json({
        success: false,
        error: 'Player name is required and must be a non-empty string'
      });
    }

    // Generate player ID
    const playerId = `player_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`;

    // Generate random spawn position
    let x, y;
    let attempts = 0;
    do {
      x = Math.floor(Math.random() * world.width);
      y = Math.floor(Math.random() * world.height);
      attempts++;
    } while (world.getPlayerAtPosition(x, y) !== null && attempts < 10);

    // Check if this is a rejoining player
    const isRejoin = world.isRejoiningPlayer(name);

    // Create and add player
    const player = new Player(playerId, name, x, y);
    world.addPlayer(player);

    // Broadcast join/rejoin message
    if (isRejoin) {
      world.setRejoinMessage(name);
      console.log(`  🔄 Player rejoined: "${name}" (ID: ${playerId}) at position (${x}, ${y}) - Total players: ${world.getPlayerCount()}`);
    } else {
      world.setJoinMessage(name);
      console.log(`  👤 Player joined: "${name}" (ID: ${playerId}) at position (${x}, ${y}) - Total players: ${world.getPlayerCount()}`);
    }

    res.status(201).json({
      success: true,
      id: playerId,
      name: name,
      x: x,
      y: y,
      health: player.health,
      status: player.status,
      world: world.getState()
    });
  });

  /**
   * GET /api/player/:id/status
   * Get specific player status
   */
  router.get('/player/:id/status', (req, res) => {
    const player = world.getPlayer(req.params.id);

    if (!player) {
      return res.status(404).json({
        success: false,
        error: 'Player not found'
      });
    }

    res.status(200).json({
      success: true,
      player: player.toJSON()
    });
  });

  /**
   * POST /api/player/:id/move
   * Submit player movement command
   */
  router.post('/player/:id/move', (req, res) => {
    const { direction } = req.body;
    const playerId = req.params.id;

    // Validate player exists
    const player = world.getPlayer(playerId);
    if (!player) {
      console.log(`  ❌ Move failed - Player not found: ${playerId}`);
      return res.status(404).json({
        success: false,
        error: 'Player not found'
      });
    }

    // Validate direction
    const validDirections = ['up', 'down', 'left', 'right'];
    if (!direction || !validDirections.includes(direction)) {
      console.log(`  ❌ Move failed - Invalid direction: ${direction}`);
      return res.status(400).json({
        success: false,
        error: 'Invalid direction. Must be: up, down, left, right'
      });
    }

    // Calculate new position
    let newX = player.x;
    let newY = player.y;

    switch (direction) {
      case 'up':
        newY = Math.max(0, newY - 1);
        break;
      case 'down':
        newY = Math.min(world.height - 1, newY + 1);
        break;
      case 'left':
        newX = Math.max(0, newX - 1);
        break;
      case 'right':
        newX = Math.min(world.width - 1, newX + 1);
        break;
    }

    // Check bounds
    if (!world.isValidPosition(newX, newY)) {
      console.log(`  ❌ Move failed - Out of bounds: (${newX}, ${newY})`);
      return res.status(400).json({
        success: false,
        error: 'Move would go out of bounds'
      });
    }

    // Update position
    player.setPosition(newX, newY);

    // Check for collision with other players
    const collidingPlayer = world.getPlayerAtPosition(newX, newY, playerId);
    // Check for collision with mobs
    const collidingMob = world.getMobAtPosition(newX, newY);
    
    let collision = false;
    let combatResult = null;

    if (collidingPlayer) {
      collision = true;
      combatResult = CombatResolver.resolveBattle(player, collidingPlayer);
      // Remove loser from world
      if (combatResult.finalLoserId === player.id) {
        world.removePlayer(player.id);
        // Broadcast kill message (player killed by player)
        world.setKillMessage(combatResult.finalWinnerName, combatResult.finalLoserName, 'player');
      } else {
        world.removePlayer(collidingPlayer.id);
        // Broadcast kill message (player killed by player)
        world.setKillMessage(combatResult.finalWinnerName, combatResult.finalLoserName, 'player');
      }
      world.setLastCombat(combatResult);
      console.log(`  ⚔️  Combat: "${player.name}" vs "${collidingPlayer.name}" - Winner: "${combatResult.finalWinnerName}" (${combatResult.finalScore})`);
    } else if (collidingMob) {
      collision = true;
      combatResult = CombatResolver.resolveBattle(player, collidingMob);
      // Remove loser from world
      if (combatResult.finalLoserId === player.id) {
        world.removePlayer(player.id);
        // Broadcast kill message (player killed by mob)
        world.setKillMessage(combatResult.finalWinnerName, combatResult.finalLoserName, 'player');
      } else {
        world.removeMob(collidingMob.id);
        // No broadcast for mob deaths
      }
      world.setLastCombat(combatResult);
      console.log(`  ⚔️  Combat: "${player.name}" vs "${collidingMob.name}" - Winner: "${combatResult.finalWinnerName}" (${combatResult.finalScore})`);
    } else {
      console.log(`  🎮 ${player.name} moved ${direction} to (${newX}, ${newY})`);
    }

    res.status(200).json({
      success: true,
      playerId: playerId,
      newPos: {
        x: newX,
        y: newY
      },
      collision: collision,
      combatResult: combatResult,
      worldState: world.getState()
    });
  });

  /**
   * POST /api/player/leave
   * Unregister player from world
   */
  router.post('/player/leave', (req, res) => {
    const { id } = req.body;

    if (!id) {
      console.log(`  ❌ Leave failed - No player ID provided`);
      return res.status(400).json({
        success: false,
        error: 'Player ID is required'
      });
    }

    const player = world.getPlayer(id);
    const removed = world.removePlayer(id);

    if (!removed) {
      console.log(`  ❌ Leave failed - Player not found: ${id}`);
      return res.status(404).json({
        success: false,
        error: 'Player not found'
      });
    }

    console.log(`  👋 Player left: "${player.name}" (ID: ${id}) - Total players: ${world.getPlayerCount()}`);

    res.status(200).json({
      success: true,
      id: id
    });
  });

  /**
   * POST /api/player/:id/attack
   * Initiate directed attack on adjacent/nearby player (optional future endpoint)
   */
  router.post('/player/:id/attack', (req, res) => {
    const { targetX, targetY } = req.body;
    const playerId = req.params.id;

    const player = world.getPlayer(playerId);
    if (!player) {
      return res.status(404).json({
        success: false,
        error: 'Player not found'
      });
    }

    if (targetX === undefined || targetY === undefined) {
      return res.status(400).json({
        success: false,
        error: 'Target coordinates required'
      });
    }

    const target = world.getPlayerAtPosition(targetX, targetY);
    if (!target) {
      return res.status(404).json({
        success: false,
        error: 'No target at specified position'
      });
    }

    const combatResult = CombatResolver.resolveCombat(player, target);
    world.removePlayer(combatResult.loserId);

    res.status(200).json({
      success: true,
      combatResult: combatResult,
      worldState: world.getState()
    });
  });

  return router;
}

module.exports = createApiRoutes;
