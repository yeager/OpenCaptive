#include "xp_level.h"
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

static int failures = 0;
#define ASSERT(cond, msg) do { \
    if (!(cond)) { printf("FAIL: %s\n", msg); failures++; } \
} while(0)

static void test_skill_base_table(void) {
    ASSERT(xp_skill_base[0] == 10, "ROBOTICS base = 10");
    ASSERT(xp_skill_base[1] == 8, "BRAWLING base = 8");
    ASSERT(xp_skill_base[9] == 7960, "EXPERIENCE base = 7960");
}

static void test_threshold_level_zero(void) {
    for (int i = 0; i < XP_SKILL_COUNT; i++) {
        ASSERT(xp_threshold(i, 0) == xp_skill_base[i],
               "threshold at level 0 equals base");
    }
}

static void test_threshold_growth(void) {
    for (int i = 0; i < XP_SKILL_COUNT; i++) {
        uint16_t prev = xp_threshold(i, 0);
        for (int lvl = 1; lvl < 10; lvl++) {
            uint16_t cur = xp_threshold(i, lvl);
            ASSERT(cur > prev, "threshold grows with level");
            prev = cur;
        }
    }
}

static void test_threshold_robotics_cap(void) {
    ASSERT(xp_threshold(0, 0x42) == 0x8A47, "ROBOTICS caps at level 66");
    ASSERT(xp_threshold(0, 0x43) == 0x8A47, "ROBOTICS above cap");
}

static void test_threshold_other_cap(void) {
    ASSERT(xp_threshold(1, 0x18) == 0x8A47, "BRAWLING caps at level 24");
    ASSERT(xp_threshold(5, 0x18) == 0x8A47, "AUTOMATICS caps at level 24");
}

static void test_xp_award(void) {
    uint32_t xp = xp_award(5, 50, 10);
    // dx = 5 + 3 + 50 = 58, m = 10 * 58 = 580, m4 = 2320, m8 = 4640, total = 6960
    ASSERT(xp == 6960, "xp_award basic calculation");
}

static void test_xp_award_zero_difficulty(void) {
    uint32_t xp = xp_award(0, 0, 1);
    // dx = 0 + 3 + 0 = 3, m = 1*3 = 3, m4=12, m8=24, total=36
    ASSERT(xp == 36, "xp_award zero difficulty");
}

static void test_xp_award_capped_difficulty(void) {
    uint32_t a = xp_award(5, 100, 10);
    uint32_t b = xp_award(5, 200, 10);
    ASSERT(a == b, "difficulty capped at 100");
}

static void test_display_level(void) {
    ASSERT(xp_to_display_level(0) == 0, "level 0 at 0 xp");
    ASSERT(xp_to_display_level(1024) == 1, "level 1 at 1024 xp");
    ASSERT(xp_to_display_level(2048) == 2, "level 2 at 2048 xp");
    ASSERT(xp_to_display_level(1023) == 0, "just under level 1");
}

static void test_xp_add_normal(void) {
    ASSERT(xp_add(0, 100) == 100, "add 100 to 0");
    ASSERT(xp_add(500, 500) == 1000, "add 500 to 500");
}

static void test_xp_add_cap(void) {
    ASSERT(xp_add(XP_MAX, 100) == XP_MAX, "already at cap");
    ASSERT(xp_add(XP_MAX - 1, 100) == XP_MAX, "overflow capped");
}

static void test_xp_add_minimum_one(void) {
    ASSERT(xp_add(50, 0) == 51, "zero amount becomes 1");
}

static void test_invalid_xp_inputs_are_safe(void) {
    ASSERT(xp_threshold(0, -1) == 0xFFFF, "negative threshold level rejected");
    ASSERT(xp_award(-1, 10, 1) == 0, "negative XP value rejected");
    ASSERT(xp_award(10, 10, -1) == 0, "negative skill level rejected");
    ASSERT(xp_award(INT_MAX, 100, INT_MAX) == UINT32_MAX,
           "XP award saturates instead of overflowing");
}

int main(void) {
    test_skill_base_table();
    test_threshold_level_zero();
    test_threshold_growth();
    test_threshold_robotics_cap();
    test_threshold_other_cap();
    test_xp_award();
    test_xp_award_zero_difficulty();
    test_xp_award_capped_difficulty();
    test_display_level();
    test_xp_add_normal();
    test_xp_add_cap();
    test_xp_add_minimum_one();
    test_invalid_xp_inputs_are_safe();
    if (failures) {
        printf("%d test(s) FAILED\n", failures);
        return 1;
    }
    printf("All xp_level tests passed\n");
    return 0;
}
