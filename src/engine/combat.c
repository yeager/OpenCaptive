#include "combat.h"
#include "game_state.h"
#include "captive_data.h"
#include "creature_stats.h"
#include "spawn.h"
#include "xp_level.h"
#include <stdlib.h>
#include <string.h>

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

    int difficulty = level_num;
    if (difficulty > 8) difficulty = 8;
    int num_groups = 3 + level_num;
    if (num_groups > 12) num_groups = 12;

    for (int g = 0; g < num_groups; g++) {
        if (cl->num_creatures >= MAX_CREATURES) break;

        int attempts = 100;
        while (attempts-- > 0) {
            int x = 1 + (combat_rand() % (MAP_WIDTH - 2));
            int y = 1 + (combat_rand() % (MAP_HEIGHT - 2));
            if (lvl->cells[y][x].type != CELL_FLOOR) continue;

            int category = combat_rand() % SPAWN_CATEGORY_COUNT;
            uint8_t direction = combat_rand() % 4;
            uint8_t position = (uint8_t)(x & 0x0F);

            SpawnResult sr = spawn_creatures(category, difficulty, direction,
                                             position, &combat_seed);

            for (int s = 0; s < sr.count; s++) {
                if (cl->num_creatures >= MAX_CREATURES) break;
                SpawnEntry *se = &sr.entries[s];
                Creature *c = &cl->creatures[cl->num_creatures++];
                memset(c, 0, sizeof(*c));

                c->type = (se->creature_type < CREATURE_COUNT)
                    ? (CreatureType)se->creature_type : CREATURE_ALIEN1;
                c->hp = se->hp;
                c->hp_max = se->hp;
                if (se->creature_type < CREATURE_TYPE_COUNT) {
                    const CreatureTypeDef *def = &creature_types[se->creature_type];
                    c->speed = def->speed;
                }
                c->damage_min = 5 + level_num * 2;
                c->damage_max = 15 + level_num * 3;
                c->defense = 2 + level_num;
                c->range = 4;
                c->x = x + (s % 2);
                c->y = y + (s / 2);
                if (c->x >= MAP_WIDTH) c->x = MAP_WIDTH - 1;
                if (c->y >= MAP_HEIGHT) c->y = MAP_HEIGHT - 1;
                c->level = level_num;
                c->active = true;
            }
            break;
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
        if (c->level != gs->current_level) continue;
        if (!c->active) {
            if (c->respawn_timer > 0) {
                c->respawn_timer--;
                if (c->respawn_timer == 0) {
                    c->hp = c->hp_max;
                    c->active = true;
                    c->alerted = false;
                    c->cooldown = 0;
                }
            }
            continue;
        }

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

    if (d->energy < 3) return false;
    d->energy -= 3;

    /* Damage formula from CAPPO.EXE:
     * 0x9BF4: ax=[di+6]; mul ah → lo_byte × hi_byte = base damage
     * 0x9BFC: shl cx,1 up to 3 times (×8), cap at 0xFFFD on sign overflow */
    uint16_t dmg_word = d->weapon_damage;
    uint8_t lo = dmg_word & 0xFF;
    uint8_t hi = (dmg_word >> 8) & 0xFF;
    int16_t cx = (int16_t)((uint16_t)lo * (uint16_t)hi);
    if (cx <= 0) cx = 5;
    for (int i = 0; i < 3; i++) {
        int16_t next = cx << 1;
        if (next < 0) { cx = (int16_t)0xFFFD; break; }
        cx = next;
    }
    int base_damage = (int)cx;
    int damage = base_damage - target->defense / 2;
    if (damage < 1) damage = 1;

    target->hp -= damage;
    if (target->hp <= 0) {
        target->active = false;
        target->respawn_timer = 600;
        d->xp = xp_add(d->xp, target->hp_max / 10);
        gs->gold += target->hp_max / 5 + 1;
        {
            uint16_t old_lvl = xp_to_display_level(d->xp - target->hp_max / 10);
            uint16_t new_lvl = xp_to_display_level(d->xp);
            if (new_lvl > old_lvl) {
                d->hp_max += 10;
                d->hp = d->hp_max;
                d->energy_max += 5;
                d->energy = d->energy_max;
            }
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
            if (gs->generators_destroyed >= gs->generators_total)
                game_state_complete_mission(gs);
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
