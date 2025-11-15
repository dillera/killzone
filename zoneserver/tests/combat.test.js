/**
 * Combat Resolution Tests
 */

const CombatResolver = require('../src/combat');
const Player = require('../src/player');

describe('CombatResolver', () => {
  describe('resolveBattle', () => {
    test('returns combat result with winner and loser', () => {
      const p1 = new Player('p1', 'Alice', 10, 10);
      const p2 = new Player('p2', 'Bob', 10, 10);

      const result = CombatResolver.resolveBattle(p1, p2);

      expect(result).toBeDefined();
      expect(result.type).toBe('combat');
      expect(result.finalWinnerId).toBeDefined();
      expect(result.finalLoserId).toBeDefined();
      expect(result.finalWinnerName).toBeDefined();
      expect(result.finalLoserName).toBeDefined();
      expect(result.timestamp).toBeDefined();
      expect(result.rounds).toBeDefined();
      expect(result.rounds.length).toBe(3);
      expect(result.messages).toBeDefined();
      expect(result.messages.length).toBe(4); // 3 rounds + final
    });

    test('winner is one of the combatants', () => {
      const p1 = new Player('p1', 'Alice', 10, 10);
      const p2 = new Player('p2', 'Bob', 10, 10);

      const result = CombatResolver.resolveBattle(p1, p2);

      expect([p1.id, p2.id]).toContain(result.finalWinnerId);
      expect([p1.id, p2.id]).toContain(result.finalLoserId);
    });

    test('winner and loser are different', () => {
      const p1 = new Player('p1', 'Alice', 10, 10);
      const p2 = new Player('p2', 'Bob', 10, 10);

      const result = CombatResolver.resolveBattle(p1, p2);

      expect(result.finalWinnerId).not.toBe(result.finalLoserId);
    });

    test('marks loser as dead', () => {
      const p1 = new Player('p1', 'Alice', 10, 10);
      const p2 = new Player('p2', 'Bob', 10, 10);

      const result = CombatResolver.resolveBattle(p1, p2);
      const loser = result.finalLoserId === p1.id ? p1 : p2;

      expect(loser.status).toBe('dead');
      expect(loser.health).toBe(0);
    });

    test('winner remains alive', () => {
      const p1 = new Player('p1', 'Alice', 10, 10);
      const p2 = new Player('p2', 'Bob', 10, 10);

      const result = CombatResolver.resolveBattle(p1, p2);
      const winner = result.finalWinnerId === p1.id ? p1 : p2;

      expect(winner.status).toBe('alive');
      expect(winner.health).toBe(100);
    });

    test('handles null players gracefully', () => {
      const p1 = new Player('p1', 'Alice', 10, 10);

      expect(CombatResolver.resolveBattle(null, p1)).toBeNull();
      expect(CombatResolver.resolveBattle(p1, null)).toBeNull();
      expect(CombatResolver.resolveBattle(null, null)).toBeNull();
    });

    test('combat result has correct structure', () => {
      const p1 = new Player('p1', 'Alice', 10, 10);
      const p2 = new Player('p2', 'Bob', 10, 10);

      const result = CombatResolver.resolveBattle(p1, p2);

      expect(result).toHaveProperty('type');
      expect(result).toHaveProperty('finalWinnerName');
      expect(result).toHaveProperty('finalLoserName');
      expect(result).toHaveProperty('finalWinnerId');
      expect(result).toHaveProperty('finalLoserId');
      expect(result).toHaveProperty('rounds');
      expect(result).toHaveProperty('messages');
      expect(result).toHaveProperty('finalScore');
      expect(result).toHaveProperty('timestamp');
    });
  });

  describe('combat randomness', () => {
    test('combat can have different winners', () => {
      const p1 = new Player('p1', 'Alice', 10, 10);
      const p2 = new Player('p2', 'Bob', 10, 10);

      const results = [];
      for (let i = 0; i < 20; i++) {
        const player1 = new Player(`p${i}a`, 'Alice', 10, 10);
        const player2 = new Player(`p${i}b`, 'Bob', 10, 10);
        results.push(CombatResolver.resolveBattle(player1, player2));
      }

      const winners = results.map(r => r.finalWinnerId);
      const uniqueWinners = new Set(winners.map(w => w.includes('a') ? 'a' : 'b'));

      // With 20 combats, we should see both types of winners
      expect(uniqueWinners.size).toBeGreaterThan(1);
    });
  });
});
