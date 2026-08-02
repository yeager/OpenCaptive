#ifndef COMBAT_H
#define COMBAT_H

#include "game_state.h"

/* Captive has 6 alien sprite sets (ALIEN1-ALIEN6.PL5).
 * The exact creature stats and combat formulas are not yet recovered
 * from the DOS executable. All stat values below are placeholders. */
typedef enum {
    CREATURE_NONE = 0,
    CREATURE_ALIEN1,
    CREATURE_ALIEN2,
    CREATURE_ALIEN3,
    CREATURE_ALIEN4,
    CREATURE_ALIEN5,
    CREATURE_ALIEN6,
    CREATURE_COUNT,
} CreatureType;

typedef struct {
    CreatureType type;
    int16_t hp;
    int16_t hp_max;
    int16_t damage_min;
    int16_t damage_max;
    int16_t defense;
    uint8_t speed;
    uint8_t range;
    int     x, y;
    int     level;
    uint8_t cooldown;
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
