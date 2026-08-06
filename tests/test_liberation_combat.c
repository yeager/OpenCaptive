#include <assert.h>
#include <stdint.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include "liberation_combat.h"
#include "xp_level.h"

static GameState test_gs_storage;

static void init_test_gs(GameState *gs) {
    memset(gs, 0, sizeof(*gs));
    for (int i = 0; i < 4; i++) {
        gs->droids[i].hp = 100;
        gs->droids[i].hp_max = 100;
        gs->droids[i].energy = 50;
        gs->droids[i].energy_max = 100;
        gs->droids[i].weapons[0] = 13; /* basic KNUCKLE-DUSTER */
        gs->droids[i].weapon_damage = 0x0305;
    }
}

static void test_init(void) {
    LibCombatState cs;
    lib_combat_init(&cs);
    assert(!cs.active);
    assert(cs.enemy_count == 0);
    printf("  PASS init\n");
}

static void test_generate_encounter(void) {
    LibCombatState cs;
    lib_combat_init(&cs);
    lib_combat_generate_encounter(&cs, 42, 1);
    assert(cs.active);
    assert(cs.enemy_count >= 1 && cs.enemy_count <= LIB_COMBAT_MAX_ENEMIES);
    for (int i = 0; i < cs.enemy_count; i++) {
        assert(cs.enemies[i].alive);
        assert(cs.enemies[i].hp > 0);
        assert(cs.enemies[i].hp == cs.enemies[i].hp_max);
    }
    printf("  PASS generate_encounter\n");
}

static void test_deterministic(void) {
    LibCombatState a, b;
    lib_combat_init(&a);
    lib_combat_init(&b);
    lib_combat_generate_encounter(&a, 1234, 2);
    lib_combat_generate_encounter(&b, 1234, 2);
    assert(a.enemy_count == b.enemy_count);
    for (int i = 0; i < a.enemy_count; i++) {
        assert(a.enemies[i].hp == b.enemies[i].hp);
        assert(strcmp(a.enemies[i].name, b.enemies[i].name) == 0);
    }
    printf("  PASS deterministic\n");
}

static void test_extreme_difficulty_is_bounded(void) {
    LibCombatState cs;
    lib_combat_init(&cs);
    lib_combat_generate_encounter(&cs, 123, -1000);
    for (int i = 0; i < cs.enemy_count; i++) {
        assert(cs.enemies[i].hp > 0);
        assert(cs.enemies[i].damage > 0);
    }
    lib_combat_generate_encounter(&cs, 123, 1000000);
    for (int i = 0; i < cs.enemy_count; i++) {
        assert(cs.enemies[i].hp > 0);
        assert(cs.enemies[i].damage > 0);
    }
}

static void test_droid_attack(void) {
    LibCombatState cs;
    GameState *gs = &test_gs_storage;
    init_test_gs(gs);
    lib_combat_init(&cs);
    lib_combat_generate_encounter(&cs, 99, 1);
    int16_t hp_before = cs.enemies[0].hp;
    bool ok = lib_combat_droid_attack(&cs, gs, 0);
    assert(ok);
    assert(cs.enemies[0].hp < hp_before);
    assert(gs->droids[0].energy == 47);
    printf("  PASS droid_attack\n");
}

static void test_energy_required(void) {
    LibCombatState cs;
    GameState *gs = &test_gs_storage;
    init_test_gs(gs);
    gs->droids[0].energy = 2;
    lib_combat_init(&cs);
    lib_combat_generate_encounter(&cs, 99, 1);
    bool ok = lib_combat_droid_attack(&cs, gs, 0);
    assert(!ok);
    printf("  PASS energy_required\n");
}

static void test_equipped_weapon_required(void) {
    LibCombatState cs;
    GameState *gs = &test_gs_storage;
    init_test_gs(gs);
    gs->droids[0].weapons[0] = 0;
    gs->droids[0].weapons[1] = 0;
    gs->droids[0].weapon_damage = UINT16_MAX;
    lib_combat_init(&cs);
    lib_combat_generate_encounter(&cs, 99, 1);
    assert(!lib_combat_droid_attack(&cs, gs, 0));
    assert(gs->droids[0].energy == 50);
}

static void test_invalid_target_does_not_consume_energy(void) {
    LibCombatState cs;
    GameState *gs = &test_gs_storage;
    init_test_gs(gs);
    lib_combat_init(&cs);
    lib_combat_generate_encounter(&cs, 99, 1);

    cs.selected_target = cs.enemy_count;
    assert(!lib_combat_droid_attack(&cs, gs, 0));
    assert(gs->droids[0].energy == 50);
}

static void test_corrupt_enemy_count_cannot_escape_array_bounds(void) {
    LibCombatState cs;
    GameState *gs = &test_gs_storage;
    init_test_gs(gs);
    lib_combat_init(&cs);
    lib_combat_generate_encounter(&cs, 99, 1);
    cs.enemy_count = 1000;
    cs.selected_target = LIB_COMBAT_MAX_ENEMIES;
    assert(!lib_combat_droid_attack(&cs, gs, 0));
    assert(gs->droids[0].energy == 50);
}

static void test_max_weapon_damage_does_not_wrap(void) {
    LibCombatState cs;
    GameState *gs = &test_gs_storage;
    init_test_gs(gs);
    gs->droids[0].weapon_damage = UINT16_MAX;
    lib_combat_init(&cs);
    lib_combat_generate_encounter(&cs, 99, 1);
    cs.enemies[0].hp = 100;
    cs.enemies[0].hp_max = 100;
    assert(lib_combat_droid_attack(&cs, gs, 0));
    assert(cs.enemies[0].hp <= 0);
    assert(!cs.enemies[0].alive);
}

static void test_negative_enemy_defense_does_not_increase_damage(void) {
    LibCombatState cs;
    GameState *gs = &test_gs_storage;
    init_test_gs(gs);
    lib_combat_init(&cs);
    lib_combat_generate_encounter(&cs, 99, 1);
    cs.enemies[0].defense = -100;
    gs->droids[0].weapon_damage = 0x0305;
    int16_t before = cs.enemies[0].hp;
    assert(lib_combat_droid_attack(&cs, gs, 0));
    int normal_damage = before - cs.enemies[0].hp;

    lib_combat_generate_encounter(&cs, 99, 1);
    cs.enemies[0].defense = 0;
    before = cs.enemies[0].hp;
    assert(lib_combat_droid_attack(&cs, gs, 0));
    assert(before - cs.enemies[0].hp == normal_damage);
}

static void test_xp_reward_saturates(void) {
    LibCombatState cs;
    GameState *gs = &test_gs_storage;
    init_test_gs(gs);
    gs->droids[0].xp = XP_MAX - 10U;
    lib_combat_init(&cs);
    lib_combat_generate_encounter(&cs, 99, 1);
    cs.enemies[0].hp = 1;
    cs.enemies[0].hp_max = 1;
    gs->droids[0].weapon_damage = UINT16_MAX;
    assert(lib_combat_droid_attack(&cs, gs, 0));
    assert(gs->droids[0].xp == XP_MAX);
}

static void test_target_advances_after_enemy_is_defeated(void) {
    LibCombatState cs;
    GameState *gs = &test_gs_storage;
    init_test_gs(gs);
    lib_combat_init(&cs);
    lib_combat_generate_encounter(&cs, 99, 1);
    cs.enemy_count = 2;
    cs.enemies[0].hp = 1;
    cs.enemies[0].hp_max = 1;
    cs.enemies[1].alive = true;
    cs.selected_target = 0;
    gs->droids[0].weapon_damage = UINT16_MAX;
    assert(lib_combat_droid_attack(&cs, gs, 0));
    assert(!cs.enemies[0].alive);
    assert(cs.selected_target == 1);
}

static void test_extreme_damage_saturates_hp(void) {
    LibCombatState cs;
    GameState *gs = &test_gs_storage;
    init_test_gs(gs);
    lib_combat_init(&cs);
    lib_combat_generate_encounter(&cs, 99, 1);
    cs.enemies[0].damage = INT16_MAX;
    cs.enemies[0].alive = true;
    lib_combat_enemy_turn(&cs, gs);
    int living = 0;
    for (int i = 0; i < 4; i++)
        if (gs->droids[i].hp > 0) living++;
    assert(living == 3);
}

static void test_turn_counter_saturates(void) {
    LibCombatState cs;
    GameState *gs = &test_gs_storage;
    init_test_gs(gs);
    lib_combat_init(&cs);
    lib_combat_generate_encounter(&cs, 99, 1);
    cs.turn = INT_MAX;
    lib_combat_enemy_turn(&cs, gs);
    assert(cs.turn == INT_MAX);
}

static void test_combat_full_round(void) {
    LibCombatState cs;
    GameState *gs = &test_gs_storage;
    init_test_gs(gs);
    lib_combat_init(&cs);
    lib_combat_generate_encounter(&cs, 77, 1);
    int rounds = 0;
    while (!lib_combat_is_over(&cs, gs) && rounds < 100) {
        for (int d = 0; d < 4; d++) {
            lib_combat_droid_attack(&cs, gs, d);
        }
        if (!lib_combat_is_over(&cs, gs))
            lib_combat_enemy_turn(&cs, gs);
        rounds++;
    }
    assert(lib_combat_is_over(&cs, gs));
    printf("  PASS full_round (%d turns)\n", rounds);
}

int main(void) {
    printf("test_liberation_combat:\n");
    test_init();
    test_generate_encounter();
    test_deterministic();
    test_extreme_difficulty_is_bounded();
    test_droid_attack();
    test_energy_required();
    test_equipped_weapon_required();
    test_invalid_target_does_not_consume_energy();
    test_corrupt_enemy_count_cannot_escape_array_bounds();
    test_max_weapon_damage_does_not_wrap();
    test_negative_enemy_defense_does_not_increase_damage();
    test_xp_reward_saturates();
    test_target_advances_after_enemy_is_defeated();
    test_extreme_damage_saturates_hp();
    test_turn_counter_saturates();
    test_combat_full_round();
    printf("All liberation combat tests passed.\n");
    return 0;
}
