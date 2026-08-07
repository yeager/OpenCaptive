#include "spawn.h"
#include "captive_data.h"
#include "creature_stats.h"

const SpawnCategory spawn_categories[SPAWN_CATEGORY_COUNT] = {
    /* CAPPO.EXE DS:0x9A42: creature types are 1-based; types 1-24
     * form eight groups of three. Type 25 is the extra special entry. */
    {{ 1,  2,  3}},
    {{ 4,  5,  6}},
    {{ 7,  8,  9}},
    {{10, 11, 12}},
    {{13, 14, 15}},
    {{16, 17, 18}},
    {{19, 20, 21}},
    {{22, 23, 24}},
};

const uint8_t spawn_difficulty_offset[SPAWN_TYPE_TABLE_COUNT] = {
    7, 0, 8, 16, 0, 8, 16, 0,
    8, 16, 0, 8, 16, 0, 8, 16,
    0, 8, 16, 0, 8, 16, 0, 8, 16,
};

const uint8_t spawn_modifier[SPAWN_TYPE_TABLE_COUNT] = {
    2, 4, 12, 30, 23, 60, 110, 90,
    16, 18, 16, 12, 10, 12, 4, 0,
    4, 8, 20, 8, 4, 2, 12, 4, 6,
};

const uint8_t spawn_subcell_table1[SPAWN_SUBCELL_COUNT] = {
    0x00, 0x06, 0x08, 0x02, 0x02, 0x00, 0x06, 0x08,
    0x06, 0x08, 0x02, 0x00, 0x08, 0x02, 0x00, 0x06,
};

const uint8_t spawn_subcell_table2[SPAWN_SUBCELL_COUNT] = {
    0x01, 0x03, 0x07, 0x05, 0x00, 0x01, 0x03, 0x04,
    0x01, 0x02, 0x04, 0x05, 0x03, 0x04, 0x06, 0x07,
};

uint8_t spawn_select_type(int category, int difficulty, uint32_t *prng) {
    (void)difficulty;
    if (category < 0 || category >= SPAWN_CATEGORY_COUNT) return 0xFF;
    if (!prng) return 0xFF;
    uint8_t type_idx = spawn_categories[category].types[captive_prng(prng) % SPAWN_TYPES_PER_CAT];
    return type_idx;
}

uint8_t spawn_subcell_from_direction(uint8_t direction, uint8_t position) {
    if (direction > 3) return 0;
    if (direction <= 1) {
        uint8_t idx = (direction << 2) + (position & 3);
        if (idx < SPAWN_SUBCELL_COUNT) return spawn_subcell_table1[idx];
    }
    uint8_t idx = position;
    if (idx < SPAWN_SUBCELL_COUNT) return spawn_subcell_table2[idx];
    return 0;
}

uint16_t spawn_compute_hp(uint8_t creature_type, int difficulty, uint8_t mod) {
    /* Creature IDs in the spawn record are 1-based (type 1..25), while
     * creature_types[] is a C array indexed 0..24. */
    if (creature_type == 0 || creature_type > CREATURE_TYPE_COUNT) return 6;
    if (difficulty < 0) difficulty = 0;
    if (difficulty > 8) difficulty = 8;
    const CreatureTypeDef *def = &creature_types[creature_type - 1];
    uint16_t range = def->hp_max - def->hp_min;
    uint32_t base = (uint32_t)difficulty * range;
    base >>= 3;
    base += def->hp_min;
    base <<= 1;
    base *= mod;
    base >>= 8;
    base += 6;
    if (base > 255) base = 255;
    return (uint16_t)base;
}

SpawnResult spawn_creatures(int category, int difficulty, uint8_t direction,
                            uint8_t position, uint32_t *prng) {
    SpawnResult result = {{{0}}, 0};
    if (!prng || direction > 3) return result;
    /* Callers use the zero-based dungeon level as difficulty.  The previous
     * implementation decremented it here as if the API accepted a one-based
     * level, so level 0 and level 1 generated identical HP and the final
     * dungeon level was always one difficulty step too weak. */
    if (difficulty < 0) difficulty = 0;
    if (difficulty > 8) difficulty = 8;

    uint8_t type = spawn_select_type(category, difficulty, prng);
    if (type == 0xFF) return result;

    /* CAPPO.EXE keeps one table entry per creature ID. The old fallback to
     * entry 0 made creature types 16-24 use the wrong HP modifier. */
    uint8_t mod = type < SPAWN_TYPE_TABLE_COUNT ? spawn_modifier[type] : 0;
    uint8_t base_subcell = spawn_subcell_from_direction(direction, position);

    if (type < 0x0A) {
        if (result.count < MAX_SPAWN_ENTRIES) {
        SpawnEntry *e = &result.entries[result.count++];
        e->creature_type = type;
        e->subcell = base_subcell;
        e->modifier = mod;
        e->hp = spawn_compute_hp(type, difficulty, mod);
        }
    } else if (type <= 0x0B) {
        if (result.count < MAX_SPAWN_ENTRIES) {
        SpawnEntry *e = &result.entries[result.count++];
        e->creature_type = type;
        e->subcell = base_subcell | 0x20;
        e->modifier = mod;
        e->hp = spawn_compute_hp(type, difficulty, mod);
        }
    } else if (type == 0x0C) {
        if (result.count < MAX_SPAWN_ENTRIES) {
        SpawnEntry *e0 = &result.entries[result.count++];
        e0->creature_type = type;
        e0->subcell = base_subcell | 0x20;
        e0->modifier = mod;
        e0->hp = spawn_compute_hp(type, difficulty, mod);
        }
        if (result.count < MAX_SPAWN_ENTRIES) {
        SpawnEntry *e1 = &result.entries[result.count++];
        e1->creature_type = type;
        e1->subcell = (base_subcell + 1) | 0x20;
        e1->modifier = mod;
        e1->hp = spawn_compute_hp(type, difficulty, mod);
        }
    } else if (type <= 0x0E) {
        for (int i = 0; i < 3 && result.count < MAX_SPAWN_ENTRIES; i++) {
            SpawnEntry *e = &result.entries[result.count++];
            e->creature_type = type;
            e->subcell = base_subcell + i;
            e->modifier = mod;
            e->hp = spawn_compute_hp(type, difficulty, mod);
        }
    } else if (type == 0x0F) {
        if (result.count < MAX_SPAWN_ENTRIES) {
        SpawnEntry *e0 = &result.entries[result.count++];
        e0->creature_type = type;
        e0->subcell = base_subcell;
        e0->modifier = mod;
        e0->hp = spawn_compute_hp(type, difficulty, mod);
        }
        uint8_t opp_dir = direction ^ 0x02;
        uint8_t opp_sub = spawn_subcell_from_direction(opp_dir, position);
        if (result.count < MAX_SPAWN_ENTRIES) {
        SpawnEntry *e1 = &result.entries[result.count++];
        e1->creature_type = type;
        e1->subcell = opp_sub;
        e1->modifier = mod;
        e1->hp = spawn_compute_hp(type, difficulty, mod);
        }
        uint8_t perp_dir = (~direction) & 0x01;
        uint8_t perp_sub = spawn_subcell_from_direction(perp_dir, position);
        if (result.count < MAX_SPAWN_ENTRIES) {
        SpawnEntry *e2 = &result.entries[result.count++];
        e2->creature_type = type;
        e2->subcell = perp_sub;
        e2->modifier = mod;
        e2->hp = spawn_compute_hp(type, difficulty, mod);
        }
    } else if (type == 0x15) {
        if (result.count < MAX_SPAWN_ENTRIES) {
        SpawnEntry *e0 = &result.entries[result.count++];
        e0->creature_type = type;
        e0->subcell = base_subcell;
        e0->modifier = mod;
        e0->hp = spawn_compute_hp(type, difficulty, mod);
        }
        if (result.count < MAX_SPAWN_ENTRIES) {
        SpawnEntry *e1 = &result.entries[result.count++];
        e1->creature_type = type;
        e1->subcell = base_subcell | 0x20;
        e1->modifier = mod;
        e1->hp = spawn_compute_hp(type, difficulty, mod);
        }
    } else {
        if (result.count < MAX_SPAWN_ENTRIES) {
        SpawnEntry *e = &result.entries[result.count++];
        e->creature_type = type;
        e->subcell = base_subcell;
        e->modifier = mod;
        e->hp = spawn_compute_hp(type, difficulty, mod);
        }
    }

    return result;
}
