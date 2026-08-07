#ifndef SPAWN_H
#define SPAWN_H

#include "game_state.h"
#include "combat.h"

#define SPAWN_CATEGORY_COUNT 8
#define SPAWN_TYPES_PER_CAT  3
#define SPAWN_TYPE_TABLE_COUNT 25
#define SPAWN_SUBCELL_COUNT  16

typedef struct {
    uint8_t types[SPAWN_TYPES_PER_CAT];
} SpawnCategory;

extern const SpawnCategory spawn_categories[SPAWN_CATEGORY_COUNT];
extern const uint8_t spawn_difficulty_offset[SPAWN_TYPE_TABLE_COUNT];
extern const uint8_t spawn_modifier[SPAWN_TYPE_TABLE_COUNT];
extern const uint8_t spawn_subcell_table1[SPAWN_SUBCELL_COUNT];
extern const uint8_t spawn_subcell_table2[SPAWN_SUBCELL_COUNT];

typedef struct {
    uint8_t creature_type;
    uint8_t subcell;
    uint8_t modifier;
    uint16_t hp;
} SpawnEntry;

#define MAX_SPAWN_ENTRIES 6

typedef struct {
    SpawnEntry entries[MAX_SPAWN_ENTRIES];
    int count;
} SpawnResult;

uint8_t spawn_select_type(int category, int difficulty, uint32_t *prng);
uint8_t spawn_subcell_from_direction(uint8_t direction, uint8_t position);
uint16_t spawn_compute_hp(uint8_t creature_type, int difficulty, uint8_t modifier);
SpawnResult spawn_creatures(int category, int difficulty, uint8_t direction,
                            uint8_t position, uint32_t *prng);

#endif
