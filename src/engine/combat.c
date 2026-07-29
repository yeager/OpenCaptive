#include "combat.h"
#include <stdlib.h>
#include <string.h>

// Creature stat tables (hp, damage_min, damage_max, defense, speed, range)
static const int16_t creature_stats[CREATURE_COUNT][6] = {
    [CREATURE_NONE]     = {0,0,0,0,0,0},
    [CREATURE_DRONE]    = {30,  5, 15, 2, 8, 4},
    [CREATURE_GUARD]    = {60, 10, 25, 5, 6, 3},
    [CREATURE_TURRET]   = {80, 15, 30, 8, 10, 6},
    [CREATURE_ROBOT]    = {120, 20, 40, 10, 5, 2},
    [CREATURE_ENFORCER] = {200, 30, 60, 15, 4, 3},
    [CREATURE_BOSS]     = {500, 50,100, 25, 3, 4},
};

static uint32_t combat_prng;

static uint32_t combat_rand(void) {
    combat_prng = combat_prng * 1103515245 + 12345;
    return (combat_prng >> 16) & 0x7FFF;
}

void combat_init(CreatureList *cl) {
    memset(cl, 0, sizeof(*cl));
}

void combat_spawn_for_level(CreatureList *cl, const DungeonLevel *lvl,
                            int level_num, uint32_t seed) {
    combat_prng = seed + level_num * 3571;

    int num_to_spawn = 3 + level_num * 2;
    if (num_to_spawn > 20) num_to_spawn = 20;

    for (int i = 0; i < num_to_spawn; i++) {
        if (cl->num_creatures >= MAX_CREATURES) break;

        // Find a floor cell
        int attempts = 100;
        while (attempts-- > 0) {
            int x = 1 + (combat_rand() % (MAP_WIDTH - 2));
            int y = 1 + (combat_rand() % (MAP_HEIGHT - 2));
            if (lvl->cells[y][x].type == CELL_FLOOR) {
                Creature *c = &cl->creatures[cl->num_creatures++];
                memset(c, 0, sizeof(*c));

                // Type scales with level
                int max_type = 1 + level_num / 2;
                if (max_type >= CREATURE_COUNT) max_type = CREATURE_COUNT - 1;
                c->type = 1 + (combat_rand() % max_type);

                c->hp = creature_stats[c->type][0] + level_num * 5;
                c->hp_max = c->hp;
                c->damage_min = creature_stats[c->type][1] + level_num * 2;
                c->damage_max = creature_stats[c->type][2] + level_num * 3;
                c->defense = creature_stats[c->type][3] + level_num;
                c->speed = creature_stats[c->type][4];
                c->range = creature_stats[c->type][5];
                c->x = x;
                c->y = y;
                c->level = level_num;
                c->active = true;
                break;
            }
        }
    }
}

static int distance(int x1, int y1, int x2, int y2) {
    int dx = x1 - x2;
    int dy = y1 - y2;
    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;
    return (dx > dy) ? dx : dy; // Chebyshev distance
}

void combat_tick(CreatureList *cl, GameState *gs) {
    for (int i = 0; i < cl->num_creatures; i++) {
        Creature *c = &cl->creatures[i];
        if (!c->active || c->level != gs->current_level) continue;

        int dist = distance(c->x, c->y, gs->party_x, gs->party_y);

        // Alert if player is nearby
        if (dist <= c->range + 2) c->alerted = true;
        if (!c->alerted) continue;

        // Cooldown
        if (c->cooldown > 0) { c->cooldown--; continue; }

        if (dist <= c->range) {
            // Attack a random droid
            int target = combat_rand() % 4;
            Droid *d = &gs->droids[target];
            int damage = c->damage_min +
                         (combat_rand() % (c->damage_max - c->damage_min + 1));
            d->hp -= damage;
            if (d->hp < 0) d->hp = 0;
            c->cooldown = c->speed;
        } else {
            // Move toward player
            int move_dx = 0, move_dy = 0;
            if (c->x < gs->party_x) move_dx = 1;
            else if (c->x > gs->party_x) move_dx = -1;
            if (c->y < gs->party_y) move_dy = 1;
            else if (c->y > gs->party_y) move_dy = -1;

            int nx = c->x + move_dx;
            int ny = c->y + move_dy;
            if (nx >= 0 && nx < MAP_WIDTH && ny >= 0 && ny < MAP_HEIGHT &&
                gs->levels[gs->current_level].cells[ny][nx].type != CELL_WALL) {
                c->x = nx;
                c->y = ny;
            }
            c->cooldown = c->speed / 2;
        }
    }
}

bool combat_droid_attack(GameState *gs, CreatureList *cl, int droid_idx) {
    if (droid_idx < 0 || droid_idx >= 4) return false;
    Droid *d = &gs->droids[droid_idx];
    if (d->hp <= 0) return false;

    // Find closest creature in front of party within range
    int fwd_x = (int[]){0,1,0,-1}[gs->party_dir];
    int fwd_y = (int[]){-1,0,1,0}[gs->party_dir];

    Creature *target = NULL;
    int best_dist = 9999;

    for (int i = 0; i < cl->num_creatures; i++) {
        Creature *c = &cl->creatures[i];
        if (!c->active || c->level != gs->current_level || c->hp <= 0) continue;

        int dx = c->x - gs->party_x;
        int dy = c->y - gs->party_y;

        // Check if roughly in front
        int dot = dx * fwd_x + dy * fwd_y;
        if (dot <= 0) continue;

        int dist = distance(c->x, c->y, gs->party_x, gs->party_y);
        if (dist < best_dist && dist <= 6) {
            best_dist = dist;
            target = c;
        }
    }

    if (!target) return false;

    // Calculate damage (base 10-20 for basic weapon)
    int base_damage = 10 + (combat_rand() % 11);
    int damage = base_damage - target->defense / 2;
    if (damage < 1) damage = 1;

    target->hp -= damage;
    if (target->hp <= 0) {
        target->active = false;
        d->xp += target->hp_max / 10;
        gs->gold += target->hp_max / 5 + 1;
        if (d->xp >= d->level * 100) {
            d->level++;
            d->hp_max += 10;
            d->hp = d->hp_max;
            d->energy_max += 5;
            d->energy = d->energy_max;
        }
    }

    return true;
}

void combat_interact(GameState *gs) {
    // Interact with cell in front of party
    int fwd_x = (int[]){0,1,0,-1}[gs->party_dir];
    int fwd_y = (int[]){-1,0,1,0}[gs->party_dir];
    int tx = gs->party_x + fwd_x;
    int ty = gs->party_y + fwd_y;

    if (tx < 0 || tx >= MAP_WIDTH || ty < 0 || ty >= MAP_HEIGHT) return;

    DungeonLevel *lvl = &gs->levels[gs->current_level];
    MapCell *cell = &lvl->cells[ty][tx];

    switch (cell->type) {
        case CELL_DOOR:
            cell->type = CELL_FLOOR; // open door
            break;
        case CELL_DOOR_LOCKED:
            // Need key or brute force
            break;
        case CELL_GENERATOR:
            cell->type = CELL_FLOOR;
            gs->generators_destroyed++;
            if (gs->generators_destroyed >= gs->generators_total) {
                // Mission complete — would trigger victory sequence
            }
            break;
        case CELL_SHOP:
            gs->mode = STATE_SHOP;
            break;
        case CELL_TERMINAL:
            gs->mode = STATE_TERMINAL;
            break;
        default:
            break;
    }
}
