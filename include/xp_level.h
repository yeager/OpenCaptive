#ifndef XP_LEVEL_H
#define XP_LEVEL_H

#include <stdint.h>

#define XP_SKILL_COUNT 10
#define XP_MAX         0xF8FFFFFF

/* Per-skill base XP values from CAPPO.EXE at DS:0xB708 (file 0x19EF8).
 * Index matches captive_skill_names order. */
extern const uint16_t xp_skill_base[XP_SKILL_COUNT];

/* Compute XP threshold for a given skill at a given level.
 * Recovered from subroutine at 0xAB92 in CAPPO.EXE. */
uint16_t xp_threshold(int skill_index, int level);

/* Compute XP to award per droid after a kill.
 * xp_value: creature's XP value (from creature_types[].speed or similar)
 * difficulty: current difficulty (0-100, capped at 100)
 * skill_level: the droid's skill byte [di+0x0B]
 * Returns 32-bit XP amount. From subroutine at 0x9621 in CAPPO.EXE. */
uint32_t xp_award(int xp_value, int difficulty, int skill_level);

/* Compute display level from 32-bit XP.
 * level = xp >> 10. From display code at 0xB142. */
uint16_t xp_to_display_level(uint32_t xp);

/* Add XP to accumulator with overflow protection.
 * From subroutine at 0x87EC. */
uint32_t xp_add(uint32_t current_xp, uint32_t amount);

#endif
