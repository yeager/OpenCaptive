#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "liberation_combat.h"

static GameState test_gs_storage;

static void init_test_gs(GameState *gs) {
    memset(gs, 0, sizeof(*gs));
    for (int i = 0; i < 4; i++) {
        gs->droids[i].hp = 100;
        gs->droids[i].hp_max = 100;
        gs->droids[i].energy = 50;
        gs->droids[i].energy_max = 100;
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
    test_droid_attack();
    test_energy_required();
    test_combat_full_round();
    printf("All liberation combat tests passed.\n");
    return 0;
}
