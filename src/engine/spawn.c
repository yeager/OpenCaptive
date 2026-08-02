#include "spawn.h"
#include "captive_data.h"
#include "creature_stats.h"

const SpawnCategory spawn_categories[SPAWN_CATEGORY_COUNT] = {
    {{13, 0, 0}},
    {{ 0, 1, 1}},
    {{ 1, 2, 2}},
    {{ 2, 3, 3}},
    {{ 3, 4, 4}},
    {{ 4, 5, 5}},
    {{ 5, 6, 6}},
    {{ 6, 7, 7}},
};

const uint8_t spawn_difficulty_offset[16] = {
    7, 0, 8, 16, 0, 8, 16, 0,
    8, 16, 0, 8, 16, 0, 8, 16,
};

const uint8_t spawn_modifier[16] = {
    2, 4, 12, 30, 23, 60, 110, 90,
    16, 18, 16, 12, 10, 12, 4, 0,
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
    if (category < 0 || category >= SPAWN_CATEGORY_COUNT) return 0xFF;
    uint8_t type_idx = spawn_categories[category].types[captive_prng(prng) % SPAWN_TYPES_PER_CAT];
    return type_idx;
}

uint8_t spawn_subcell_from_direction(uint8_t direction, uint8_t position) {
    if (direction <= 1) {
        uint8_t idx = (direction << 2) + (position & 3);
        if (idx < SPAWN_SUBCELL_COUNT) return spawn_subcell_table1[idx];
    }
    uint8_t idx = position;
    if (idx < SPAWN_SUBCELL_COUNT) return spawn_subcell_table2[idx];
    return 0;
}

uint16_t spawn_compute_hp(uint8_t creature_type, int difficulty, uint8_t mod) {
    if (creature_type == 0 || creature_type >= CREATURE_TYPE_COUNT) return 6;
    const CreatureTypeDef *def = &creature_types[creature_type];
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
    if (difficulty > 8) difficulty = 8;
    difficulty--;
    if (difficulty < 0) difficulty = 0;

    uint8_t type = spawn_select_type(category, difficulty, prng);
    if (type == 0xFF) return result;

    uint8_t mod = spawn_modifier[type < 16 ? type : 0];
    uint8_t base_subcell = spawn_subcell_from_direction(direction, position);

    if (type < 0x0A) {
        SpawnEntry *e = &result.entries[result.count++];
        e->creature_type = type;
        e->subcell = base_subcell;
        e->modifier = mod;
        e->hp = spawn_compute_hp(type, difficulty, mod);
    } else if (type <= 0x0B) {
        SpawnEntry *e = &result.entries[result.count++];
        e->creature_type = type;
        e->subcell = base_subcell | 0x20;
        e->modifier = mod;
        e->hp = spawn_compute_hp(type, difficulty, mod);
    } else if (type == 0x0C) {
        SpawnEntry *e0 = &result.entries[result.count++];
        e0->creature_type = type;
        e0->subcell = base_subcell | 0x20;
        e0->modifier = mod;
        e0->hp = spawn_compute_hp(type, difficulty, mod);

        SpawnEntry *e1 = &result.entries[result.count++];
        e1->creature_type = type;
        e1->subcell = (base_subcell + 1) | 0x20;
        e1->modifier = mod;
        e1->hp = spawn_compute_hp(type, difficulty, mod);
    } else if (type <= 0x0E) {
        for (int i = 0; i < 3 && result.count < MAX_SPAWN_ENTRIES; i++) {
            SpawnEntry *e = &result.entries[result.count++];
            e->creature_type = type;
            e->subcell = base_subcell + i;
            e->modifier = mod;
            e->hp = spawn_compute_hp(type, difficulty, mod);
        }
    } else if (type == 0x0F) {
        SpawnEntry *e0 = &result.entries[result.count++];
        e0->creature_type = type;
        e0->subcell = base_subcell;
        e0->modifier = mod;
        e0->hp = spawn_compute_hp(type, difficulty, mod);

        uint8_t opp_dir = direction ^ 0x02;
        uint8_t opp_sub = spawn_subcell_from_direction(opp_dir, position);
        SpawnEntry *e1 = &result.entries[result.count++];
        e1->creature_type = type;
        e1->subcell = opp_sub;
        e1->modifier = mod;
        e1->hp = spawn_compute_hp(type, difficulty, mod);

        uint8_t perp_dir = (~direction) & 0x01;
        uint8_t perp_sub = spawn_subcell_from_direction(perp_dir, position);
        SpawnEntry *e2 = &result.entries[result.count++];
        e2->creature_type = type;
        e2->subcell = perp_sub;
        e2->modifier = mod;
        e2->hp = spawn_compute_hp(type, difficulty, mod);
    } else if (type == 0x15) {
        SpawnEntry *e0 = &result.entries[result.count++];
        e0->creature_type = type;
        e0->subcell = base_subcell;
        e0->modifier = mod;
        e0->hp = spawn_compute_hp(type, difficulty, mod);

        SpawnEntry *e1 = &result.entries[result.count++];
        e1->creature_type = type;
        e1->subcell = base_subcell | 0x20;
        e1->modifier = mod;
        e1->hp = spawn_compute_hp(type, difficulty, mod);
    } else {
        SpawnEntry *e = &result.entries[result.count++];
        e->creature_type = type;
        e->subcell = base_subcell;
        e->modifier = mod;
        e->hp = spawn_compute_hp(type, difficulty, mod);
    }

    return result;
}
