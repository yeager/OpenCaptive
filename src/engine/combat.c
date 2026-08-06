#include "combat.h"
#include "game_state.h"
#include "captive_data.h"
#include "creature_stats.h"
#include "spawn.h"
#include "xp_level.h"
#include "inventory.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

static uint32_t combat_seed;

static uint32_t combat_rand(void) {
    return captive_combat_prng(&combat_seed);
}

static bool combat_cell_occupied(const CreatureList *cl, int level,
                                 int x, int y) {
    if (!cl || cl->num_creatures <= 0) return false;
    int count = cl->num_creatures > MAX_CREATURES ? MAX_CREATURES :
                cl->num_creatures;
    for (int i = 0; i < count; i++) {
        const Creature *c = &cl->creatures[i];
        if (c->active && c->level == level && c->x == x && c->y == y)
            return true;
    }
    return false;
}

void combat_init(CreatureList *cl) {
    if (!cl) return;
    memset(cl, 0, sizeof(*cl));
}

void combat_spawn_for_level(CreatureList *cl, const DungeonLevel *lvl,
                            int level_num, uint32_t seed) {
    if (!cl || !lvl || level_num < 0 || level_num >= MAX_LEVELS) return;
    if (cl->num_creatures < 0) cl->num_creatures = 0;
    if (cl->num_creatures > MAX_CREATURES) return;
    combat_seed = (uint32_t)((uint64_t)seed +
                             (uint64_t)(int64_t)level_num * UINT64_C(3571));

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
                int spawn_x = x + (s % 2);
                int spawn_y = y + (s / 2);
                if (spawn_x < 0 || spawn_x >= MAP_WIDTH ||
                    spawn_y < 0 || spawn_y >= MAP_HEIGHT ||
                    lvl->cells[spawn_y][spawn_x].type != CELL_FLOOR ||
                    combat_cell_occupied(cl, level_num, spawn_x, spawn_y))
                    continue;
                SpawnEntry *se = &sr.entries[s];
                Creature *c = &cl->creatures[cl->num_creatures++];
                memset(c, 0, sizeof(*c));

                c->type = (se->creature_type >= 1 &&
                           se->creature_type < CREATURE_COUNT)
                    ? (CreatureType)se->creature_type : CREATURE_ALIEN1;
                c->hp = se->hp;
                c->hp_max = se->hp;
                if (se->creature_type >= 1 &&
                    se->creature_type <= CREATURE_TYPE_COUNT) {
                    const CreatureTypeDef *def =
                        &creature_types[se->creature_type - 1];
                    c->speed = def->speed;
                }
                {
                    /* Creature damage formula from CAPPO.EXE at 0x5380:
                     * base = min(20, combat_val / divisor + 1)
                     * type 0x60: base ×4; type 0x61: base ×8
                     * packed as lo*hi word (same encoding as weapons).
                     * Simplified here using category as the scaling tier. */
                    int cat = 0;
                    if (se->creature_type >= 1 &&
                        se->creature_type <= CREATURE_TYPE_COUNT)
                        cat = creature_types[se->creature_type - 1].category;
                    int base = 2 + cat + level_num;
                    if (base > 20) base = 20;
                    int dmg_lo = (base >> 1) | 1;
                    int dmg_hi = base;
                    c->damage_min = (int16_t)(dmg_lo * dmg_hi);
                    c->damage_max = (int16_t)(dmg_lo * dmg_hi + dmg_hi);
                    c->defense = (int16_t)(cat * 2 + level_num);
                    c->range = (cat >= 4) ? 4 + (cat - 4) : 1 + cat / 3;
                }
                c->x = spawn_x;
                c->y = spawn_y;
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

static bool combat_is_ranged_weapon_id(uint8_t item_id) {
    /* Inventory IDs 13-17 are melee weapons and 18-38 are ranged weapons.
     * Do not classify arbitrary non-weapon IDs as ranged merely because they
     * fall outside the melee interval: these values can survive in a legacy
     * or damaged save and must not silently extend attack reach. */
    return item_id >= 18 && item_id <= 38;
}

static bool combat_is_weapon_id(uint8_t item_id) {
    return item_id >= 13 && item_id <= 38;
}

static bool combat_has_line_of_sight(const GameState *gs,
                                     int x0, int y0, int x1, int y1) {
    if (!gs || gs->current_level < 0 || gs->current_level >= gs->num_levels ||
        gs->current_level >= MAX_LEVELS)
        return false;
    int dx = x1 > x0 ? x1 - x0 : x0 - x1;
    int sx = x0 < x1 ? 1 : -1;
    int dy = y1 > y0 ? y0 - y1 : y1 - y0;
    int sy = y0 < y1 ? 1 : -1;
    int error = dx - dy;
    int x = x0, y = y0;

    while (x != x1 || y != y1) {
        int twice_error = error * 2;
        int old_x = x;
        int old_y = y;
        if (twice_error > -dy) { error -= dy; x += sx; }
        if (twice_error < dx) { error += dx; y += sy; }
        if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT ||
            blocks_movement_or_sight(gs->levels[gs->current_level].cells[y][x].type))
            return false;
        /* A diagonal step cannot see through the corner formed by two
         * blocking cells.  Without this check the target at (x+1,y+1) was
         * visible even when both (x+1,y) and (x,y+1) were closed. */
        if (x != old_x && y != old_y) {
            CellType side_a = gs->levels[gs->current_level].cells[old_y][x].type;
            CellType side_b = gs->levels[gs->current_level].cells[y][old_x].type;
            if (blocks_movement_or_sight(side_a) &&
                blocks_movement_or_sight(side_b)) return false;
        }
        if (x == x1 && y == y1) break;
    }
    return true;
}

void combat_tick(CreatureList *cl, GameState *gs) {
    if (!cl || !gs || gs->current_level < 0 ||
        gs->current_level >= gs->num_levels || gs->current_level >= MAX_LEVELS ||
        gs->party_x < 0 || gs->party_x >= MAP_WIDTH ||
        gs->party_y < 0 || gs->party_y >= MAP_HEIGHT) return;
    int creature_count = cl->num_creatures;
    if (creature_count < 0) creature_count = 0;
    if (creature_count > MAX_CREATURES) creature_count = MAX_CREATURES;
    cl->attack_occurred = false;
    for (int i = 0; i < creature_count; i++) {
        Creature *c = &cl->creatures[i];
        if (c->level != gs->current_level) continue;
        if (c->x < 0 || c->x >= MAP_WIDTH || c->y < 0 || c->y >= MAP_HEIGHT)
            continue;
        if (c->active && c->hp <= 0) {
            c->active = false;
            if (c->respawn_timer == 0) c->respawn_timer = 600;
        }
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
            int living = 0;
            for (int di = 0; di < 4; di++)
                if (gs->droids[di].hp > 0) living++;
            if (living == 0) continue;
            int target_pick = combat_rand() % living;
            int target = 0;
            for (int di = 0; di < 4; di++) {
                if (gs->droids[di].hp <= 0) continue;
                if (target_pick-- == 0) {
                    target = di;
                    break;
                }
            }
            Droid *d = &gs->droids[target];
            int damage_min = c->damage_min;
            int damage_max = c->damage_max;
            int64_t damage_range64 = (int64_t)damage_max - damage_min + 1;
            if (damage_min < 0 || damage_max < damage_min || damage_range64 <= 0)
                continue;
            uint32_t damage_range = damage_range64 > UINT32_MAX
                ? UINT32_MAX : (uint32_t)damage_range64;
            int damage = damage_min + (int)(combat_rand() % damage_range);
            if (damage < 1) damage = 1;
            int part = combat_rand() % 6;
            if (d->body_parts[part] != 0 && d->body_part_hp[part] > 0) {
                int armor_reduce = d->body_part_hp[part] / 32;
                if (armor_reduce > 0) damage -= armor_reduce;
                if (damage < 1) damage = 1;
                int part_dmg = damage / 4;
                if (part_dmg < 1) part_dmg = 1;
                if (part_dmg >= d->body_part_hp[part])
                    d->body_part_hp[part] = 0;
                else
                    d->body_part_hp[part] -= (uint8_t)part_dmg;
            }
            if (damage >= d->hp)
                d->hp = 0;
            else
                d->hp = (int16_t)(d->hp - damage);
            c->cooldown = c->speed;
            cl->last_attack_damage = damage;
            cl->last_attack_target = target;
            cl->attack_occurred = true;
        } else {
            int move_dx = 0, move_dy = 0;
            int dx_to_party = gs->party_x - c->x;
            int dy_to_party = gs->party_y - c->y;
            /* Captive movement is cardinal.  Choosing one axis also keeps a
             * pursuer from cutting diagonally through a blocked corner. */
            if (dx_to_party != 0 &&
                (dy_to_party == 0 || abs(dx_to_party) >= abs(dy_to_party)))
                move_dx = dx_to_party > 0 ? 1 : -1;
            else if (dy_to_party != 0)
                move_dy = dy_to_party > 0 ? 1 : -1;

            int nx = c->x + move_dx;
            int ny = c->y + move_dy;
            if (nx >= 0 && nx < MAP_WIDTH && ny >= 0 && ny < MAP_HEIGHT &&
                !blocks_movement_or_sight(gs->levels[gs->current_level].cells[ny][nx].type)) {
                /* Creatures may attack from an adjacent/visible cell, but
                 * must never occupy the party's tile.  Otherwise chasing
                 * creatures overlap the party and the next distance check
                 * changes combat semantics. */
                bool blocked = nx == gs->party_x && ny == gs->party_y;
                for (int j = 0; j < creature_count; j++) {
                    if (j != i && cl->creatures[j].active &&
                        cl->creatures[j].level == gs->current_level &&
                        cl->creatures[j].x == nx && cl->creatures[j].y == ny) {
                        blocked = true;
                        break;
                    }
                }
                if (!blocked) { c->x = nx; c->y = ny; }
            }
            c->cooldown = c->speed / 2;
        }
    }
}

bool combat_droid_attack(GameState *gs, CreatureList *cl, int droid_idx) {
    if (!gs || !cl || droid_idx < 0 || droid_idx >= 4 ||
        gs->current_level < 0 || gs->current_level >= gs->num_levels ||
        gs->current_level >= MAX_LEVELS ||
        gs->party_x < 0 || gs->party_x >= MAP_WIDTH ||
        gs->party_y < 0 || gs->party_y >= MAP_HEIGHT ||
        gs->party_dir < DIR_NORTH || gs->party_dir > DIR_WEST) return false;
    int creature_count = cl->num_creatures;
    if (creature_count < 0) creature_count = 0;
    if (creature_count > MAX_CREATURES) creature_count = MAX_CREATURES;
    Droid *d = &gs->droids[droid_idx];
    if (d->hp <= 0) return false;

    bool has_weapon = false;
    for (int w = 0; w < 2; w++) {
        if (combat_is_weapon_id(d->weapons[w])) {
            has_weapon = true;
            break;
        }
    }
    if (!has_weapon) return false;

    int fwd_x = (int[]){0,1,0,-1}[gs->party_dir];
    int fwd_y = (int[]){-1,0,1,0}[gs->party_dir];

    /* A droid may carry one weapon in each hand.  The damage field stores
     * the strongest equipped weapon, while the old range check only looked
     * at the first non-empty hand.  That made a melee weapon in hand 0 hide
     * a ranged weapon in hand 1 and prevented attacks beyond one tile. */
    bool has_ranged_weapon = false;
    for (int w = 0; w < 2; w++) {
        uint8_t wid = d->weapons[w];
        if (combat_is_ranged_weapon_id(wid)) {
            has_ranged_weapon = true;
            break;
        }
    }
    bool is_melee = !has_ranged_weapon;
    int attack_range = is_melee ? 1 : 6;

    Creature *target = NULL;
    int best_dist = 9999;

    for (int i = 0; i < creature_count; i++) {
        Creature *c = &cl->creatures[i];
        if (!c->active || c->level != gs->current_level || c->hp <= 0 ||
            c->hp_max <= 0 ||
            c->x < 0 || c->x >= MAP_WIDTH || c->y < 0 || c->y >= MAP_HEIGHT)
            continue;

        int dx = c->x - gs->party_x;
        int dy = c->y - gs->party_y;

        int dot = dx * fwd_x + dy * fwd_y;
        if (dot <= 0) continue;

        int dist = distance(c->x, c->y, gs->party_x, gs->party_y);
        if (dist < best_dist && dist <= attack_range &&
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
    int cx = (int)lo * (int)hi;
    if (cx <= 0) cx = 5;
    for (int i = 0; i < 3; i++) {
        int next = cx * 2;
        if (next > 0x7FFF) { cx = 0xFFFD; break; }
        cx = next;
    }
    int base_damage = (int)cx;
    int damage = base_damage - target->defense / 2;
    if (damage < 1) damage = 1;

    if (damage >= target->hp)
        target->hp = 0;
    else
        target->hp = (int16_t)(target->hp - damage);
    if (target->hp <= 0) {
        target->active = false;
        target->respawn_timer = 600;
        cl->creature_killed = true;
        uint32_t old_xp = d->xp;
        uint32_t xp_reward = (uint32_t)(target->hp_max / 10);
        d->xp = xp_add(old_xp, xp_reward);
        int reward = target->hp_max / 5 + 1;
        if (reward > 0 && gs->gold <= INT_MAX - reward)
            gs->gold += reward;
        else if (reward > 0)
            gs->gold = INT_MAX;
        if (target->x >= 0 && target->x < MAP_WIDTH &&
            target->y >= 0 && target->y < MAP_HEIGHT) {
            MapCell *drop_cell = &gs->levels[gs->current_level].cells[target->y][target->x];
            if (drop_cell->item_id == 0 && (combat_rand() % 3) == 0) {
                drop_cell->item_id = (uint8_t)(1 + combat_rand() % 20);
            }
        }
        {
            uint16_t old_lvl = xp_to_display_level(old_xp);
            uint16_t new_lvl = xp_to_display_level(d->xp);
            if (new_lvl > old_lvl) {
                d->hp_max = (d->hp_max > INT16_MAX - 10) ?
                    INT16_MAX : (int16_t)(d->hp_max + 10);
                d->hp = d->hp_max;
                d->energy_max = (d->energy_max > INT16_MAX - 5) ?
                    INT16_MAX : (int16_t)(d->energy_max + 5);
                d->energy = d->energy_max;
                cl->level_up_occurred = true;
            }
        }
    }

    return true;
}

void combat_interact(GameState *gs, const void *item_db_ptr) {
    if (!gs || gs->current_level < 0 || gs->current_level >= gs->num_levels ||
        gs->current_level >= MAX_LEVELS ||
        gs->party_x < 0 || gs->party_x >= MAP_WIDTH ||
        gs->party_y < 0 || gs->party_y >= MAP_HEIGHT ||
        gs->party_dir < DIR_NORTH || gs->party_dir > DIR_WEST) return;
    const ItemDatabase *idb = (const ItemDatabase *)item_db_ptr;
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
            if (idb) {
                for (int di = 0; di < 4; di++) {
                    for (int si = 0; si < 10; si++) {
                        if (gs->droids[di].items[si] != 0) {
                            const Item *ki = item_db_get(idb, gs->droids[di].items[si]);
                            if (ki && ki->category == ITEM_KEY) {
                                gs->droids[di].items[si] = 0;
                                cell->type = CELL_FLOOR;
                                return;
                            }
                        }
                    }
                }
            }
            break;
        case CELL_GENERATOR:
            cell->type = CELL_FLOOR;
            if (gs->generators_destroyed < INT_MAX)
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
