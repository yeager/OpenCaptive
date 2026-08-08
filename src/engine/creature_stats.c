#include "creature_stats.h"

const CreatureTypeDef creature_types[CREATURE_TYPE_COUNT] = {
    /*          hp_min  hp_max  cat  spd */
    {    10,    150,   0,    0},  /* type  1 */
    {   200,    400,   0,    5},  /* type  2 */
    {   400,    600,   0,    7},  /* type  3 */
    {   600,    800,   1,   10},  /* type  4 */
    {   800,   1000,   1,   16},  /* type  5 */
    {  1000,   1500,   1,   20},  /* type  6 */
    {  1000,   1700,   2,   30},  /* type  7 */
    {  1700,   2400,   2,   20},  /* type  8 */
    {  2400,   3000,   2,   22},  /* type  9 */
    {  3000,   3700,   3,   24},  /* type 10 */
    {  3700,   4400,   3,   24},  /* type 11 */
    {  4000,   4900,   3,   40},  /* type 12 */
    {  3000,   3700,   4,   32},  /* type 13 */
    {  3700,   4400,   4,   36},  /* type 14 */
    {  4400,   5700,   4,   46},  /* type 15 */
    {  9000,  10000,   5,   56},  /* type 16 */
    { 10000,  12000,   5,   20},  /* type 17 */
    { 12000,  15000,   5,   24},  /* type 18 */
    { 15000,  17000,   6,   26},  /* type 19 */
    { 17000,  19000,   6,   25},  /* type 20 */
    { 19000,  22000,   6,   30},  /* type 21 */
    {  1000,   3000,   7,   40},  /* type 22 */
    {  3000,   5000,   7,   25},  /* type 23 */
    {  5000,   7000,   7,   30},  /* type 24 */
    {  1000,   1100,   0,   30},  /* type 25 */
};

const CreatureSpriteEntry creature_sprite_map[CREATURE_SPRITE_ENTRIES] = {
    {0x00, 0x00},  /* type  0 (null) */
    {0x67, 0x20},  /* type  1 */
    {0x67, 0x20},  /* type  2 */
    {0x67, 0x20},  /* type  3 */
    {0x67, 0x20},  /* type  4 */
    {0x67, 0x20},  /* type  5 */
    {0x67, 0x20},  /* type  6 */
    {0x6a, 0x15},  /* type  7 */
    {0x6a, 0x15},  /* type  8 */
    {0x6a, 0x15},  /* type  9 */
    {0x6a, 0x16},  /* type 10 */
    {0x6a, 0x16},  /* type 11 */
    {0x6a, 0x16},  /* type 12 */
    {0x6a, 0x01},  /* type 13 */
    {0x6a, 0x01},  /* type 14 */
    {0x6a, 0x01},  /* type 15 */
    {0x6b, 0x0f},  /* type 16 */
    {0x6b, 0x0f},  /* type 17 */
    {0x6b, 0x0f},  /* type 18 */
    {0x6c, 0x17},  /* type 19 */
    {0x6b, 0x17},  /* type 20 */
    {0x6c, 0x17},  /* type 21 */
    {0x6d, 0x10},  /* type 22 */
    {0x6d, 0x10},  /* type 23 */
    {0x6e, 0x10},  /* type 24 */
    {0x60, 0x00},  /* type 25: non-ALIEN graphic; no PL5 frame */
};

int creature_sprite_sheet_index(int creature_type) {
    if (creature_type < 1 || creature_type > CREATURE_TYPE_COUNT)
        return -1;

    uint8_t graphic_id = creature_sprite_map[creature_type].graphic_id;
    switch (graphic_id) {
    case 0x67: return 0; /* ALIEN1.PL5 */
    case 0x6A: return 1; /* ALIEN2.PL5 */
    case 0x6B: return 2; /* ALIEN3.PL5 */
    case 0x6C: return 3; /* ALIEN4.PL5 */
    case 0x6D: return 4; /* ALIEN5.PL5 */
    case 0x6E: return 5; /* ALIEN6.PL5 */
    default:   return -1;
    }
}

int creature_sprite_frame_index(int creature_type) {
    if (creature_type < 1 || creature_type > CREATURE_TYPE_COUNT)
        return -1;
    return creature_sprite_map[creature_type].frame_index;
}

int creature_calc_hp(int creature_type, int difficulty, int modifier) {
    if (creature_type < 1 || creature_type > CREATURE_TYPE_COUNT)
        return 6;
    const CreatureTypeDef *def = &creature_types[creature_type - 1];
    if (difficulty < 0) difficulty = 0;
    /* Recovered formula at 0x9B12 uses an eight-step difficulty scale:
     * base = min + ((max - min) * difficulty) / 8.  Keep this helper
     * aligned with spawn_compute_hp(), including the highest dungeon level. */
    if (difficulty > 8) difficulty = 8;

    int range = def->hp_max - def->hp_min;
    int base = def->hp_min + (range * difficulty) / 8;
    base *= 2;

    int result = (base * modifier) >> 8;
    result += 6;
    if (result > 255) result = 255;
    if (result < 1) result = 1;
    return result;
}
