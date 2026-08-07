# Combat System

> Updated for v1.1.79. Combat documentation reflects the current tested prototype boundary.

## Overview

OpenCaptive's combat system implements Captive's real-time creature encounters with AI behavior, damage calculation, and experience progression.

## Creatures

25 creature types grouped into 8 categories (0-7). Per-type HP/category/speed tables recovered from CAPPO.EXE (DS:0xA1BF, DS:0x9A42, DS:0xA1A4).

### Damage formula (from CAPPO.EXE at 0x5380)

Creature damage is procedurally computed at spawn time, not stored in a per-type table:
```
base = min(20, 2 + category + dungeon_level)
dmg_lo = (base >> 1) | 1
dmg_hi = base
damage_min = dmg_lo * dmg_hi
damage_max = dmg_lo * dmg_hi + dmg_hi
```
Uses the same lo*hi byte encoding as weapon damage (formula at 0x97F2: `mul ah`, shift ×8, cap 0xFFFD).

Defense scales with category and level: `category * 2 + level`.
Range: categories 0-3 are melee (1-2), categories 4-7 are ranged (4-7).

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
- Range check: the equipped weapon's range versus Manhattan distance to the
  nearest creature; spray weapons use range 4 while the other ranged weapons
  use range 6
- Cooldown between attacks based on weapon speed

## Experience and Leveling

- XP per kill follows the recovered CAPPO formula:
  `skill_level * (creature_xp + 3 + difficulty) * 12`
- Captive uses the creature's recovered speed/XP value and the droid's
  `Experience` skill for this calculation.
- Display level is `xp >> 10`; crossing a display level restores HP and
  energy and increases their maximums.

## Spawning

Creatures spawn based on level difficulty:
- Number: `3 + level * 2` creatures per dungeon level
- Type distribution shifts toward harder creatures at higher levels
- Placed in rooms during map generation, avoiding party spawn area
