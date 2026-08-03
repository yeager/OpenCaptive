#ifndef LIBERATION_COMBAT_H
#define LIBERATION_COMBAT_H

#include <stdbool.h>
#include <stdint.h>
#include "game_state.h"

#define LIB_COMBAT_MAX_ENEMIES 8

typedef struct {
    char name[24];
    int16_t hp;
    int16_t hp_max;
    int16_t damage;
    int16_t defense;
    uint8_t speed;
    bool alive;
} LibCombatEnemy;

typedef struct {
    bool active;
    LibCombatEnemy enemies[LIB_COMBAT_MAX_ENEMIES];
    int enemy_count;
    int selected_droid;
    int selected_target;
    int turn;
    uint16_t prng_state;
} LibCombatState;

void lib_combat_init(LibCombatState *cs);
void lib_combat_generate_encounter(LibCombatState *cs, uint16_t seed,
                                    int difficulty);
bool lib_combat_droid_attack(LibCombatState *cs, GameState *gs, int droid_idx);
void lib_combat_enemy_turn(LibCombatState *cs, GameState *gs);
bool lib_combat_is_over(const LibCombatState *cs, const GameState *gs);
bool lib_combat_player_won(const LibCombatState *cs);

#endif
