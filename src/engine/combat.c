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

static bool combat_all_droids_dead(const GameState *gs) {
    if (!gs) return false;
    for (int i = 0; i < 4; ++i)
        if (gs->droids[i].hp > 0) return false;
    return true;
}

bool combat_cell_occupied(const CreatureList *cl, int level, int x, int y) {
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

bool combat_change_floor_if_clear(GameState *gs, CreatureList *cl,
                                  int direction) {
    if (!gs) return false;
    int old_level = gs->current_level;
    int old_x = gs->party_x;
    int old_y = gs->party_y;
    if (!game_state_change_floor(gs, direction)) return false;
    if (combat_cell_occupied(cl, gs->current_level, gs->party_x, gs->party_y)) {
        gs->current_level = old_level;
        gs->party_x = old_x;
        gs->party_y = old_y;
        return false;
    }
    return true;
}

void combat_init(CreatureList *cl) {
    if (!cl) return;
    memset(cl, 0, sizeof(*cl));
}

static void combat_spawn_for_level_internal(CreatureList *cl,
                                             const DungeonLevel *lvl,
                                             int level_num, uint32_t seed,
                                             const GameState *avoid_party) {
    if (!cl || !lvl || level_num < 0 || level_num >= MAX_LEVELS) return;
    if (cl->num_creatures < 0) cl->num_creatures = 0;
    if (cl->num_creatures > MAX_CREATURES) return;
    combat_seed = (uint32_t)((uint64_t)seed +
                             (uint64_t)(int64_t)level_num * UINT64_C(3571));

    int difficulty = level_num;
    if (difficulty > 8) difficulty = 8;
    /* Captive increases encounter density by two groups per dungeon level.
     * Keeping this as 3 + level_num silently made the later floors under-
     * populated even though the recovered progression contract is
     * documented as 3 + (level * 2). */
    int num_groups = 3 + level_num * 2;
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
                    (avoid_party && level_num == avoid_party->current_level &&
                     spawn_x == avoid_party->party_x &&
                     spawn_y == avoid_party->party_y) ||
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
                    /* Compatibility approximation for creature attack stats.
                     * CAPPO.EXE 0x5380 is the weapon/attack bytecode path
                     * that builds the lo/hi damage word; it does not by
                     * itself prove the creature-record damage formula. Keep
                     * this bounded until the enemy attack record and caller
                     * are recovered from the disassembly. */
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

    if (level_num >= 3 && cl->num_creatures < MAX_CREATURES) {
        int attempts = 100;
        while (attempts-- > 0) {
            int bx = 1 + (combat_rand() % (MAP_WIDTH - 2));
            int by = 1 + (combat_rand() % (MAP_HEIGHT - 2));
            if (lvl->cells[by][bx].type != CELL_FLOOR) continue;
            if (avoid_party && level_num == avoid_party->current_level &&
                bx == avoid_party->party_x && by == avoid_party->party_y) continue;
            if (combat_cell_occupied(cl, level_num, bx, by)) continue;
            Creature *boss = &cl->creatures[cl->num_creatures++];
            memset(boss, 0, sizeof(*boss));
            boss->type = CREATURE_ALIEN6;
            int16_t bhp = (int16_t)(80 + level_num * 30);
            boss->hp = bhp;
            boss->hp_max = bhp;
            boss->damage_min = (int16_t)(10 + level_num * 4);
            boss->damage_max = (int16_t)(20 + level_num * 6);
            boss->defense = (int16_t)(8 + level_num * 2);
            boss->speed = 6;
            boss->range = 3;
            boss->x = bx;
            boss->y = by;
            boss->level = level_num;
            boss->active = true;
            boss->is_boss = true;
            if (level_num >= 6) boss->status_attack = STATUS_POISON;
            if (level_num >= 9) boss->status_attack |= STATUS_STUN;
            break;
        }
    }
}

void combat_spawn_for_level(CreatureList *cl, const DungeonLevel *lvl,
                            int level_num, uint32_t seed) {
    combat_spawn_for_level_internal(cl, lvl, level_num, seed, NULL);
}

void combat_spawn_for_level_avoiding_party(CreatureList *cl,
                                           const DungeonLevel *lvl,
                                           int level_num, uint32_t seed,
                                           const GameState *gs) {
    if (!gs || gs->current_level < 0 || gs->current_level >= MAX_LEVELS ||
        gs->party_x < 0 || gs->party_x >= MAP_WIDTH ||
        gs->party_y < 0 || gs->party_y >= MAP_HEIGHT)
        combat_spawn_for_level_internal(cl, lvl, level_num, seed, NULL);
    else
        combat_spawn_for_level_internal(cl, lvl, level_num, seed, gs);
}

static int distance(int x1, int y1, int x2, int y2) {
    int dx = x1 - x2;
    int dy = y1 - y2;
    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;
    /* Captive movement and weapon reach are cardinal.  Chebyshev distance
     * incorrectly allowed a melee weapon to hit a diagonal target and made
     * ranged/creature detection extend farther around corners than the
     * documented Manhattan-based combat rules. */
    return dx + dy;
}

static bool blocks_movement_or_sight(CellType cell) {
    return cell == CELL_WALL || cell == CELL_DOOR || cell == CELL_DOOR_LOCKED;
}

static int combat_weapon_range(uint8_t item_id) {
    /* CAPPO's item table gives spray weapons (IDs 33-35) a shorter range
     * than handguns, rifles and the other ranged weapons.  Keep this small
     * lookup aligned with inventory.c until combat receives an ItemDatabase
     * parameter; treating every ranged weapon as range six lets sprays hit
     * targets outside their documented reach. */
    if (item_id >= 13 && item_id <= 17) return 1;
    if (item_id >= 33 && item_id <= 35) return 4;
    if ((item_id >= 18 && item_id <= 32) ||
        (item_id >= 36 && item_id <= 38)) return 6;
    return 0;
}

static void combat_award_kill_xp(GameState *gs, CreatureList *cl,
                                 const Creature *target) {
    if (!gs || !cl || !target) return;

    /* CAPPO.EXE: XP award path at 0x9621 iterates over the droid roster and
     * awards the kill value to every living droid.  Giving the reward only to
     * the attacking droid made inactive party members permanently lag behind
     * the original shared progression model. */
    for (int i = 0; i < 4; ++i) {
        Droid *recipient = &gs->droids[i];
        if (recipient->hp <= 0) continue;

        uint32_t old_xp = recipient->xp;
        uint32_t xp_reward = xp_award((int)target->speed, target->level,
                                      recipient->skill_levels[XP_SKILL_COUNT - 1]);
        recipient->xp = xp_add(old_xp, xp_reward);

        uint16_t old_level = xp_to_display_level(old_xp);
        uint16_t new_level = xp_to_display_level(recipient->xp);
        if (new_level > old_level) {
            recipient->hp_max = (recipient->hp_max > INT16_MAX - 10) ?
                INT16_MAX : (int16_t)(recipient->hp_max + 10);
            recipient->hp = recipient->hp_max;
            recipient->energy_max = (recipient->energy_max > INT16_MAX - 5) ?
                INT16_MAX : (int16_t)(recipient->energy_max + 5);
            recipient->energy = recipient->energy_max;
            cl->level_up_occurred = true;
        }
    }
}

void combat_register_kill(GameState *gs, CreatureList *cl,
                          Creature *target) {
    if (!gs || !cl || !target || !target->active || target->hp > 0) return;

    /* Keep every kill source on the same recovered progression path.  The
     * primary target used to do this inline, while spray splash and grenade
     * kills only flipped active=false and therefore lost all rewards and event
     * state.  The active guard also makes alternate weapon callers idempotent
     * if an already-finalized target is presented again. */
    target->active = false;
    target->respawn_timer = 600;
    cl->creature_killed = true;

    uint32_t score_reward = (uint32_t)(target->hp_max / 2 + 1);
    if (target->is_boss) score_reward += 500U;
    if (UINT32_MAX - gs->score < score_reward)
        gs->score = UINT32_MAX;
    else
        gs->score += score_reward;

    int reward = target->hp_max / 5 + 1;
    if (reward > 0) {
        if (gs->gold > INT_MAX - reward)
            gs->gold = INT_MAX;
        else
            gs->gold += reward;
    }

    if (target->level >= 0 && target->level < gs->num_levels &&
        target->level < MAX_LEVELS && target->x >= 0 && target->x < MAP_WIDTH &&
        target->y >= 0 && target->y < MAP_HEIGHT) {
        MapCell *drop_cell = &gs->levels[target->level].cells[target->y][target->x];
        if (drop_cell->item_id == 0 && (combat_rand() % 3) == 0)
            drop_cell->item_id = (uint8_t)(1 + combat_rand() % 20);
    }

    combat_award_kill_xp(gs, cl, target);
}

bool combat_throw_grenade(GameState *gs, CreatureList *cl,
                          const ItemDatabase *db) {
    if (cl) {
        cl->creature_killed = false;
        cl->level_up_occurred = false;
    }
    if (!gs || !cl || !db || gs->game_type != GAME_CAPTIVE ||
        gs->current_level < 0 || gs->current_level >= gs->num_levels ||
        gs->current_level >= MAX_LEVELS ||
        gs->party_x < 0 || gs->party_x >= MAP_WIDTH ||
        gs->party_y < 0 || gs->party_y >= MAP_HEIGHT ||
        gs->party_dir < DIR_NORTH || gs->party_dir > DIR_WEST ||
        gs->selected_droid < 0 || gs->selected_droid >= 4 ||
        cl->num_creatures < 0)
        return false;

    Droid *droid = &gs->droids[gs->selected_droid];
    /* A destroyed droid cannot perform an alternate weapon action. */
    if (droid->hp <= 0) return false;

    int grenade_slot = -1;
    for (int slot = 0; slot < 10; ++slot) {
        uint8_t item_id = droid->items[slot];
        if (item_id == 60 || item_id == 61) {
            grenade_slot = slot;
            break;
        }
    }
    if (grenade_slot < 0) return false;

    const Item *grenade = item_db_get(db, droid->items[grenade_slot]);
    int damage = grenade ? (grenade->damage_min + grenade->damage_max) / 2 : 30;
    if (damage < 1) damage = 1;
    droid->items[grenade_slot] = 0;

    const int forward_x[4] = {0, 1, 0, -1};
    const int forward_y[4] = {-1, 0, 1, 0};
    int target_x = gs->party_x + forward_x[gs->party_dir] * 2;
    int target_y = gs->party_y + forward_y[gs->party_dir] * 2;
    int creature_count = cl->num_creatures > MAX_CREATURES ? MAX_CREATURES :
                         cl->num_creatures;
    for (int i = 0; i < creature_count; ++i) {
        Creature *target = &cl->creatures[i];
        if (!target->active || target->level != gs->current_level ||
            target->hp <= 0) continue;
        int distance = abs(target->x - target_x) + abs(target->y - target_y);
        if (distance > 2) continue;
        int effective = damage - target->defense / 3;
        if (effective < 1) effective = 1;
        if (effective >= target->hp) {
            target->hp = 0;
            combat_register_kill(gs, cl, target);
        } else {
            target->hp = (int16_t)(target->hp - effective);
        }
    }
    return true;
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
    int dy = -(y1 > y0 ? y1 - y0 : y0 - y1);
    int sy = y0 < y1 ? 1 : -1;
    int error = dx + dy;
    int x = x0, y = y0;

    while (x != x1 || y != y1) {
        int twice_error = error * 2;
        int old_x = x;
        int old_y = y;
        if (twice_error >= dy) { error += dy; x += sx; }
        if (twice_error <= dx) { error += dx; y += sy; }
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

    for (int di = 0; di < 4; di++) {
        Droid *d = &gs->droids[di];
        if (d->hp <= 0) continue;
        if (d->poison_timer > 0) {
            d->poison_timer--;
            int pdmg = 2;
            if (pdmg >= d->hp) d->hp = 0;
            else d->hp -= (int16_t)pdmg;
            if (d->hp <= 0 && combat_all_droids_dead(gs))
                gs->mode = STATE_GAMEOVER;
        }
        if (d->stun_timer > 0) d->stun_timer--;
        if (d->poison_timer == 0) d->status &= ~STATUS_POISON;
        if (d->stun_timer == 0) d->status &= ~STATUS_STUN;
    }
    int creature_count = cl->num_creatures;
    if (creature_count < 0) creature_count = 0;
    if (creature_count > MAX_CREATURES) creature_count = MAX_CREATURES;
    cl->attack_occurred = false;
    for (int i = 0; i < creature_count; i++) {
        Creature *c = &cl->creatures[i];
        /* Respawn timers are world timers, not viewport timers.  Process
         * inactive creatures on every level before limiting combat AI to the
         * currently visible level; otherwise a killed creature on another
         * floor never respawns while the party is away. */
        if (c->level < 0 || c->level >= gs->num_levels ||
            c->level >= MAX_LEVELS)
            continue;
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
                    if (c->hp_max > 0) {
                        c->hp = c->hp_max;
                        c->active = true;
                        c->alerted = false;
                        c->cooldown = 0;
                    }
                }
            }
            continue;
        }
        if (c->level != gs->current_level) continue;

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
            if (gs->difficulty == 0) damage = damage * 2 / 3;
            else if (gs->difficulty == 2) damage = damage * 3 / 2;
            if (damage < 1) damage = 1;
            /* The runtime shield is a first-line damage sink.  Do not apply
             * body-part wear until damage has passed through it; otherwise a
             * fully charged shield still damages armor on every hit. */
            if (d->shield != 0 && d->shield_hp > 0) {
                if (damage <= d->shield_hp) {
                    d->shield_hp -= (int16_t)damage;
                    damage = 0;
                } else {
                    damage -= d->shield_hp;
                    d->shield_hp = 0;
                }
            }
            if (damage > 0) {
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
            }
            if (damage > 0) {
                if (damage >= d->hp)
                    d->hp = 0;
                else
                    d->hp = (int16_t)(d->hp - damage);
            }
            if (c->status_attack & STATUS_POISON) {
                d->status |= STATUS_POISON;
                if (d->poison_timer < 10) d->poison_timer = 10;
            }
            if (c->status_attack & STATUS_STUN) {
                d->status |= STATUS_STUN;
                if (d->stun_timer < 5) d->stun_timer = 5;
            }
            if (c->status_attack & STATUS_DRAIN) {
                int drain = damage > 0 ? damage / 2 : 5;
                if (drain > d->energy) d->energy = 0;
                else d->energy -= (int16_t)drain;
            }
            if (combat_all_droids_dead(gs)) gs->mode = STATE_GAMEOVER;
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
            bool wall_blocked = (nx < 0 || nx >= MAP_WIDTH || ny < 0 || ny >= MAP_HEIGHT ||
                blocks_movement_or_sight(gs->levels[gs->current_level].cells[ny][nx].type));
            if (wall_blocked) {
                int alt_dx = 0, alt_dy = 0;
                if (move_dx != 0) alt_dy = dy_to_party > 0 ? 1 : (dy_to_party < 0 ? -1 : 1);
                else alt_dx = dx_to_party > 0 ? 1 : (dx_to_party < 0 ? -1 : 1);
                nx = c->x + alt_dx;
                ny = c->y + alt_dy;
                if (nx < 0 || nx >= MAP_WIDTH || ny < 0 || ny >= MAP_HEIGHT ||
                    blocks_movement_or_sight(gs->levels[gs->current_level].cells[ny][nx].type)) {
                    nx = c->x; ny = c->y;
                }
            }
            if (nx != c->x || ny != c->y) {
                bool blocked = nx == gs->party_x && ny == gs->party_y;
                for (int j = 0; j < creature_count && !blocked; j++) {
                    if (j != i && cl->creatures[j].active &&
                        cl->creatures[j].level == gs->current_level &&
                        cl->creatures[j].x == nx && cl->creatures[j].y == ny)
                        blocked = true;
                }
                if (!blocked) { c->x = nx; c->y = ny; }
            }
            c->cooldown = c->speed / 2;
        }
    }
}

bool combat_droid_attack(GameState *gs, CreatureList *cl, int droid_idx) {
    /* These flags describe the current attack, not the last successful one.
     * Clear them before validation so callers cannot observe a stale kill or
     * level-up event after a miss or a non-lethal shot. */
    if (cl) {
        cl->creature_killed = false;
        cl->level_up_occurred = false;
    }
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
    if (d->stun_timer > 0) return false;

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

    /* A droid may carry one weapon in each hand.  Use the longest actual
     * equipped range, while still allowing a melee weapon in hand 0 to
     * coexist with a ranged weapon in hand 1. */
    int attack_range = 0;
    for (int w = 0; w < 2; w++) {
        uint8_t wid = d->weapons[w];
        int weapon_range = combat_weapon_range(wid);
        if (weapon_range > attack_range) attack_range = weapon_range;
    }
    if (attack_range == 0) return false;

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
        /* CAPPO keeps this saturated value as an unsigned 16-bit damage
         * word.  Converting 0xFFFD through int16_t would turn it into -3 and
         * collapse an over-range weapon into the minimum one-point hit. */
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
        combat_register_kill(gs, cl, target);
    }

    bool is_spray = false;
    for (int w = 0; w < 2; w++) {
        uint8_t wid = d->weapons[w];
        if (wid >= 33 && wid <= 35) { is_spray = true; break; }
    }
    if (is_spray) {
        for (int i = 0; i < creature_count; i++) {
            Creature *c = &cl->creatures[i];
            if (c == target || !c->active || c->level != gs->current_level ||
                c->hp <= 0) continue;
            int cdist = distance(c->x, c->y, target->x, target->y);
            if (cdist > 2) continue;
            int splash = base_damage / 3 - c->defense / 4;
            if (splash < 1) splash = 1;
            if (splash >= c->hp) {
                c->hp = 0;
                combat_register_kill(gs, cl, c);
            } else {
                c->hp = (int16_t)(c->hp - splash);
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
        case CELL_TERMINAL:
            gs->mode = STATE_TERMINAL;
            break;
        default:
            break;
    }
}
