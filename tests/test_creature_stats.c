#include "creature_stats.h"
#include <assert.h>
#include <stdio.h>

static void test_table_integrity(void) {
    for (int i = 0; i < CREATURE_TYPE_COUNT; i++) {
        assert(creature_types[i].hp_min > 0);
        assert(creature_types[i].hp_max > creature_types[i].hp_min);
        assert(creature_types[i].category <= 7);
    }
    printf("PASS: table_integrity\n");
}

static void test_hp_formula_weakest(void) {
    int hp = creature_calc_hp(1, 0, 128);
    assert(hp >= 1 && hp <= 255);
    printf("PASS: hp_formula_weakest (type 1, diff 0, mod 128 → %d)\n", hp);
}

static void test_hp_formula_strongest(void) {
    int hp = creature_calc_hp(21, 7, 255);
    assert(hp == 255);
    printf("PASS: hp_formula_strongest (type 21, diff 7, mod 255 → %d)\n", hp);
}

static void test_hp_scales_with_difficulty(void) {
    int hp_low = creature_calc_hp(5, 0, 128);
    int hp_high = creature_calc_hp(5, 7, 128);
    assert(hp_high >= hp_low);
    printf("PASS: hp_scales_with_difficulty (%d → %d)\n", hp_low, hp_high);
}

static void test_hp_bounds(void) {
    assert(creature_calc_hp(0, 0, 128) == 6);
    assert(creature_calc_hp(99, 0, 128) == 6);
    int hp = creature_calc_hp(1, 0, 0);
    assert(hp >= 1);
    printf("PASS: hp_bounds\n");
}

static void test_sprite_map(void) {
    assert(creature_sprite_map[0].graphic_id == 0x00);
    assert(creature_sprite_map[1].graphic_id == 0x67);
    assert(creature_sprite_map[7].graphic_id == 0x6a);
    assert(creature_sprite_map[16].graphic_id == 0x6b);
    printf("PASS: sprite_map\n");
}

static void test_category_groups(void) {
    assert(creature_types[0].category == 0);
    assert(creature_types[3].category == 1);
    assert(creature_types[6].category == 2);
    assert(creature_types[9].category == 3);
    assert(creature_types[21].category == 7);
    printf("PASS: category_groups\n");
}

int main(void) {
    test_table_integrity();
    test_hp_formula_weakest();
    test_hp_formula_strongest();
    test_hp_scales_with_difficulty();
    test_hp_bounds();
    test_sprite_map();
    test_category_groups();
    printf("All creature_stats tests passed.\n");
    return 0;
}
