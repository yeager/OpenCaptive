#include "xp_level.h"

const uint16_t xp_skill_base[XP_SKILL_COUNT] = {
    10,    /* ROBOTICS */
    8,     /* BRAWLING */
    30,    /* SWORDS */
    72,    /* HANDGUNS */
    216,   /* RIFLES */
    648,   /* AUTOMATICS */
    1944,  /* LASERS */
    3888,  /* CANNONS */
    7776,  /* SPAYGUNS */
    7960,  /* EXPERIENCE */
};

uint16_t xp_threshold(int skill_index, int level) {
    if (skill_index < 0 || skill_index >= XP_SKILL_COUNT || level < 0)
        return 0xFFFF;

    uint16_t ax = xp_skill_base[skill_index];

    int max_level = (skill_index == 0) ? 0x42 : 0x18;
    if (level >= max_level) return 0x8A47;

    if (skill_index == 0) {
        for (int i = 0; i < level; i++) {
            uint16_t bx = ax >> 3;
            uint16_t sum = ax + bx;
            if (sum < ax) return 0xFFFF;
            sum += 8;
            if (sum < 8) return 0xFFFF;
            ax = sum;
        }
    } else {
        for (int i = 0; i < level; i++) {
            uint16_t bx = ax >> 4;
            if ((ax >> 3) & 1) bx++;
            uint16_t sum = ax + bx;
            if (sum < ax) return 0xFFFF;
            ax = sum;
        }
    }

    return ax;
}

uint32_t xp_award(int xp_value, int difficulty, int skill_level) {
    if (xp_value < 0 || skill_level < 0) return 0;
    if (difficulty > 100) difficulty = 100;
    if (difficulty < 0) difficulty = 0;

    uint64_t dx = (uint64_t)(uint32_t)xp_value + 3u + (uint32_t)difficulty;
    uint64_t multiplier = (uint64_t)(uint32_t)skill_level * dx;
    uint64_t award = multiplier * 12u;
    return award > UINT32_MAX ? UINT32_MAX : (uint32_t)award;
}

uint16_t xp_to_display_level(uint32_t xp) {
    uint32_t shifted = xp >> 10;
    if (shifted > 0xFFFF) return 0xFFFF;
    return (uint16_t)shifted;
}

uint32_t xp_add(uint32_t current_xp, uint32_t amount) {
    if (current_xp >= XP_MAX) return current_xp;
    if (amount == 0) amount = 1;
    uint32_t result = current_xp + amount;
    if (result < current_xp || result > XP_MAX) return XP_MAX;
    return result;
}
