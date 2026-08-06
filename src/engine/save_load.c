#include "save_load.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SAVE_MAGIC 0x4F435356  // "OCSV" - OpenCaptive Save
#define SAVE_VERSION 4
#define SAVE_VERSION_LEGACY 3

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t game_type;
    uint32_t mission;
    uint32_t mission_seed;
    uint32_t base_id;
    int32_t  party_x;
    int32_t  party_y;
    uint32_t party_dir;
    int32_t  current_level;
    int32_t  num_levels;
    int32_t  generators_total;
    int32_t  generators_destroyed;
    int32_t  gold;
    int32_t  num_creatures;
    int32_t  num_puzzles;
    uint32_t tick;
    int32_t  selected_droid;
} SaveHeader;

bool save_game(const GameState *gs, const CreatureList *creatures,
               const PuzzleList *puzzles, const char *path) {
    if (!gs || !creatures || !puzzles || !path || gs->game_type != GAME_CAPTIVE ||
        gs->mission <= 0 || gs->base_id < 0 ||
        gs->num_levels < 1 || gs->num_levels > MAX_LEVELS ||
        gs->current_level < 0 || gs->current_level >= gs->num_levels ||
        gs->party_x < 0 || gs->party_x >= MAP_WIDTH ||
        gs->party_y < 0 || gs->party_y >= MAP_HEIGHT ||
        gs->party_dir < DIR_NORTH || gs->party_dir > DIR_WEST ||
        gs->selected_droid < 0 || gs->selected_droid >= 4 ||
        gs->generators_total < 0 ||
        gs->generators_destroyed < 0 ||
        gs->generators_destroyed > gs->generators_total || gs->gold < 0) return false;
    if (creatures->num_creatures < 0 || creatures->num_creatures > MAX_CREATURES ||
        puzzles->num_puzzles < 0 || puzzles->num_puzzles > MAX_PUZZLES) return false;
    for (int i = 0; i < creatures->num_creatures; i++) {
        const Creature *c = &creatures->creatures[i];
        if (c->type < CREATURE_NONE || c->type >= CREATURE_COUNT ||
            c->x < 0 || c->x >= MAP_WIDTH || c->y < 0 || c->y >= MAP_HEIGHT ||
            c->level < 0 || c->level >= gs->num_levels || c->hp < 0 ||
            c->hp_max < 0 || c->hp > c->hp_max || c->damage_min < 0 ||
            c->damage_max < c->damage_min || c->defense < 0)
            return false;
    }
    for (int i = 0; i < puzzles->num_puzzles; i++) {
        const Puzzle *p = &puzzles->puzzles[i];
        if (p->type < PUZZLE_NONE || p->type > PUZZLE_WALL_ELECTRIC ||
            p->x < 0 || p->x >= MAP_WIDTH || p->y < 0 || p->y >= MAP_HEIGHT ||
            p->level < 0 || p->level >= gs->num_levels ||
            p->face < 0 || p->face > DIR_WEST ||
            p->target_x < -1 || p->target_x >= MAP_WIDTH ||
            p->target_y < -1 || p->target_y >= MAP_HEIGHT)
            return false;
    }
    for (int i = 0; i < 4; i++) {
        const Droid *d = &gs->droids[i];
        if (d->hp < 0 || d->hp_max < 0 || d->hp > d->hp_max ||
            d->energy < 0 || d->energy_max < 0 || d->energy > d->energy_max)
            return false;
    }
    for (int level = 0; level < gs->num_levels; level++) {
        for (int y = 0; y < MAP_HEIGHT; y++) {
            for (int x = 0; x < MAP_WIDTH; x++) {
                CellType type = gs->levels[level].cells[y][x].type;
                if (type < CELL_WALL || type > CELL_PIT) return false;
            }
        }
    }
    FILE *f = fopen(path, "wb");
    if (!f) return false;

    SaveHeader hdr = {
        .magic = SAVE_MAGIC,
        .version = SAVE_VERSION,
        .game_type = gs->game_type,
        .mission = gs->mission,
        .mission_seed = gs->mission_seed,
        .base_id = gs->base_id,
        .party_x = gs->party_x,
        .party_y = gs->party_y,
        .party_dir = gs->party_dir,
        .current_level = gs->current_level,
        .num_levels = gs->num_levels,
        .generators_total = gs->generators_total,
        .generators_destroyed = gs->generators_destroyed,
        .gold = gs->gold,
        .num_creatures = creatures->num_creatures,
        .num_puzzles = puzzles->num_puzzles,
        .tick = gs->tick,
        .selected_droid = gs->selected_droid,
    };

    bool ok = fwrite(&hdr, sizeof(hdr), 1, f) == 1 &&
              fwrite(gs->droids, sizeof(gs->droids), 1, f) == 1;

    // Save dungeon state (doors opened, generators destroyed, etc.)
    for (int i = 0; i < gs->num_levels; i++) {
        for (int y = 0; y < MAP_HEIGHT; y++) {
            for (int x = 0; x < MAP_WIDTH; x++) {
                uint8_t cell_state[2] = {
                    (uint8_t)gs->levels[i].cells[y][x].type,
                    gs->levels[i].cells[y][x].item_id
                };
                if (fwrite(cell_state, 1, sizeof(cell_state), f) != sizeof(cell_state))
                    ok = false;
            }
        }
    }

    if (fwrite(creatures->creatures, sizeof(Creature),
               (size_t)creatures->num_creatures, f) != (size_t)creatures->num_creatures ||
        fwrite(puzzles->puzzles, sizeof(Puzzle),
               (size_t)puzzles->num_puzzles, f) != (size_t)puzzles->num_puzzles)
        ok = false;

    if (fclose(f) != 0) ok = false;
    return ok;
}

bool load_game(GameState *gs, CreatureList *creatures, PuzzleList *puzzles,
               const char *path) {
    if (!gs || !creatures || !puzzles || !path) return false;
    FILE *f = fopen(path, "rb");
    if (!f) return false;

    SaveHeader hdr;
    if (fread(&hdr, sizeof(hdr), 1, f) != 1) { fclose(f); return false; }
    if (hdr.magic != SAVE_MAGIC ||
        (hdr.version != SAVE_VERSION && hdr.version != SAVE_VERSION_LEGACY) ||
        hdr.game_type != GAME_CAPTIVE || hdr.mission == 0 ||
        hdr.mission > INT_MAX || hdr.base_id > INT_MAX ||
        hdr.party_x < 0 || hdr.party_x >= MAP_WIDTH ||
        hdr.party_y < 0 || hdr.party_y >= MAP_HEIGHT ||
        hdr.party_dir > DIR_WEST || hdr.current_level < 0 ||
        hdr.current_level >= MAX_LEVELS || hdr.num_levels < 1 ||
        hdr.num_levels > MAX_LEVELS || hdr.generators_total < 0 ||
        hdr.generators_destroyed < 0 ||
        hdr.generators_destroyed > hdr.generators_total ||
        hdr.gold < 0 ||
        hdr.num_creatures < 0 || hdr.num_creatures > MAX_CREATURES ||
        hdr.num_puzzles < 0 || hdr.num_puzzles > MAX_PUZZLES ||
        hdr.selected_droid < 0 || hdr.selected_droid >= 4) {
        fclose(f);
        return false;
    }

    // Build a replacement state first.  A truncated or invalid save must not
    // damage the game currently in memory.
    GameState *restored = calloc(1, sizeof(*restored));
    if (!restored) {
        fclose(f);
        return false;
    }
    CreatureList restored_creatures = {0};
    PuzzleList restored_puzzles = {0};
#define LOAD_FAIL() do { fclose(f); free(restored); return false; } while (0)
    game_state_init(restored, GAME_CAPTIVE, (int)hdr.mission);
    restored->base_id = (int)hdr.base_id;
    game_state_new_mission(restored, (int)hdr.mission);
    if (restored->mission_seed != hdr.mission_seed ||
        restored->num_levels != hdr.num_levels ||
        hdr.current_level >= restored->num_levels)
        LOAD_FAIL();

    restored->party_x = hdr.party_x;
    restored->party_y = hdr.party_y;
    restored->party_dir = (Direction)hdr.party_dir;
    restored->current_level = hdr.current_level;
    restored->generators_total = hdr.generators_total;
    restored->generators_destroyed = hdr.generators_destroyed;
    restored->gold = hdr.gold;
    restored->tick = hdr.tick;
    restored->selected_droid = hdr.selected_droid;

    if (fread(restored->droids, sizeof(restored->droids), 1, f) != 1)
        LOAD_FAIL();
    for (size_t i = 0; i < sizeof(restored->droids) / sizeof(restored->droids[0]); ++i) {
        restored->droids[i].name[sizeof(restored->droids[i].name) - 1] = '\0';
        const Droid *droid = &restored->droids[i];
        if (droid->hp < 0 || droid->hp > droid->hp_max ||
            droid->hp_max < 0 || droid->energy < 0 ||
            droid->energy > droid->energy_max || droid->energy_max < 0)
            LOAD_FAIL();
    }

    // Restore cell types (doors opened etc.)
    for (int i = 0; i < restored->num_levels; i++) {
        for (int y = 0; y < MAP_HEIGHT; y++) {
            for (int x = 0; x < MAP_WIDTH; x++) {
                uint8_t type;
                if (fread(&type, 1, 1, f) != 1 || type > CELL_PIT)
                    LOAD_FAIL();
                restored->levels[i].cells[y][x].type = (CellType)type;
                if (hdr.version >= SAVE_VERSION) {
                    if (fread(&restored->levels[i].cells[y][x].item_id, 1, 1, f) != 1)
                        LOAD_FAIL();
                }
            }
        }
    }

    restored_creatures.num_creatures = hdr.num_creatures;
    restored_puzzles.num_puzzles = hdr.num_puzzles;
    if (fread(restored_creatures.creatures, sizeof(Creature),
              (size_t)hdr.num_creatures, f) != (size_t)hdr.num_creatures ||
        fread(restored_puzzles.puzzles, sizeof(Puzzle),
              (size_t)hdr.num_puzzles, f) != (size_t)hdr.num_puzzles) {
        LOAD_FAIL();
    }
    for (int i = 0; i < restored_creatures.num_creatures; i++) {
        const Creature *creature = &restored_creatures.creatures[i];
        if (creature->type < CREATURE_NONE || creature->type >= CREATURE_COUNT ||
            creature->x < 0 || creature->x >= MAP_WIDTH ||
            creature->y < 0 || creature->y >= MAP_HEIGHT ||
            creature->level < 0 || creature->level >= restored->num_levels ||
            creature->hp < 0 || creature->hp_max < 0 ||
            creature->hp > creature->hp_max || creature->damage_min < 0 ||
            creature->damage_max < creature->damage_min ||
            creature->defense < 0) {
            LOAD_FAIL();
        }
    }
    for (int i = 0; i < restored_puzzles.num_puzzles; i++) {
        const Puzzle *puzzle = &restored_puzzles.puzzles[i];
        if (puzzle->type < PUZZLE_NONE || puzzle->type > PUZZLE_WALL_ELECTRIC ||
            puzzle->x < 0 || puzzle->x >= MAP_WIDTH ||
            puzzle->y < 0 || puzzle->y >= MAP_HEIGHT ||
            puzzle->level < 0 || puzzle->level >= restored->num_levels ||
            puzzle->face < 0 || puzzle->face > DIR_WEST ||
            puzzle->target_x < -1 || puzzle->target_x >= MAP_WIDTH ||
            puzzle->target_y < -1 || puzzle->target_y >= MAP_HEIGHT) {
            LOAD_FAIL();
        }
    }

    /* The format has no extension area.  Reject trailing bytes so a
     * concatenated or mismatched save cannot be accepted as valid state. */
    long payload_end = ftell(f);
    if (payload_end < 0 || fseek(f, 0, SEEK_END) != 0) {
        LOAD_FAIL();
    }
    long file_end = ftell(f);
    if (file_end < 0 || payload_end != file_end) {
        LOAD_FAIL();
    }
    if (fclose(f) != 0) {
        free(restored);
        return false;
    }
    restored->mode = STATE_GAME;
    *gs = *restored;
    free(restored);
    *creatures = restored_creatures;
    *puzzles = restored_puzzles;
#undef LOAD_FAIL
    return true;
}
