#include "save_load.h"
#include <limits.h>
#include <stdio.h>
#include <string.h>

#define SAVE_MAGIC 0x4F435356  // "OCSV" - OpenCaptive Save
#define SAVE_VERSION 3

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
        gs->num_levels < 1 || gs->num_levels > MAX_LEVELS) return false;
    if (creatures->num_creatures < 0 || creatures->num_creatures > MAX_CREATURES ||
        puzzles->num_puzzles < 0 || puzzles->num_puzzles > MAX_PUZZLES) return false;
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
                uint8_t type = gs->levels[i].cells[y][x].type;
                if (fwrite(&type, 1, 1, f) != 1) ok = false;
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
    if (hdr.magic != SAVE_MAGIC || hdr.version != SAVE_VERSION ||
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
    GameState restored;
    CreatureList restored_creatures = {0};
    PuzzleList restored_puzzles = {0};
    game_state_init(&restored, GAME_CAPTIVE, (int)hdr.mission);
    restored.base_id = (int)hdr.base_id;
    game_state_new_mission(&restored, (int)hdr.mission);
    if (restored.mission_seed != hdr.mission_seed ||
        restored.num_levels != hdr.num_levels ||
        hdr.current_level >= restored.num_levels) {
        fclose(f);
        return false;
    }

    restored.party_x = hdr.party_x;
    restored.party_y = hdr.party_y;
    restored.party_dir = (Direction)hdr.party_dir;
    restored.current_level = hdr.current_level;
    restored.generators_total = hdr.generators_total;
    restored.generators_destroyed = hdr.generators_destroyed;
    restored.gold = hdr.gold;
    restored.tick = hdr.tick;
    restored.selected_droid = hdr.selected_droid;

    if (fread(restored.droids, sizeof(restored.droids), 1, f) != 1) {
        fclose(f);
        return false;
    }

    // Restore cell types (doors opened etc.)
    for (int i = 0; i < restored.num_levels; i++) {
        for (int y = 0; y < MAP_HEIGHT; y++) {
            for (int x = 0; x < MAP_WIDTH; x++) {
                uint8_t type;
                if (fread(&type, 1, 1, f) != 1 || type > CELL_TERMINAL) {
                    fclose(f);
                    return false;
                }
                restored.levels[i].cells[y][x].type = (CellType)type;
            }
        }
    }

    restored_creatures.num_creatures = hdr.num_creatures;
    restored_puzzles.num_puzzles = hdr.num_puzzles;
    if (fread(restored_creatures.creatures, sizeof(Creature),
              (size_t)hdr.num_creatures, f) != (size_t)hdr.num_creatures ||
        fread(restored_puzzles.puzzles, sizeof(Puzzle),
              (size_t)hdr.num_puzzles, f) != (size_t)hdr.num_puzzles) {
        fclose(f);
        return false;
    }
    for (int i = 0; i < restored_creatures.num_creatures; i++) {
        const Creature *creature = &restored_creatures.creatures[i];
        if (creature->type < CREATURE_NONE || creature->type >= CREATURE_COUNT ||
            creature->x < 0 || creature->x >= MAP_WIDTH ||
            creature->y < 0 || creature->y >= MAP_HEIGHT ||
            creature->level < 0 || creature->level >= restored.num_levels ||
            creature->hp_max < 0 || creature->hp > creature->hp_max) {
            fclose(f);
            return false;
        }
    }
    for (int i = 0; i < restored_puzzles.num_puzzles; i++) {
        const Puzzle *puzzle = &restored_puzzles.puzzles[i];
        if (puzzle->type < PUZZLE_NONE || puzzle->type > PUZZLE_HIDDEN_BUTTON ||
            puzzle->x < 0 || puzzle->x >= MAP_WIDTH ||
            puzzle->y < 0 || puzzle->y >= MAP_HEIGHT ||
            puzzle->level < 0 || puzzle->level >= restored.num_levels ||
            puzzle->face < 0 || puzzle->face > DIR_WEST ||
            puzzle->target_x < -1 || puzzle->target_x >= MAP_WIDTH ||
            puzzle->target_y < -1 || puzzle->target_y >= MAP_HEIGHT) {
            fclose(f);
            return false;
        }
    }

    fclose(f);
    restored.mode = STATE_GAME;
    *gs = restored;
    *creatures = restored_creatures;
    *puzzles = restored_puzzles;
    return true;
}
