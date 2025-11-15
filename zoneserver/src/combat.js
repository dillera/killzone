/**
 * Combat Resolution Logic
 * 
 * Handles combat between players when collisions occur.
 * Uses simple 50/50 random winner determination.
 */

class CombatResolver {
  /**
   * Resolve a three-round weighted battle between attacker and defender
   * @param {Object} attacker
   * @param {Object} defender
   * @returns {Object}
   */
  static resolveBattle(attacker, defender) {
    if (!attacker || !defender) {
      return null;
    }

    const rounds = [];
    let attackerWins = 0;
    let defenderWins = 0;

    const weightFor = (entity) => {
      const base = entity.health || 1;
      const typeBonus = entity.type === 'player' ? 20 : 0;
      const statusPenalty = entity.status === 'dead' ? -100 : 0;
      return Math.max(1, base + typeBonus + statusPenalty);
    };

    for (let round = 1; round <= 3; round++) {
      const attackerWeight = weightFor(attacker);
      const defenderWeight = weightFor(defender);
      const total = attackerWeight + defenderWeight;
      const roll = Math.random() * total;
      const roundWinner = roll < attackerWeight ? attacker : defender;
      const roundLoser = roundWinner === attacker ? defender : attacker;

      if (roundWinner === attacker) {
        attackerWins++;
      } else {
        defenderWins++;
      }

      rounds.push({
        round,
        winnerId: roundWinner.id,
        message: `Round ${round}: ${roundWinner.name} hits ${roundLoser.name}`
      });
    }

    const finalWinner = attackerWins >= defenderWins ? attacker : defender;
    const finalLoser = finalWinner === attacker ? defender : attacker;

    finalLoser.setStatus('dead');
    finalLoser.setHealth(0);

    return {
      type: 'combat',
      attackerId: attacker.id,
      defenderId: defender.id,
      rounds,
      finalWinnerId: finalWinner.id,
      finalLoserId: finalLoser.id,
      finalWinnerName: finalWinner.name,
      finalLoserName: finalLoser.name,
      finalScore: `${attackerWins}-${defenderWins}`,
      messages: [
        ...rounds.map(r => r.message),
        `${finalWinner.name} defeats ${finalLoser.name} (${attackerWins}-${defenderWins})`
      ],
      timestamp: Date.now()
    };
  }
}

module.exports = CombatResolver;
