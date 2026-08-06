#include "custom_features.h"
#include "game_state.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#endif

#define CROSS_SAVE_MAGIC 0x4F435356 /* "OCSV" */
#define CROSS_SAVE_VERSION 2
#define CROSS_SAVE_VERSION_LEGACY 1
#define CROSS_SAVE_DROID_V1_SIZE 58U
#define CROSS_SAVE_DROID_V2_SIZE 64U

static bool read_exact(FILE *fp, void *dst, size_t size, size_t count);
static bool write_exact(FILE *fp, const void *src, size_t size, size_t count);

/* Cross-save is explicitly portable. Do not write native integer object
 * representations: the old implementation happened to work on the current
 * little-endian runners but produced unreadable files on big-endian hosts. */
static bool write_le16(FILE *fp, uint16_t value) {
    uint8_t bytes[2] = { (uint8_t)value, (uint8_t)(value >> 8) };
    return write_exact(fp, bytes, 1, sizeof(bytes));
}

static bool write_le32(FILE *fp, uint32_t value) {
    uint8_t bytes[4] = { (uint8_t)value, (uint8_t)(value >> 8),
                         (uint8_t)(value >> 16), (uint8_t)(value >> 24) };
    return write_exact(fp, bytes, 1, sizeof(bytes));
}

static bool read_le16(FILE *fp, uint16_t *value) {
    uint8_t bytes[2];
    if (!read_exact(fp, bytes, 1, sizeof(bytes))) return false;
    *value = (uint16_t)bytes[0] | ((uint16_t)bytes[1] << 8);
    return true;
}

static bool read_le32(FILE *fp, uint32_t *value) {
    uint8_t bytes[4];
    if (!read_exact(fp, bytes, 1, sizeof(bytes))) return false;
    *value = (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8) |
             ((uint32_t)bytes[2] << 16) | ((uint32_t)bytes[3] << 24);
    return true;
}

static int16_t decode_i16(uint16_t raw) {
    if (raw <= (uint16_t)INT16_MAX) return (int16_t)raw;
    if (raw == UINT16_C(0x8000)) return INT16_MIN;
    return (int16_t)-(int)(UINT32_C(0x10000) - raw);
}

static bool read_exact(FILE *fp, void *dst, size_t size, size_t count) {
    return fp && dst && fread(dst, size, count, fp) == count;
}

static bool write_exact(FILE *fp, const void *src, size_t size, size_t count) {
    return fp && src && fwrite(src, size, count, fp) == count;
}

static bool replace_cross_save(const char *temporary_path, const char *path) {
#ifdef _WIN32
    return MoveFileExA(temporary_path, path,
                       MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
    return rename(temporary_path, path) == 0;
#endif
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
    size_t path_length = strlen(path);
    if (path_length > SIZE_MAX - 5) return false;
    char *temporary_path = malloc(path_length + 5);
    if (!temporary_path) return false;
    snprintf(temporary_path, path_length + 5, "%s.tmp", path);
    FILE *fp = fopen(temporary_path, "wb");
    if (!fp) {
        free(temporary_path);
        return false;
    }

#define WRITE_OR_FAIL(src, size, count) \
    do { \
        if (!write_exact(fp, (src), (size), (count))) { \
            fclose(fp); \
            remove(temporary_path); \
            free(temporary_path); \
            return false; \
        } \
    } while (0)

    if (!write_le32(fp, CROSS_SAVE_MAGIC) ||
        !write_le32(fp, CROSS_SAVE_VERSION)) {
        fclose(fp);
        remove(temporary_path);
        free(temporary_path);
        return false;
    }

    uint8_t game_type = (uint8_t)gs->game_type;
    WRITE_OR_FAIL(&game_type, 1, 1);

    int32_t vals[] = {
        gs->party_x, gs->party_y, gs->party_dir,
        gs->current_level, gs->mission,
        gs->num_levels, gs->base_id,
        gs->generators_total, gs->generators_destroyed, gs->gold
    };
    for (size_t i = 0; i < sizeof(vals) / sizeof(vals[0]); i++)
        if (!write_le32(fp, (uint32_t)vals[i])) {
            fclose(fp);
            remove(temporary_path);
            free(temporary_path);
            return false;
        }
    if (!write_le32(fp, gs->mission_seed) || !write_le32(fp, gs->tick)) {
        fclose(fp);
        remove(temporary_path);
        free(temporary_path);
        return false;
    }

    for (int d = 0; d < 4; d++) {
        const Droid *dr = &gs->droids[d];
        WRITE_OR_FAIL(dr->name, 16, 1);
        if (!write_le16(fp, (uint16_t)dr->hp) ||
            !write_le16(fp, (uint16_t)dr->hp_max) ||
            !write_le16(fp, (uint16_t)dr->energy) ||
            !write_le16(fp, (uint16_t)dr->energy_max)) {
            fclose(fp);
            remove(temporary_path);
            free(temporary_path);
            return false;
        }
        WRITE_OR_FAIL(dr->body_parts, 6, 1);
        WRITE_OR_FAIL(dr->body_part_hp, 6, 1);
        WRITE_OR_FAIL(dr->weapons, 2, 1);
        WRITE_OR_FAIL(dr->items, 10, 1);
        WRITE_OR_FAIL(dr->skill_levels, 10, 1);
        if (!write_le32(fp, dr->xp) ||
            !write_le16(fp, dr->weapon_damage)) {
            fclose(fp);
            remove(temporary_path);
            free(temporary_path);
            return false;
        }
    }

    for (int l = 0; l < gs->num_levels && l < MAX_LEVELS; l++) {
        const DungeonLevel *lvl = &gs->levels[l];
        if (!write_le32(fp, (uint32_t)lvl->level) ||
            !write_le32(fp, lvl->seed)) {
            fclose(fp);
            remove(temporary_path);
            free(temporary_path);
            return false;
        }
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
    if (!ok) {
        remove(temporary_path);
        free(temporary_path);
        return false;
    }
    ok = replace_cross_save(temporary_path, path);
    if (!ok) remove(temporary_path);
    free(temporary_path);
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
    if (!read_le32(fp, &magic) || magic != CROSS_SAVE_MAGIC) {
        IMPORT_FAIL();
    }
    if (!read_le32(fp, &version) ||
        (version != CROSS_SAVE_VERSION && version != CROSS_SAVE_VERSION_LEGACY)) {
        IMPORT_FAIL();
    }

    uint8_t game_type;
    if (fread(&game_type, 1, 1, fp) != 1 || game_type != GAME_CAPTIVE) {
        IMPORT_FAIL();
    }

    uint32_t raw_vals[10];
    int32_t vals[10];
    bool vals_fit = true;
    for (size_t i = 0; i < sizeof(raw_vals) / sizeof(raw_vals[0]); i++) {
        if (!read_le32(fp, &raw_vals[i]) || raw_vals[i] > INT32_MAX) {
            vals_fit = false;
            break;
        }
        vals[i] = (int32_t)raw_vals[i];
    }
    if (!vals_fit ||
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
    if (!read_le32(fp, &gs->mission_seed) ||
        !read_le32(fp, &gs->tick)) {
        IMPORT_FAIL();
    }

    long payload_start = ftell(fp);
    if (payload_start < 0 || fseek(fp, 0, SEEK_END) != 0) {
        IMPORT_FAIL();
    }
    long file_end = ftell(fp);
    long long level_bytes = (long long)vals[5] *
                            (8LL + (long long)MAP_WIDTH * MAP_HEIGHT * 10);
    uint32_t droid_record_size = version == CROSS_SAVE_VERSION
        ? CROSS_SAVE_DROID_V2_SIZE : CROSS_SAVE_DROID_V1_SIZE;
    long long required_end = (long long)payload_start +
                             4LL * droid_record_size + level_bytes;
    if (file_end < 0 || (long long)file_end < required_end ||
        fseek(fp, payload_start, SEEK_SET) != 0) {
        IMPORT_FAIL();
    }

    for (int d = 0; d < 4; d++) {
        Droid *dr = &gs->droids[d];
        uint16_t hp, hp_max, energy, energy_max, weapon_damage;
        if (!read_exact(fp, dr->name, 16, 1) ||
            !read_le16(fp, &hp) || !read_le16(fp, &hp_max) ||
            !read_le16(fp, &energy) || !read_le16(fp, &energy_max) ||
            !read_exact(fp, dr->body_parts, 6, 1) ||
            (version == CROSS_SAVE_VERSION &&
             !read_exact(fp, dr->body_part_hp, 6, 1)) ||
            !read_exact(fp, dr->weapons, 2, 1) ||
            !read_exact(fp, dr->items, 10, 1) ||
            !read_exact(fp, dr->skill_levels, 10, 1) ||
            !read_le32(fp, &dr->xp) || !read_le16(fp, &weapon_damage)) {
            IMPORT_FAIL();
        }
        if (version == CROSS_SAVE_VERSION_LEGACY)
            memset(dr->body_part_hp, UINT8_MAX, sizeof(dr->body_part_hp));
        dr->hp = decode_i16(hp);
        dr->hp_max = decode_i16(hp_max);
        dr->energy = decode_i16(energy);
        dr->energy_max = decode_i16(energy_max);
        dr->weapon_damage = weapon_damage;
        if (dr->hp < 0 || dr->hp_max < 0 || dr->hp > dr->hp_max ||
            dr->energy < 0 || dr->energy_max < 0 || dr->energy > dr->energy_max) {
            IMPORT_FAIL();
        }
        dr->name[sizeof(dr->name) - 1] = '\0';
    }

    for (int l = 0; l < gs->num_levels && l < MAX_LEVELS; l++) {
        DungeonLevel *lvl = &gs->levels[l];
        uint32_t level, seed;
        if (!read_le32(fp, &level) || level > INT32_MAX ||
            !read_le32(fp, &seed) || level >= MAX_LEVELS ||
            level != (uint32_t)l) {
            IMPORT_FAIL();
        }
        lvl->level = (int)level;
        lvl->seed = seed;
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
