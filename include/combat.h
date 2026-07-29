#ifndef COMBAT_H
#define COMBAT_H

#include "game_state.h"

// Creature types matching Captive's encounter system
typedef enum {
    CREATURE_NONE = 0,
    CREATURE_DRONE,
    CREATURE_GUARD,
    CREATURE_TURRET,
    CREATURE_ROBOT,
    CREATURE_ENFORCER,
    CREATURE_BOSS,
    CREATURE_COUNT,
} CreatureType;

typedef struct {
    CreatureType type;
    int16_t hp;
    int16_t hp_max;
    int16_t damage_min;
    int16_t damage_max;
    int16_t defense;
    uint8_t speed;      // attack cooldown in ticks
    uint8_t range;      // attack range in cells
    int     x, y;       // map position
    int     level;      // dungeon level
    uint8_t cooldown;   // current attack cooldown
    bool    active;
    bool    alerted;
} Creature;

#define MAX_CREATURES 64

typedef struct {
    Creature creatures[MAX_CREATURES];
    int      num_creatures;
} CreatureList;

void combat_init(CreatureList *cl);
void combat_spawn_for_level(CreatureList *cl, const DungeonLevel *lvl,
                            int level_num, uint32_t seed);
void combat_tick(CreatureList *cl, GameState *gs);
bool combat_droid_attack(GameState *gs, CreatureList *cl, int droid_idx);
void combat_interact(GameState *gs);

#endif
