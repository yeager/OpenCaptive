#ifndef COMBAT_H
#define COMBAT_H

#include "game_state.h"

/* Captive has 6 alien sprite sets (ALIEN1-ALIEN6.PL5).
 * Stats recovered from CAPPO.EXE: HP from DS:0xA1BF, categories from
 * DS:0x9A42, speeds from DS:0xA1A4. Damage derived from category. */
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
    uint16_t respawn_timer;
} Creature;

#define MAX_CREATURES 64

typedef struct {
    Creature creatures[MAX_CREATURES];
    int      num_creatures;
    int      last_attack_damage;
    int      last_attack_target;
    bool     attack_occurred;
    bool     creature_killed;
    bool     level_up_occurred;
} CreatureList;

void combat_init(CreatureList *cl);
void combat_spawn_for_level(CreatureList *cl, const DungeonLevel *lvl,
                            int level_num, uint32_t seed);
void combat_tick(CreatureList *cl, GameState *gs);
bool combat_droid_attack(GameState *gs, CreatureList *cl, int droid_idx);
void combat_interact(GameState *gs, const void *item_db);

#endif
