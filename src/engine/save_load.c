#include "save_load.h"
#include <stdio.h>
#include <string.h>

#define SAVE_MAGIC 0x4F435356  // "OCSV" - OpenCaptive Save
#define SAVE_VERSION 1

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
    uint32_t tick;
    int32_t  selected_droid;
} SaveHeader;

bool save_game(const GameState *gs, const char *path) {
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
        .tick = gs->tick,
        .selected_droid = gs->selected_droid,
    };

    fwrite(&hdr, sizeof(hdr), 1, f);
    fwrite(gs->droids, sizeof(gs->droids), 1, f);

    // Save dungeon state (doors opened, generators destroyed, etc.)
    for (int i = 0; i < gs->num_levels; i++) {
        for (int y = 0; y < MAP_HEIGHT; y++) {
            for (int x = 0; x < MAP_WIDTH; x++) {
                uint8_t type = gs->levels[i].cells[y][x].type;
                fwrite(&type, 1, 1, f);
            }
        }
    }

    fclose(f);
    return true;
}

bool load_game(GameState *gs, const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return false;

    SaveHeader hdr;
    if (fread(&hdr, sizeof(hdr), 1, f) != 1) { fclose(f); return false; }
    if (hdr.magic != SAVE_MAGIC || hdr.version != SAVE_VERSION) {
        fclose(f);
        return false;
    }

    // Re-generate maps from seed, then overlay saved cell states
    game_state_init(gs, (GameType)hdr.game_type, hdr.mission);
    gs->mission_seed = hdr.mission_seed;
    gs->base_id = hdr.base_id;
    game_state_new_mission(gs, hdr.mission);

    gs->party_x = hdr.party_x;
    gs->party_y = hdr.party_y;
    gs->party_dir = (Direction)hdr.party_dir;
    gs->current_level = hdr.current_level;
    gs->generators_total = hdr.generators_total;
    gs->generators_destroyed = hdr.generators_destroyed;
    gs->tick = hdr.tick;
    gs->selected_droid = hdr.selected_droid;

    if (fread(gs->droids, sizeof(gs->droids), 1, f) != 1) {
        fclose(f);
        return false;
    }

    // Restore cell types (doors opened etc.)
    for (int i = 0; i < gs->num_levels; i++) {
        for (int y = 0; y < MAP_HEIGHT; y++) {
            for (int x = 0; x < MAP_WIDTH; x++) {
                uint8_t type;
                if (fread(&type, 1, 1, f) != 1) { fclose(f); return false; }
                gs->levels[i].cells[y][x].type = (CellType)type;
            }
        }
    }

    fclose(f);
    gs->mode = STATE_GAME;
    return true;
}
