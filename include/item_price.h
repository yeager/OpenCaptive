#ifndef ITEM_PRICE_H
#define ITEM_PRICE_H

#include <stdint.h>

#define ITEM_VARIANT_COUNT 23
#define ITEM_VARIANT_MELEE_START 1
#define ITEM_VARIANT_MELEE_END   5
#define ITEM_VARIANT_RANGED_START 6
#define ITEM_VARIANT_RANGED_END   19

typedef struct {
    uint8_t type_code;
    uint16_t price;
    char name[8];
} ItemVariant;

extern const ItemVariant item_variants[ITEM_VARIANT_COUNT];

/* Compute shop item price from CAPPO.EXE formula at 0xBB63.
 * game_level: combined 32-bit level value (hi = [0x8D13], lo = [0x8D11])
 * base_val: per-item base from item record
 * shop_tier: material/grade modifier from [0x8D03]
 * difficulty: current difficulty (capped at 100) */
uint16_t item_compute_price(uint16_t game_level_hi, uint16_t game_level_lo,
                            uint16_t base_val, uint8_t shop_tier,
                            uint16_t difficulty);

/* Item availability check from CAPPO.EXE at 0xB8D6.
 * Returns the grade/tier of item available at given difficulty. */
uint8_t item_check_availability(uint8_t grade, uint16_t difficulty,
                                uint8_t shop_material);

#endif
