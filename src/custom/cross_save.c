#include "custom_features.h"
#include "game_state.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CROSS_SAVE_MAGIC 0x4F435356 /* "OCSV" */
#define CROSS_SAVE_VERSION 1

static bool read_exact(FILE *fp, void *dst, size_t size, size_t count) {
    return fp && dst && fread(dst, size, count, fp) == count;
}

static bool write_exact(FILE *fp, const void *src, size_t size, size_t count) {
    return fp && src && fwrite(src, size, count, fp) == count;
}

bool cross_save_export(const void *game_state_ptr, const char *path) {
    const GameState *gs = (const GameState *)game_state_ptr;
    /* Version 1 stores dungeon/party state only.  Liberation uses a
     * different city-navigation state and must not be advertised as
     * exportable through this Captive-only format. */
    if (!gs || !path || gs->game_type != GAME_CAPTIVE ||
        gs->num_levels < 1 || gs->num_levels > MAX_LEVELS ||
        gs->current_level < 0 || gs->current_level >= gs->num_levels ||
        gs->party_x < 0 || gs->party_x >= MAP_WIDTH ||
        gs->party_y < 0 || gs->party_y >= MAP_HEIGHT ||
        gs->party_dir < DIR_NORTH || gs->party_dir > DIR_WEST || gs->mission < 1 ||
        gs->base_id < 0 || gs->gold < 0 ||
        gs->generators_total < 0 || gs->generators_destroyed < 0 ||
        gs->generators_destroyed > gs->generators_total) return false;
    for (int d = 0; d < 4; d++) {
        const Droid *dr = &gs->droids[d];
        if (dr->hp < 0 || dr->hp_max < 0 || dr->hp > dr->hp_max ||
            dr->energy < 0 || dr->energy_max < 0 ||
            dr->energy > dr->energy_max) return false;
    }
    for (int l = 0; l < gs->num_levels; l++) {
        for (int y = 0; y < MAP_HEIGHT; y++) {
            for (int x = 0; x < MAP_WIDTH; x++) {
                if (gs->levels[l].cells[y][x].type < CELL_WALL ||
                    gs->levels[l].cells[y][x].type > CELL_PIT) return false;
            }
        }
    }
    FILE *fp = fopen(path, "wb");
    if (!fp) return false;

#define WRITE_OR_FAIL(src, size, count) \
    do { \
        if (!write_exact(fp, (src), (size), (count))) { \
            fclose(fp); \
            return false; \
        } \
    } while (0)

    uint32_t magic = CROSS_SAVE_MAGIC;
    uint32_t version = CROSS_SAVE_VERSION;
    WRITE_OR_FAIL(&magic, 4, 1);
    WRITE_OR_FAIL(&version, 4, 1);

    uint8_t game_type = (uint8_t)gs->game_type;
    WRITE_OR_FAIL(&game_type, 1, 1);

    int32_t vals[] = {
        gs->party_x, gs->party_y, gs->party_dir,
        gs->current_level, gs->mission,
        gs->num_levels, gs->base_id,
        gs->generators_total, gs->generators_destroyed, gs->gold
    };
    WRITE_OR_FAIL(vals, sizeof(vals), 1);
    WRITE_OR_FAIL(&gs->mission_seed, 4, 1);
    WRITE_OR_FAIL(&gs->tick, 4, 1);

    for (int d = 0; d < 4; d++) {
        const Droid *dr = &gs->droids[d];
        WRITE_OR_FAIL(dr->name, 16, 1);
        WRITE_OR_FAIL(&dr->hp, 2, 1);
        WRITE_OR_FAIL(&dr->hp_max, 2, 1);
        WRITE_OR_FAIL(&dr->energy, 2, 1);
        WRITE_OR_FAIL(&dr->energy_max, 2, 1);
        WRITE_OR_FAIL(dr->body_parts, 6, 1);
        WRITE_OR_FAIL(dr->weapons, 2, 1);
        WRITE_OR_FAIL(dr->items, 10, 1);
        WRITE_OR_FAIL(dr->skill_levels, 10, 1);
        WRITE_OR_FAIL(&dr->xp, 4, 1);
        WRITE_OR_FAIL(&dr->weapon_damage, 2, 1);
    }

    for (int l = 0; l < gs->num_levels && l < MAX_LEVELS; l++) {
        const DungeonLevel *lvl = &gs->levels[l];
        WRITE_OR_FAIL(&lvl->level, 4, 1);
        WRITE_OR_FAIL(&lvl->seed, 4, 1);
        for (int y = 0; y < MAP_HEIGHT; y++) {
            for (int x = 0; x < MAP_WIDTH; x++) {
                const MapCell *c = &lvl->cells[y][x];
                uint8_t ct = (uint8_t)c->type;
                WRITE_OR_FAIL(&ct, 1, 1);
                WRITE_OR_FAIL(c->wall_tex, 4, 1);
                WRITE_OR_FAIL(&c->floor_tex, 1, 1);
                WRITE_OR_FAIL(&c->ceil_tex, 1, 1);
                WRITE_OR_FAIL(&c->item_id, 1, 1);
                WRITE_OR_FAIL(&c->creature_id, 1, 1);
                WRITE_OR_FAIL(&c->flags, 1, 1);
            }
        }
    }

    bool ok = fclose(fp) == 0;
#undef WRITE_OR_FAIL
    return ok;
}

bool cross_save_import(void *game_state_ptr, const char *path) {
    GameState *destination = (GameState *)game_state_ptr;
    if (!destination || !path) return false;
    /* Runtime/display options are deliberately not serialized. Preserve the
     * active session configuration while importing the gameplay snapshot. */
    OpenCaptiveConfig active_config = destination->config;
    /* Import reconstructs every serialized field.  Do not copy the caller's
     * possibly uninitialised state before validation; failed imports leave
     * the destination untouched and successful imports receive deterministic
     * values for fields not present in the cross-save format. */
    FILE *fp = fopen(path, "rb");
    if (!fp) return false;
    GameState *gs = calloc(1, sizeof(*gs));
    if (!gs) {
        fclose(fp);
        return false;
    }
#define IMPORT_FAIL() do { fclose(fp); free(gs); return false; } while (0)

    uint32_t magic, version;
    if (fread(&magic, 4, 1, fp) != 1 || magic != CROSS_SAVE_MAGIC) {
        IMPORT_FAIL();
    }
    if (fread(&version, 4, 1, fp) != 1 || version != CROSS_SAVE_VERSION) {
        IMPORT_FAIL();
    }

    uint8_t game_type;
    if (fread(&game_type, 1, 1, fp) != 1 || game_type != GAME_CAPTIVE) {
        IMPORT_FAIL();
    }

    int32_t vals[10];
    if (fread(vals, sizeof(vals), 1, fp) != 1 ||
        vals[0] < 0 || vals[0] >= MAP_WIDTH || vals[1] < 0 || vals[1] >= MAP_HEIGHT ||
        vals[2] < DIR_NORTH || vals[2] > DIR_WEST ||
        vals[3] < 0 || vals[3] >= MAX_LEVELS || vals[4] < 1 ||
        vals[5] < 1 || vals[5] > MAX_LEVELS || vals[3] >= vals[5] ||
        vals[6] < 0 || vals[9] < 0 ||
        vals[7] < 0 || vals[8] < 0 || vals[8] > vals[7]) {
        IMPORT_FAIL();
    }
    gs->game_type = (GameType)game_type;
    gs->party_x = vals[0];
    gs->party_y = vals[1];
    gs->party_dir = (Direction)vals[2];
    gs->current_level = vals[3];
    gs->mission = vals[4];
    gs->num_levels = vals[5];
    gs->base_id = vals[6];
    gs->generators_total = vals[7];
    gs->generators_destroyed = vals[8];
    gs->gold = vals[9];
    if (!read_exact(fp, &gs->mission_seed, 4, 1) ||
        !read_exact(fp, &gs->tick, 4, 1)) {
        IMPORT_FAIL();
    }

    long payload_start = ftell(fp);
    if (payload_start < 0 || fseek(fp, 0, SEEK_END) != 0) {
        IMPORT_FAIL();
    }
    long file_end = ftell(fp);
    long long level_bytes = (long long)vals[5] *
                            (8LL + (long long)MAP_WIDTH * MAP_HEIGHT * 10);
    long long required_end = (long long)payload_start + 4LL * 58 + level_bytes;
    if (file_end < 0 || (long long)file_end < required_end ||
        fseek(fp, payload_start, SEEK_SET) != 0) {
        IMPORT_FAIL();
    }

    for (int d = 0; d < 4; d++) {
        Droid *dr = &gs->droids[d];
        if (!read_exact(fp, dr->name, 16, 1) ||
            !read_exact(fp, &dr->hp, 2, 1) ||
            !read_exact(fp, &dr->hp_max, 2, 1) ||
            !read_exact(fp, &dr->energy, 2, 1) ||
            !read_exact(fp, &dr->energy_max, 2, 1) ||
            !read_exact(fp, dr->body_parts, 6, 1) ||
            !read_exact(fp, dr->weapons, 2, 1) ||
            !read_exact(fp, dr->items, 10, 1) ||
            !read_exact(fp, dr->skill_levels, 10, 1) ||
            !read_exact(fp, &dr->xp, 4, 1) ||
            !read_exact(fp, &dr->weapon_damage, 2, 1) ||
            dr->hp < 0 || dr->hp_max < 0 || dr->hp > dr->hp_max ||
            dr->energy < 0 || dr->energy_max < 0 || dr->energy > dr->energy_max) {
            IMPORT_FAIL();
        }
        dr->name[sizeof(dr->name) - 1] = '\0';
    }

    for (int l = 0; l < gs->num_levels && l < MAX_LEVELS; l++) {
        DungeonLevel *lvl = &gs->levels[l];
        if (!read_exact(fp, &lvl->level, 4, 1) ||
            !read_exact(fp, &lvl->seed, 4, 1) ||
            lvl->level < 0 || lvl->level >= MAX_LEVELS) {
            IMPORT_FAIL();
        }
        for (int y = 0; y < MAP_HEIGHT; y++) {
            for (int x = 0; x < MAP_WIDTH; x++) {
                MapCell *c = &lvl->cells[y][x];
                uint8_t ct;
                if (!read_exact(fp, &ct, 1, 1) || ct > CELL_PIT ||
                    !read_exact(fp, c->wall_tex, 1, 4) ||
                    !read_exact(fp, &c->floor_tex, 1, 1) ||
                    !read_exact(fp, &c->ceil_tex, 1, 1) ||
                    !read_exact(fp, &c->item_id, 1, 1) ||
                    !read_exact(fp, &c->creature_id, 1, 1) ||
                    !read_exact(fp, &c->flags, 1, 1)) {
                    IMPORT_FAIL();
                }
                c->type = (CellType)ct;
            }
        }
    }

    long payload_end = ftell(fp);
    if (payload_end < 0 || payload_end != file_end) {
        IMPORT_FAIL();
    }
    if (fclose(fp) != 0) {
        free(gs);
        return false;
    }
    gs->mode = STATE_GAME;
    gs->config = active_config;
    *destination = *gs;
    free(gs);
#undef IMPORT_FAIL
    return true;
}
