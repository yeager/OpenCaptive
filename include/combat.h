#ifndef COMBAT_H
#define COMBAT_H

#include "game_state.h"
#include "inventory.h"

/* Captive has 6 alien sprite sets (ALIEN1-ALIEN6.PL5).
 * Stats recovered from CAPPO.EXE: HP from DS:0xA1BF, categories from
 * DS:0x9A43, speeds from DS:0xA1A4. Creature attack damage remains a
 * bounded compatibility approximation until its source record is recovered. */
typedef enum {
    CREATURE_NONE = 0,
    CREATURE_ALIEN1,
    CREATURE_ALIEN2,
    CREATURE_ALIEN3,
    CREATURE_ALIEN4,
    CREATURE_ALIEN5,
    CREATURE_ALIEN6,
    /* Creature IDs 7..25 use the same six source sheets but retain their
     * original type ID for stats, saves, and gameplay. */
    CREATURE_TYPE7,
    CREATURE_TYPE8,
    CREATURE_TYPE9,
    CREATURE_TYPE10,
    CREATURE_TYPE11,
    CREATURE_TYPE12,
    CREATURE_TYPE13,
    CREATURE_TYPE14,
    CREATURE_TYPE15,
    CREATURE_TYPE16,
    CREATURE_TYPE17,
    CREATURE_TYPE18,
    CREATURE_TYPE19,
    CREATURE_TYPE20,
    CREATURE_TYPE21,
    CREATURE_TYPE22,
    CREATURE_TYPE23,
    CREATURE_TYPE24,
    CREATURE_TYPE25,
    CREATURE_COUNT,
} CreatureType;

typedef enum {
    STATUS_NONE    = 0,
    STATUS_POISON  = 1,
    STATUS_STUN    = 2,
    STATUS_DRAIN   = 4,
} StatusEffect;

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
    uint8_t status_attack;
    bool    is_boss;
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
/* Spawn a level's encounters without placing an active creature on the
 * current party cell.  The plain API remains useful for isolated generation
 * tests that do not have a party state. */
void combat_spawn_for_level_avoiding_party(CreatureList *cl,
                                           const DungeonLevel *lvl,
                                           int level_num, uint32_t seed,
                                           const GameState *gs);
bool combat_cell_occupied(const CreatureList *cl, int level, int x, int y);

/* Change Captive floors only when the generated arrival cell is not occupied
 * by an active creature.  A rejected transition leaves the game state intact. */
bool combat_change_floor_if_clear(GameState *gs, CreatureList *cl, int direction);
void combat_tick(CreatureList *cl, GameState *gs);
bool combat_droid_attack(GameState *gs, CreatureList *cl, int droid_idx);
/* Runtime entry point that resolves the equipped weapon whose range can reach
 * the selected target.  The legacy wrapper above remains available for
 * state-only callers and uses the cached damage value. */
bool combat_droid_attack_with_items(GameState *gs, CreatureList *cl,
                                    int droid_idx, const ItemDatabase *db);
/* Register a kill performed by an alternate weapon path (for example a
 * grenade).  This keeps score, respawn, drops, XP and event flags identical
 * to the normal attack path.  The target must already have hp <= 0. */
void combat_register_kill(GameState *gs, CreatureList *cl, Creature *target);
/* Throw the first grenade in the selected droid's inventory at the cell two
 * squares ahead. Dead droids cannot act and therefore do not consume it. */
bool combat_throw_grenade(GameState *gs, CreatureList *cl,
                          const ItemDatabase *db);
void combat_interact(GameState *gs, const void *item_db);

#endif
