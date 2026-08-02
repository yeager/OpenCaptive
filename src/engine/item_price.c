#include "item_price.h"

const ItemVariant item_variants[ITEM_VARIANT_COUNT] = {
    { 0x30,   19, ""    },  /*  0: base ranged (1.9) */
    { 0x21,   20, ""    },  /*  1: melee tier 1 */
    { 0x21,   30, ""    },  /*  2: melee tier 2 (3C) */
    { 0x21,   50, ""    },  /*  3: melee tier 3 */
    { 0x21,   70, ""    },  /*  4: melee tier 4 */
    { 0x21,  140, ""    },  /*  5: melee tier 5 (14A) */
    { 0x30,  140, ""    },  /*  6: ranged tier 1 (14B) */
    { 0x30,  270, ""    },  /*  7: ranged tier 2 */
    { 0x30,  230, ""    },  /*  8: ranged tier 3 */
    { 0x30,  331, ""    },  /*  9: ranged tier 4 (33.1) */
    { 0x30,  560, ""    },  /* 10: ranged tier 5 */
    { 0x30,  780, ""    },  /* 11: ranged tier 6 */
    { 0x30,  990, ""    },  /* 12: ranged tier 7 */
    { 0x30, 1110, ""    },  /* 13: ranged tier 8 */
    { 0x30, 1410, ""    },  /* 14: ranged tier 9 */
    { 0x30, 1658, ""    },  /* 15: ranged tier 10 (165.8) */
    { 0x30, 1800, ""    },  /* 16: ranged tier 11 */
    { 0x30, 2000, ""    },  /* 17: ranged tier 12 */
    { 0x30, 2110, ""    },  /* 18: ranged tier 13 */
    { 0x30, 2310, ""    },  /* 19: ranged tier 14 */
    { 0x30,  120, "A12" },  /* 20: named variant */
    { 0x30,  220, "L22" },  /* 21: named variant */
    { 0x30,  420, "X42" },  /* 22: named variant */
};

static uint16_t rol16(uint16_t val, int count) {
    return (val << count) | (val >> (16 - count));
}

uint16_t item_compute_price(uint16_t game_level_hi, uint16_t game_level_lo,
                            uint16_t base_val, uint8_t shop_tier,
                            uint16_t difficulty) {
    if (game_level_hi == 0 && game_level_lo <= 5) {
        return 0x80;
    }

    uint16_t ax = game_level_hi;
    ax = rol16(ax, 4);
    ax += game_level_lo;
    ax += base_val;
    ax += shop_tier;
    ax = rol16(ax, 4);
    ax += difficulty;
    return ax;
}

uint8_t item_check_availability(uint8_t grade, uint16_t difficulty,
                                uint8_t shop_material) {
    if (grade == 0) return 1;

    uint16_t ax = difficulty;
    if (ax > 100) ax = 100;
    ax++;

    if (grade == 1) {
        uint8_t dh = shop_material;
        if (dh > 0) dh--;
        ax >>= 1;
        if ((uint8_t)ax <= dh) return 0;
        if (dh > 0) ax--;
        ax -= dh;
        if ((uint8_t)ax > 9) ax = 9;
        return (grade > 0) ? (uint8_t)ax : 0;
    }

    if (grade == shop_material) {
        uint8_t dl = grade - 2;
        if ((uint8_t)ax > dl) return 0;
        return (uint8_t)ax;
    }

    uint8_t dh1 = shop_material + 1;
    if (grade == dh1 || grade == dh1 + 1) {
        uint8_t dl = grade + 1;
        if ((uint8_t)ax > dl) return 0;
        return (uint8_t)ax;
    }

    return 0;
}
