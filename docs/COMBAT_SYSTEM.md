# Combat System

## Overview

OpenCaptive's combat system implements Captive's real-time creature encounters with AI behavior, damage calculation, and experience progression.

## Creatures

| Type | HP | Damage | Defense | Speed | Range | Notes |
|------|-----|--------|---------|-------|-------|-------|
| Drone | 15-25 | 3-6 | 1 | 8 | 1 | Weakest, early levels |
| Guard | 30-50 | 5-10 | 3 | 6 | 1 | Standard melee |
| Turret | 20-35 | 8-15 | 5 | 10 | 3 | Ranged, stationary |
| Robot | 50-80 | 10-18 | 6 | 5 | 1 | Tanky melee |
| Enforcer | 60-100 | 12-22 | 8 | 4 | 2 | Mid-boss |
| Boss | 100-200 | 15-30 | 10 | 3 | 2 | Level boss |

Stats scale with dungeon level. Base HP multiplied by `1 + level * 0.3`.

## AI Behavior

1. **Idle**: creature waits at spawn position
2. **Alert**: player enters detection range (varies by type)
3. **Chase**: creature moves toward player each AI tick
4. **Attack**: creature is adjacent to player and cooldown expired

AI ticks occur every 4 game ticks (combat_tick). Movement uses Manhattan distance pathfinding toward the player's position.

## Droid Attacks

- Attack key (Space) fires selected droid's equipped weapon
- Damage = weapon base damage + random range + skill bonus
- Defense reduces damage: `effective = max(1, damage - target.defense)`
- Range check: weapon range vs Manhattan distance to nearest creature
- Cooldown between attacks based on weapon speed

## Experience and Leveling

- XP awarded per kill: `creature.hp_max / 2`
- Level up threshold: `level * 100`
- On level up: HP+10, energy+5, damage+1

## Spawning

Creatures spawn based on level difficulty:
- Number: `3 + level * 2` creatures per dungeon level
- Type distribution shifts toward harder creatures at higher levels
- Placed in rooms during map generation, avoiding party spawn area
