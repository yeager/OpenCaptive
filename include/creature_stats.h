#ifndef CREATURE_STATS_H
#define CREATURE_STATS_H

#include <stdint.h>

/* Creature stat tables recovered from CAPPO.EXE v1.06 (unpacked).
 * SHA-256: fa7d5ca76d26f614476ed41f27cf737084942e9216b20b4605734df9ede9aee4
 *
 * HP table at DS:0xA1BF (file offset 0x189AF): 4 bytes per type (min_hp, max_hp).
 * Category table at DS:0x9A42 (file 0x18232): 1 byte per type (0-7 group ID).
 * Speed table at DS:0xA1A4 (file 0x18994): 1 byte per type.
 *
 * HP formula from 0x9B12:
 *   base = min + ((max - min) * difficulty) / 8
 *   hp = ((base * 2 * creature_modifier) >> 8) + 6
 */

#define CREATURE_TYPE_COUNT 25

typedef struct {
    uint16_t hp_min;
    uint16_t hp_max;
    uint8_t  category;
    uint8_t  speed;
} CreatureTypeDef;

extern const CreatureTypeDef creature_types[CREATURE_TYPE_COUNT];

/* Sprite assignments from DS:0xA16E (file 0x1895E).
 * 2 bytes per entry: graphic_id, frame_index. */
#define CREATURE_SPRITE_ENTRIES (CREATURE_TYPE_COUNT + 1)

typedef struct {
    uint8_t graphic_id;
    uint8_t frame_index;
} CreatureSpriteEntry;

extern const CreatureSpriteEntry creature_sprite_map[CREATURE_SPRITE_ENTRIES];

/* Resolve the disassembly's graphic_id to the corresponding ALIEN1-6.PL5
 * source sheet.  IDs outside the six creature sheets are intentionally
 * reported as -1 so callers use their fallback renderer. */
int creature_sprite_sheet_index(int creature_type);
int creature_sprite_frame_index(int creature_type);

int creature_calc_hp(int creature_type, int difficulty, int modifier);

#endif
