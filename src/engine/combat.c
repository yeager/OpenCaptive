#include "combat.h"
#include "captive_data.h"
#include <stdlib.h>
#include <string.h>

/* Placeholder creature stats indexed by type.
 * Format: {hp, damage_min, damage_max, defense, speed, range}.
 * These values are NOT recovered from the original executable —
 * they are functional placeholders until the combat formula code
 * section of CAPPO.EXE is disassembled. */
static const int16_t creature_stats[CREATURE_COUNT][6] = {
    [CREATURE_NONE]   = {0,0,0,0,0,0},
    [CREATURE_ALIEN1] = {30,  5, 15, 2, 8, 4},
    [CREATURE_ALIEN2] = {60, 10, 25, 5, 6, 3},
    [CREATURE_ALIEN3] = {80, 15, 30, 8, 10, 6},
    [CREATURE_ALIEN4] = {120, 20, 40, 10, 5, 2},
    [CREATURE_ALIEN5] = {200, 30, 60, 15, 4, 3},
    [CREATURE_ALIEN6] = {500, 50,100, 25, 3, 4},
};

static uint32_t combat_seed;

static uint32_t combat_rand(void) {
    return captive_combat_prng(&combat_seed);
}

void combat_init(CreatureList *cl) {
    memset(cl, 0, sizeof(*cl));
}

void combat_spawn_for_level(CreatureList *cl, const DungeonLevel *lvl,
                            int level_num, uint32_t seed) {
    combat_seed = seed + level_num * 3571;

    int num_to_spawn = 3 + level_num * 2;
    if (num_to_spawn > 20) num_to_spawn = 20;

    for (int i = 0; i < num_to_spawn; i++) {
        if (cl->num_creatures >= MAX_CREATURES) break;

        int attempts = 100;
        while (attempts-- > 0) {
            int x = 1 + (combat_rand() % (MAP_WIDTH - 2));
            int y = 1 + (combat_rand() % (MAP_HEIGHT - 2));
            if (lvl->cells[y][x].type == CELL_FLOOR) {
                Creature *c = &cl->creatures[cl->num_creatures++];
                memset(c, 0, sizeof(*c));

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
    return (dx > dy) ? dx : dy;
}

static bool blocks_movement_or_sight(CellType cell) {
    return cell == CELL_WALL || cell == CELL_DOOR || cell == CELL_DOOR_LOCKED;
}

static bool combat_has_line_of_sight(const GameState *gs,
                                     int x0, int y0, int x1, int y1) {
    if (!gs || gs->current_level < 0 || gs->current_level >= gs->num_levels)
        return false;
    int dx = x1 > x0 ? x1 - x0 : x0 - x1;
    int sx = x0 < x1 ? 1 : -1;
    int dy = y1 > y0 ? y0 - y1 : y1 - y0;
    int sy = y0 < y1 ? 1 : -1;
    int error = dx - dy;
    int x = x0, y = y0;

    while (x != x1 || y != y1) {
        int twice_error = error * 2;
        if (twice_error > -dy) { error -= dy; x += sx; }
        if (twice_error < dx) { error += dx; y += sy; }
        if (x == x1 && y == y1) break;
        if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT ||
            blocks_movement_or_sight(gs->levels[gs->current_level].cells[y][x].type))
            return false;
    }
    return true;
}

void combat_tick(CreatureList *cl, GameState *gs) {
    for (int i = 0; i < cl->num_creatures; i++) {
        Creature *c = &cl->creatures[i];
        if (!c->active || c->level != gs->current_level) continue;

        int dist = distance(c->x, c->y, gs->party_x, gs->party_y);

        if (dist <= c->range + 2) c->alerted = true;
        if (!c->alerted) continue;

        if (c->cooldown > 0) { c->cooldown--; continue; }

        if (dist <= c->range && combat_has_line_of_sight(gs, c->x, c->y,
                                                          gs->party_x, gs->party_y)) {
            int target = combat_rand() % 4;
            Droid *d = &gs->droids[target];
            int damage = c->damage_min +
                         (combat_rand() % (c->damage_max - c->damage_min + 1));
            d->hp -= damage;
            if (d->hp < 0) d->hp = 0;
            c->cooldown = c->speed;
        } else {
            int move_dx = 0, move_dy = 0;
            if (c->x < gs->party_x) move_dx = 1;
            else if (c->x > gs->party_x) move_dx = -1;
            if (c->y < gs->party_y) move_dy = 1;
            else if (c->y > gs->party_y) move_dy = -1;

            int nx = c->x + move_dx;
            int ny = c->y + move_dy;
            if (nx >= 0 && nx < MAP_WIDTH && ny >= 0 && ny < MAP_HEIGHT &&
                !blocks_movement_or_sight(gs->levels[gs->current_level].cells[ny][nx].type)) {
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

    int fwd_x = (int[]){0,1,0,-1}[gs->party_dir];
    int fwd_y = (int[]){-1,0,1,0}[gs->party_dir];

    Creature *target = NULL;
    int best_dist = 9999;

    for (int i = 0; i < cl->num_creatures; i++) {
        Creature *c = &cl->creatures[i];
        if (!c->active || c->level != gs->current_level || c->hp <= 0) continue;

        int dx = c->x - gs->party_x;
        int dy = c->y - gs->party_y;

        int dot = dx * fwd_x + dy * fwd_y;
        if (dot <= 0) continue;

        int dist = distance(c->x, c->y, gs->party_x, gs->party_y);
        if (dist < best_dist && dist <= 6 &&
            combat_has_line_of_sight(gs, gs->party_x, gs->party_y, c->x, c->y)) {
            best_dist = dist;
            target = c;
        }
    }

    if (!target) return false;

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
    int fwd_x = (int[]){0,1,0,-1}[gs->party_dir];
    int fwd_y = (int[]){-1,0,1,0}[gs->party_dir];
    int tx = gs->party_x + fwd_x;
    int ty = gs->party_y + fwd_y;

    if (tx < 0 || tx >= MAP_WIDTH || ty < 0 || ty >= MAP_HEIGHT) return;

    DungeonLevel *lvl = &gs->levels[gs->current_level];
    MapCell *cell = &lvl->cells[ty][tx];

    switch (cell->type) {
        case CELL_DOOR:
            cell->type = CELL_FLOOR;
            break;
        case CELL_DOOR_LOCKED:
            break;
        case CELL_GENERATOR:
            cell->type = CELL_FLOOR;
            gs->generators_destroyed++;
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
