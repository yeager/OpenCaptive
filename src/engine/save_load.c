#include "save_load.h"
#include "save_thumbnail.h"
#include "inventory.h"
#ifdef _WIN32
#include <windows.h>
#endif
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SAVE_MAGIC 0x4F435356  // "OCSV" - OpenCaptive Save
#define SAVE_VERSION 5
#define SAVE_VERSION_RAW 4
#define SAVE_VERSION_LEGACY 3

/* Optional trailer appended after the v3/4/5 payload: a small preview of
 * the screen at the moment of saving, shown next to a save slot in menu
 * UI. It is intentionally NOT a member of SaveHeader -- that struct is
 * `fread` directly into for legacy version 3/4 files
 * (`(uint8_t *)hdr + 8`), and growing it by 16000 bytes would make that
 * raw copy read far past the end of an old, smaller save file. Keeping
 * the thumbnail as a distinct, self-describing trailer (written/read by
 * save_thumbnail.c) means saves without one (every file written before
 * this feature) still load unmodified: load_game() accepts either zero
 * trailing bytes or exactly one well-formed thumbnail block. */
#define SAVE_THUMB_MAGIC 0x424D5443u /* "CTMB", matches save_thumbnail.c */
#define SAVE_THUMB_BLOCK_SIZE (4 + (long)SAVE_THUMB_PIXELS * 4)

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

static bool write_u16_le(FILE *f, uint16_t value) {
    uint8_t bytes[2] = {(uint8_t)value, (uint8_t)(value >> 8)};
    return f && fwrite(bytes, 1, sizeof(bytes), f) == sizeof(bytes);
}

static bool write_u32_le(FILE *f, uint32_t value) {
    uint8_t bytes[4] = {(uint8_t)value, (uint8_t)(value >> 8),
                        (uint8_t)(value >> 16), (uint8_t)(value >> 24)};
    return f && fwrite(bytes, 1, sizeof(bytes), f) == sizeof(bytes);
}

static bool write_i16_le(FILE *f, int16_t value) {
    return write_u16_le(f, (uint16_t)value);
}

static bool write_i32_le(FILE *f, int32_t value) {
    return write_u32_le(f, (uint32_t)value);
}

static bool read_u16_le(FILE *f, uint16_t *value) {
    uint8_t bytes[2];
    if (!f || !value || fread(bytes, 1, sizeof(bytes), f) != sizeof(bytes)) return false;
    *value = (uint16_t)bytes[0] | ((uint16_t)bytes[1] << 8);
    return true;
}

static bool read_u32_le(FILE *f, uint32_t *value) {
    uint8_t bytes[4];
    if (!f || !value || fread(bytes, 1, sizeof(bytes), f) != sizeof(bytes)) return false;
    *value = (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8) |
             ((uint32_t)bytes[2] << 16) | ((uint32_t)bytes[3] << 24);
    return true;
}

static bool read_i16_le(FILE *f, int16_t *value) {
    uint16_t raw;
    if (!read_u16_le(f, &raw)) return false;
    *value = (int16_t)raw;
    return true;
}

static bool read_i32_le(FILE *f, int32_t *value) {
    uint32_t raw;
    if (!read_u32_le(f, &raw)) return false;
    *value = (int32_t)raw;
    return true;
}

static bool write_header_v5(FILE *f, const SaveHeader *hdr) {
    const uint32_t fields[] = {
        hdr->magic, hdr->version, hdr->game_type, hdr->mission,
        hdr->mission_seed, hdr->base_id, (uint32_t)hdr->party_x,
        (uint32_t)hdr->party_y, hdr->party_dir, (uint32_t)hdr->current_level,
        (uint32_t)hdr->num_levels, (uint32_t)hdr->generators_total,
        (uint32_t)hdr->generators_destroyed, (uint32_t)hdr->gold,
        (uint32_t)hdr->num_creatures, (uint32_t)hdr->num_puzzles,
        hdr->tick, (uint32_t)hdr->selected_droid,
    };
    for (size_t i = 0; i < sizeof(fields) / sizeof(fields[0]); ++i)
        if (!write_u32_le(f, fields[i])) return false;
    return true;
}

static bool read_header(FILE *f, SaveHeader *hdr) {
    uint32_t first[2];
    if (!read_u32_le(f, &first[0]) || !read_u32_le(f, &first[1])) return false;
    hdr->magic = first[0];
    hdr->version = first[1];
    if (hdr->version == SAVE_VERSION) {
        uint32_t fields[16];
        for (size_t i = 0; i < sizeof(fields) / sizeof(fields[0]); ++i)
            if (!read_u32_le(f, &fields[i])) return false;
        hdr->game_type = fields[0];
        hdr->mission = fields[1];
        hdr->mission_seed = fields[2];
        hdr->base_id = fields[3];
        hdr->party_x = (int32_t)fields[4];
        hdr->party_y = (int32_t)fields[5];
        hdr->party_dir = fields[6];
        hdr->current_level = (int32_t)fields[7];
        hdr->num_levels = (int32_t)fields[8];
        hdr->generators_total = (int32_t)fields[9];
        hdr->generators_destroyed = (int32_t)fields[10];
        hdr->gold = (int32_t)fields[11];
        hdr->num_creatures = (int32_t)fields[12];
        hdr->num_puzzles = (int32_t)fields[13];
        hdr->tick = fields[14];
        hdr->selected_droid = (int32_t)fields[15];
        return true;
    }
    /* Version 3/4 used the native SaveHeader layout.  Keep this compatibility
     * path for existing saves; new files never use it. */
    return fread((uint8_t *)hdr + sizeof(uint32_t) * 2,
                 1, sizeof(*hdr) - sizeof(uint32_t) * 2, f) ==
           sizeof(*hdr) - sizeof(uint32_t) * 2;
}

static bool write_droid_v5(FILE *f, const Droid *d) {
    return fwrite(d->name, 1, sizeof(d->name), f) == sizeof(d->name) &&
           write_i16_le(f, d->hp) && write_i16_le(f, d->hp_max) &&
           write_i16_le(f, d->energy) && write_i16_le(f, d->energy_max) &&
           fwrite(d->body_parts, 1, sizeof(d->body_parts), f) == sizeof(d->body_parts) &&
           fwrite(d->body_part_hp, 1, sizeof(d->body_part_hp), f) == sizeof(d->body_part_hp) &&
           fwrite(d->weapons, 1, sizeof(d->weapons), f) == sizeof(d->weapons) &&
           fwrite(d->items, 1, sizeof(d->items), f) == sizeof(d->items) &&
           fwrite(d->skill_levels, 1, sizeof(d->skill_levels), f) == sizeof(d->skill_levels) &&
           write_u32_le(f, d->xp) && write_u16_le(f, d->weapon_damage);
}

static bool read_droid_v5(FILE *f, Droid *d) {
    return fread(d->name, 1, sizeof(d->name), f) == sizeof(d->name) &&
           read_i16_le(f, &d->hp) && read_i16_le(f, &d->hp_max) &&
           read_i16_le(f, &d->energy) && read_i16_le(f, &d->energy_max) &&
           fread(d->body_parts, 1, sizeof(d->body_parts), f) == sizeof(d->body_parts) &&
           fread(d->body_part_hp, 1, sizeof(d->body_part_hp), f) == sizeof(d->body_part_hp) &&
           fread(d->weapons, 1, sizeof(d->weapons), f) == sizeof(d->weapons) &&
           fread(d->items, 1, sizeof(d->items), f) == sizeof(d->items) &&
           fread(d->skill_levels, 1, sizeof(d->skill_levels), f) == sizeof(d->skill_levels) &&
           read_u32_le(f, &d->xp) && read_u16_le(f, &d->weapon_damage);
}

static bool write_creature_v5(FILE *f, const Creature *c) {
    return write_u32_le(f, (uint32_t)c->type) &&
           write_i16_le(f, c->hp) && write_i16_le(f, c->hp_max) &&
           write_i16_le(f, c->damage_min) && write_i16_le(f, c->damage_max) &&
           write_i16_le(f, c->defense) && fwrite(&c->speed, 1, 1, f) == 1 &&
           fwrite(&c->range, 1, 1, f) == 1 && write_i32_le(f, c->x) &&
           write_i32_le(f, c->y) && write_i32_le(f, c->level) &&
           fwrite(&c->cooldown, 1, 1, f) == 1 && fwrite(&c->active, 1, 1, f) == 1 &&
           fwrite(&c->alerted, 1, 1, f) == 1 && write_u16_le(f, c->respawn_timer);
}

static bool read_creature_v5(FILE *f, Creature *c) {
    uint32_t type;
    uint8_t active, alerted;
    if (!read_u32_le(f, &type) || !read_i16_le(f, &c->hp) ||
        !read_i16_le(f, &c->hp_max) || !read_i16_le(f, &c->damage_min) ||
        !read_i16_le(f, &c->damage_max) || !read_i16_le(f, &c->defense) ||
        fread(&c->speed, 1, 1, f) != 1 || fread(&c->range, 1, 1, f) != 1 ||
        !read_i32_le(f, &c->x) || !read_i32_le(f, &c->y) ||
        !read_i32_le(f, &c->level) || fread(&c->cooldown, 1, 1, f) != 1 ||
        fread(&active, 1, 1, f) != 1 || fread(&alerted, 1, 1, f) != 1 ||
        !read_u16_le(f, &c->respawn_timer) || active > 1 || alerted > 1)
        return false;
    c->type = (CreatureType)type;
    c->active = active != 0;
    c->alerted = alerted != 0;
    return true;
}

static bool write_puzzle_v5(FILE *f, const Puzzle *p) {
    return write_u32_le(f, (uint32_t)p->type) && write_i32_le(f, p->x) &&
           write_i32_le(f, p->y) && write_i32_le(f, p->level) &&
           write_i32_le(f, p->face) && fwrite(&p->state, 1, 1, f) == 1 &&
           fwrite(&p->solution, 1, 1, f) == 1 && write_i32_le(f, p->target_x) &&
           write_i32_le(f, p->target_y) && fwrite(&p->solved, 1, 1, f) == 1;
}

static bool read_puzzle_v5(FILE *f, Puzzle *p) {
    uint32_t type;
    uint8_t solved;
    bool ok = read_u32_le(f, &type) && (p->type = (PuzzleType)type, true) &&
           read_i32_le(f, &p->x) && read_i32_le(f, &p->y) &&
           read_i32_le(f, &p->level) && read_i32_le(f, &p->face) &&
           fread(&p->state, 1, 1, f) == 1 && fread(&p->solution, 1, 1, f) == 1 &&
           read_i32_le(f, &p->target_x) && read_i32_le(f, &p->target_y) &&
           fread(&solved, 1, 1, f) == 1 && solved <= 1;
    if (ok) p->solved = solved != 0;
    return ok;
}

static bool valid_puzzle_target(const Puzzle *p) {
    if (!p) return false;
    if ((p->target_x == -1) != (p->target_y == -1)) return false;
    return (p->target_x == -1 && p->target_y == -1) ||
           (p->target_x >= 0 && p->target_x < MAP_WIDTH &&
            p->target_y >= 0 && p->target_y < MAP_HEIGHT);
}

static OrnamentType puzzle_ornament(PuzzleType type) {
    switch (type) {
        case PUZZLE_BUTTON: return ORNAMENT_BUTTON;
        case PUZZLE_BARS: return ORNAMENT_SCREEN;
        case PUZZLE_LEVER:
        case PUZZLE_TRIPLE_LEVER:
        case PUZZLE_BUTTON_COMBO:
        case PUZZLE_WALL_ELECTRIC: return ORNAMENT_PANEL;
        case PUZZLE_HIDDEN_BUTTON: return ORNAMENT_VENT;
        case PUZZLE_POWER_SOCKET: return ORNAMENT_PIPE;
        default: return ORNAMENT_NONE;
    }
}

static void restore_puzzle_ornaments(GameState *gs, const PuzzleList *puzzles) {
    if (!gs || !puzzles || puzzles->num_puzzles < 0 ||
        puzzles->num_puzzles > MAX_PUZZLES) return;
    for (int i = 0; i < puzzles->num_puzzles; i++) {
        const Puzzle *p = &puzzles->puzzles[i];
        OrnamentType ornament = puzzle_ornament(p->type);
        if (ornament == ORNAMENT_NONE || p->level < 0 ||
            p->level >= gs->num_levels || p->level >= MAX_LEVELS ||
            p->x < 0 || p->x >= MAP_WIDTH || p->y < 0 || p->y >= MAP_HEIGHT ||
            p->face < DIR_NORTH || p->face > DIR_WEST) continue;
        gs->levels[p->level].cells[p->y][p->x].ornament[p->face] = ornament;
    }
}

static bool valid_runtime_position(const GameState *gs, int level,
                                   int x, int y) {
    if (!gs || level < 0 || level >= gs->num_levels || level >= MAX_LEVELS ||
        x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT)
        return false;
    CellType type = gs->levels[level].cells[y][x].type;
    return type != CELL_WALL && type != CELL_DOOR &&
           type != CELL_DOOR_LOCKED;
}

static bool valid_creature_runtime_state(const Creature *c) {
    return c && c->speed >= 0 && c->speed <= 60 && c->range >= 0 &&
           c->range <= 7 && c->cooldown >= 0 && c->cooldown <= 60 &&
           c->respawn_timer >= 0 && c->respawn_timer <= 600 &&
           ((!c->active && c->hp == 0) ||
            (c->active && c->hp > 0));
}

static bool valid_creature_placement(const CreatureList *creatures, int index,
                                     int party_level, int party_x, int party_y) {
    if (!creatures || index < 0 || index >= creatures->num_creatures)
        return false;
    const Creature *current = &creatures->creatures[index];
    if (!current->active) return true;
    if (current->level == party_level && current->x == party_x &&
        current->y == party_y)
        return false;
    for (int other_index = 0; other_index < index; other_index++) {
        const Creature *other = &creatures->creatures[other_index];
        if (other->active && other->level == current->level &&
            other->x == current->x && other->y == current->y)
            return false;
    }
    return true;
}

static bool valid_item_id(const ItemDatabase *db, uint8_t item_id) {
    return item_id == 0 || (db && item_db_get(db, item_id) != NULL);
}

static bool valid_body_part_id(const ItemDatabase *db, size_t slot,
                               uint8_t item_id) {
    if (item_id == 0) return true;
    const Item *item = db ? item_db_get(db, item_id) : NULL;
    return item && slot < 6 &&
           item->category == (ItemCategory)(ITEM_ARMOR_HEAD + slot);
}

static bool valid_weapon_id(const ItemDatabase *db, uint8_t item_id) {
    if (item_id == 0) return true;
    const Item *item = db ? item_db_get(db, item_id) : NULL;
    return item && item->category >= ITEM_WEAPON_MELEE &&
           item->category <= ITEM_WEAPON_SPRAY;
}

static bool valid_droid_items(const ItemDatabase *db, const Droid *d) {
    if (!db || !d) return false;
    for (size_t i = 0; i < sizeof(d->body_parts); ++i)
        if (!valid_body_part_id(db, i, d->body_parts[i])) return false;
    for (size_t i = 0; i < sizeof(d->weapons); ++i)
        if (!valid_weapon_id(db, d->weapons[i])) return false;
    for (size_t i = 0; i < sizeof(d->items); ++i)
        if (!valid_item_id(db, d->items[i])) return false;
    return true;
}

static void recalc_weapon_damage(const ItemDatabase *db, Droid *d) {
    if (!db || !d) return;
    uint16_t best = 0;
    int best_damage = 0;
    for (size_t i = 0; i < sizeof(d->weapons); ++i) {
        const Item *item = item_db_get(db, d->weapons[i]);
        if (!item || item->damage_min < 0 || item->damage_min > UINT8_MAX ||
            item->damage_max < 0 || item->damage_max > UINT8_MAX)
            continue;
        uint8_t lo = (uint8_t)item->damage_min;
        uint8_t hi = (uint8_t)item->damage_max;
        uint16_t encoded = (uint16_t)lo | ((uint16_t)hi << 8);
        int damage = (int)lo * (int)hi;
        if (damage > best_damage || (damage == best_damage && encoded > best)) {
            best_damage = damage;
            best = encoded;
        }
    }
    d->weapon_damage = best;
}

static bool replace_save_file(const char *temporary_path, const char *path) {
#ifdef _WIN32
    /* The CRT rename() refuses to replace an existing file on Windows. */
    return MoveFileExA(temporary_path, path,
                       MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
    return rename(temporary_path, path) == 0;
#endif
}

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
    if (!valid_runtime_position(gs, gs->current_level, gs->party_x,
                                gs->party_y)) return false;
    if (creatures->num_creatures < 0 || creatures->num_creatures > MAX_CREATURES ||
        puzzles->num_puzzles < 0 || puzzles->num_puzzles > MAX_PUZZLES) return false;
    for (int i = 0; i < creatures->num_creatures; i++) {
        const Creature *c = &creatures->creatures[i];
        if (c->type < CREATURE_NONE || c->type >= CREATURE_COUNT ||
            c->x < 0 || c->x >= MAP_WIDTH || c->y < 0 || c->y >= MAP_HEIGHT ||
            c->level < 0 || c->level >= gs->num_levels || c->hp < 0 ||
            c->hp_max < 0 || c->hp > c->hp_max || c->damage_min < 0 ||
            c->damage_max < c->damage_min || c->defense < 0 ||
            !valid_runtime_position(gs, c->level, c->x, c->y) ||
            !valid_creature_runtime_state(c) ||
            !valid_creature_placement(creatures, i, gs->current_level,
                                       gs->party_x, gs->party_y))
            return false;
    }
    for (int i = 0; i < puzzles->num_puzzles; i++) {
        const Puzzle *p = &puzzles->puzzles[i];
        if (p->type < PUZZLE_NONE || p->type > PUZZLE_WALL_ELECTRIC ||
            p->x < 0 || p->x >= MAP_WIDTH || p->y < 0 || p->y >= MAP_HEIGHT ||
            p->level < 0 || p->level >= gs->num_levels ||
            p->face < 0 || p->face > DIR_WEST ||
            !valid_puzzle_target(p))
            return false;
    }
    ItemDatabase item_db;
    item_db_init(&item_db);
    for (int i = 0; i < 4; i++) {
        const Droid *d = &gs->droids[i];
        if (d->hp < 0 || d->hp_max < 0 || d->hp > d->hp_max ||
            d->energy < 0 || d->energy_max < 0 || d->energy > d->energy_max ||
            !valid_droid_items(&item_db, d))
            return false;
    }
    for (int level = 0; level < gs->num_levels; level++) {
        for (int y = 0; y < MAP_HEIGHT; y++) {
            for (int x = 0; x < MAP_WIDTH; x++) {
                CellType type = gs->levels[level].cells[y][x].type;
                if (type < CELL_WALL || type > CELL_PIT) return false;
                if (!valid_item_id(&item_db, gs->levels[level].cells[y][x].item_id))
                    return false;
            }
        }
    }
    size_t path_length = strlen(path);
    if (path_length > SIZE_MAX - 5) return false;
    char *temporary_path = malloc(path_length + 5);
    if (!temporary_path) return false;
    snprintf(temporary_path, path_length + 5, "%s.tmp", path);

    /* Write beside the destination and replace it only after every byte has
     * been written.  This keeps the previous save usable after a disk-full,
     * permission, or interrupted-write failure. */
    FILE *f = fopen(temporary_path, "wb");
    if (!f) {
        free(temporary_path);
        return false;
    }

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

    bool ok = write_header_v5(f, &hdr);
    for (size_t i = 0; ok && i < sizeof(gs->droids) / sizeof(gs->droids[0]); ++i)
        ok = write_droid_v5(f, &gs->droids[i]);

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

    for (int i = 0; ok && i < creatures->num_creatures; ++i)
        ok = write_creature_v5(f, &creatures->creatures[i]);
    for (int i = 0; ok && i < puzzles->num_puzzles; ++i)
        ok = write_puzzle_v5(f, &puzzles->puzzles[i]);

    if (fclose(f) != 0) ok = false;
    if (!ok) {
        remove(temporary_path);
        free(temporary_path);
        return false;
    }
    ok = replace_save_file(temporary_path, path);
    if (!ok) remove(temporary_path);
    free(temporary_path);
    return ok;
}

bool load_game(GameState *gs, CreatureList *creatures, PuzzleList *puzzles,
               const char *path) {
    if (!gs || !creatures || !puzzles || !path) return false;
    /* Display/runtime options are not part of the save format.  Preserve the
     * active configuration while replacing the gameplay state below; this
     * keeps F10 settings and the selected data root intact after loading. */
    OpenCaptiveConfig active_config = gs->config;
    FILE *f = fopen(path, "rb");
    if (!f) return false;

    SaveHeader hdr;
    if (!read_header(f, &hdr)) { fclose(f); return false; }
    if (hdr.magic != SAVE_MAGIC ||
        (hdr.version != SAVE_VERSION && hdr.version != SAVE_VERSION_RAW &&
         hdr.version != SAVE_VERSION_LEGACY) ||
        hdr.game_type != GAME_CAPTIVE || hdr.mission == 0 ||
        hdr.mission > INT_MAX || hdr.base_id > INT_MAX ||
        hdr.party_x < 0 || hdr.party_x >= MAP_WIDTH ||
        hdr.party_y < 0 || hdr.party_y >= MAP_HEIGHT ||
        hdr.party_dir > DIR_WEST || hdr.current_level < 0 ||
        hdr.current_level >= MAX_LEVELS ||
        hdr.current_level >= hdr.num_levels || hdr.num_levels < 1 ||
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
    ItemDatabase item_db;
    item_db_init(&item_db);
#define LOAD_FAIL() do { fclose(f); free(restored); return false; } while (0)
    game_state_init(restored, GAME_CAPTIVE, (int)hdr.mission);
    restored->base_id = (int)hdr.base_id;
    /* The save stores the authoritative mission seed.  Rebuilding through
     * game_state_new_mission() would derive a different seed for missions
     * created with game_state_new_mission_seeded(), making valid saves fail
     * validation before their level state can be restored. */
    if (!game_state_new_mission_seeded(restored, (int)hdr.mission,
                                       hdr.mission_seed)) LOAD_FAIL();
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

    if (hdr.version == SAVE_VERSION) {
        for (size_t i = 0; i < sizeof(restored->droids) / sizeof(restored->droids[0]); ++i)
            if (!read_droid_v5(f, &restored->droids[i])) LOAD_FAIL();
    } else if (fread(restored->droids, sizeof(restored->droids), 1, f) != 1) {
        LOAD_FAIL();
    }
    for (size_t i = 0; i < sizeof(restored->droids) / sizeof(restored->droids[0]); ++i) {
        restored->droids[i].name[sizeof(restored->droids[i].name) - 1] = '\0';
        const Droid *droid = &restored->droids[i];
        if (droid->hp < 0 || droid->hp > droid->hp_max ||
            droid->hp_max < 0 || droid->energy < 0 ||
            droid->energy > droid->energy_max || droid->energy_max < 0 ||
            !valid_droid_items(&item_db, droid))
            LOAD_FAIL();
        /* weapon_damage is a derived combat cache, not authoritative save
         * state. Recompute it from validated weapon slots so a modified or
         * stale save cannot grant arbitrary damage after loading. */
        recalc_weapon_damage(&item_db, &restored->droids[i]);
    }

    // Restore cell types (doors opened etc.)
    for (int i = 0; i < restored->num_levels; i++) {
        for (int y = 0; y < MAP_HEIGHT; y++) {
            for (int x = 0; x < MAP_WIDTH; x++) {
                uint8_t type;
                if (fread(&type, 1, 1, f) != 1 || type > CELL_PIT)
                    LOAD_FAIL();
                restored->levels[i].cells[y][x].type = (CellType)type;
                if (hdr.version >= SAVE_VERSION_RAW) {
                    if (fread(&restored->levels[i].cells[y][x].item_id, 1, 1, f) != 1)
                        LOAD_FAIL();
                    if (!valid_item_id(&item_db,
                                       restored->levels[i].cells[y][x].item_id))
                        LOAD_FAIL();
                }
            }
        }
    }

    if (!valid_runtime_position(restored, restored->current_level,
                                restored->party_x, restored->party_y))
        LOAD_FAIL();

    restored_creatures.num_creatures = hdr.num_creatures;
    restored_puzzles.num_puzzles = hdr.num_puzzles;
    if (hdr.version == SAVE_VERSION) {
        for (int i = 0; i < restored_creatures.num_creatures; ++i)
            if (!read_creature_v5(f, &restored_creatures.creatures[i])) LOAD_FAIL();
        for (int i = 0; i < restored_puzzles.num_puzzles; ++i)
            if (!read_puzzle_v5(f, &restored_puzzles.puzzles[i])) LOAD_FAIL();
    } else if (fread(restored_creatures.creatures, sizeof(Creature),
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
            creature->defense < 0 ||
            !valid_runtime_position(restored, creature->level,
                                    creature->x, creature->y) ||
            !valid_creature_runtime_state(creature) ||
            !valid_creature_placement(&restored_creatures, i,
                                       restored->current_level,
                                       restored->party_x, restored->party_y)) {
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
            !valid_puzzle_target(puzzle)) {
            LOAD_FAIL();
        }
    }
    /* The deterministic map rebuild restores ordinary ornaments, but puzzle
     * panels are generated at runtime and are not part of the cell-type/item
     * payload. Reattach them after validation so Continue/F9 preserves the
     * visible interaction as well as the puzzle state. */
    restore_puzzle_ornaments(restored, &restored_puzzles);

    /* The only extension area is an optional thumbnail trailer.  Reject
     * anything else so a concatenated or mismatched save cannot be
     * accepted as valid state. */
    long payload_end = ftell(f);
    if (payload_end < 0 || fseek(f, 0, SEEK_END) != 0) {
        LOAD_FAIL();
    }
    long file_end = ftell(f);
    if (file_end < 0) {
        LOAD_FAIL();
    }
    long trailing = file_end - payload_end;
    if (trailing != 0 && trailing != SAVE_THUMB_BLOCK_SIZE) {
        LOAD_FAIL();
    }
    if (trailing == SAVE_THUMB_BLOCK_SIZE) {
        uint32_t thumb_magic;
        if (fseek(f, payload_end, SEEK_SET) != 0 ||
            !read_u32_le(f, &thumb_magic) || thumb_magic != SAVE_THUMB_MAGIC) {
            LOAD_FAIL();
        }
    }
    if (fclose(f) != 0) {
        free(restored);
        return false;
    }
    restored->mode = STATE_GAME;
    restored->config = active_config;
    *gs = *restored;
    free(restored);
    *creatures = restored_creatures;
    *puzzles = restored_puzzles;
#undef LOAD_FAIL
    return true;
}

/* save_game_write_thumbnail() and save_read_thumbnail() are implemented in
 * save_thumbnail.c, a dependency-free translation unit shared with the
 * start menu (which previews a save slot without linking the rest of the
 * engine). Declared for callers via save_load.h -> save_thumbnail.h. */
