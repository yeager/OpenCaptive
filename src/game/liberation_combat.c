#include "liberation_combat.h"
#include <string.h>
#include <stdio.h>

static uint16_t combat_prng(uint16_t *state) {
    *state = (uint16_t)(*state * 0x5E5 + 0x29);
    return *state;
}

static const char *enemy_names[] = {
    "Guard", "Soldier", "Enforcer", "Drone",
    "Sentinel", "Trooper", "Agent", "Warden",
};

void lib_combat_init(LibCombatState *cs) {
    memset(cs, 0, sizeof(*cs));
}

void lib_combat_generate_encounter(LibCombatState *cs, uint16_t seed,
                                    int difficulty) {
    cs->prng_state = seed;
    cs->active = true;
    cs->turn = 0;
    cs->selected_droid = 0;
    cs->selected_target = 0;

    int count = 1 + (combat_prng(&cs->prng_state) % 3);
    if (difficulty > 3) count += 1;
    if (count > LIB_COMBAT_MAX_ENEMIES) count = LIB_COMBAT_MAX_ENEMIES;

    for (int i = 0; i < count; i++) {
        LibCombatEnemy *e = &cs->enemies[i];
        int name_idx = combat_prng(&cs->prng_state) % 8;
        snprintf(e->name, sizeof(e->name), "%s", enemy_names[name_idx]);
        e->hp_max = (int16_t)(20 + difficulty * 10 + (combat_prng(&cs->prng_state) % 20));
        e->hp = e->hp_max;
        e->damage = (int16_t)(5 + difficulty * 3 + (combat_prng(&cs->prng_state) % 8));
        e->defense = (int16_t)(2 + difficulty * 2);
        e->speed = (uint8_t)(3 + (combat_prng(&cs->prng_state) % 4));
        e->alive = true;
    }
    cs->enemy_count = count;
}

bool lib_combat_droid_attack(LibCombatState *cs, GameState *gs, int droid_idx) {
    if (!cs->active || droid_idx < 0 || droid_idx >= 4) return false;

    Droid *d = &gs->droids[droid_idx];
    if (d->hp <= 0 || d->energy < 3) return false;
    d->energy -= 3;

    if (cs->selected_target < 0 || cs->selected_target >= cs->enemy_count)
        return false;
    LibCombatEnemy *target = &cs->enemies[cs->selected_target];
    if (!target->alive) return false;

    uint16_t dmg_word = d->weapon_damage;
    uint8_t lo = dmg_word & 0xFF;
    uint8_t hi = (dmg_word >> 8) & 0xFF;
    int base = (int)lo * (int)hi;
    if (base <= 0) base = 5;
    int damage = base - target->defense / 2;
    if (damage < 1) damage = 1;

    target->hp -= (int16_t)damage;
    if (target->hp <= 0) {
        target->alive = false;
        d->xp += 50;
    }

    return true;
}

void lib_combat_enemy_turn(LibCombatState *cs, GameState *gs) {
    if (!cs->active) return;

    for (int i = 0; i < cs->enemy_count; i++) {
        LibCombatEnemy *e = &cs->enemies[i];
        if (!e->alive) continue;

        int target_idx = combat_prng(&cs->prng_state) % 4;
        Droid *d = &gs->droids[target_idx];
        if (d->hp <= 0) {
            for (int j = 0; j < 4; j++) {
                if (gs->droids[j].hp > 0) { d = &gs->droids[j]; break; }
            }
        }
        if (d->hp <= 0) continue;

        int damage = e->damage - 2;
        if (damage < 1) damage = 1;
        d->hp -= (int16_t)damage;
    }

    cs->turn++;
}

bool lib_combat_is_over(const LibCombatState *cs, const GameState *gs) {
    if (!cs->active) return true;

    bool enemies_alive = false;
    for (int i = 0; i < cs->enemy_count; i++) {
        if (cs->enemies[i].alive) { enemies_alive = true; break; }
    }
    if (!enemies_alive) return true;

    bool droids_alive = false;
    for (int i = 0; i < 4; i++) {
        if (gs->droids[i].hp > 0) { droids_alive = true; break; }
    }
    return !droids_alive;
}

bool lib_combat_player_won(const LibCombatState *cs) {
    for (int i = 0; i < cs->enemy_count; i++) {
        if (cs->enemies[i].alive) return false;
    }
    return true;
}
