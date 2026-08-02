#include "custom_features.h"
#include "game_state.h"
#include <stdio.h>
#include <string.h>

#define CROSS_SAVE_MAGIC 0x4F435356 /* "OCSV" */
#define CROSS_SAVE_VERSION 1

bool cross_save_export(const void *game_state_ptr, const char *path) {
    const GameState *gs = (const GameState *)game_state_ptr;
    FILE *fp = fopen(path, "wb");
    if (!fp) return false;

    uint32_t magic = CROSS_SAVE_MAGIC;
    uint32_t version = CROSS_SAVE_VERSION;
    fwrite(&magic, 4, 1, fp);
    fwrite(&version, 4, 1, fp);

    uint8_t game_type = (uint8_t)gs->game_type;
    fwrite(&game_type, 1, 1, fp);

    int32_t vals[] = {
        gs->party_x, gs->party_y, gs->party_dir,
        gs->current_level, gs->mission,
        gs->num_levels, gs->base_id,
        gs->generators_total, gs->generators_destroyed, gs->gold
    };
    fwrite(vals, sizeof(vals), 1, fp);
    fwrite(&gs->mission_seed, 4, 1, fp);
    fwrite(&gs->tick, 4, 1, fp);

    for (int d = 0; d < 4; d++) {
        const Droid *dr = &gs->droids[d];
        fwrite(dr->name, 16, 1, fp);
        fwrite(&dr->hp, 2, 1, fp);
        fwrite(&dr->hp_max, 2, 1, fp);
        fwrite(&dr->energy, 2, 1, fp);
        fwrite(&dr->energy_max, 2, 1, fp);
        fwrite(dr->body_parts, 6, 1, fp);
        fwrite(dr->weapons, 2, 1, fp);
        fwrite(dr->items, 10, 1, fp);
        fwrite(dr->skill_levels, 10, 1, fp);
        fwrite(&dr->xp, 4, 1, fp);
        fwrite(&dr->weapon_damage, 2, 1, fp);
    }

    for (int l = 0; l < gs->num_levels && l < MAX_LEVELS; l++) {
        const DungeonLevel *lvl = &gs->levels[l];
        fwrite(&lvl->level, 4, 1, fp);
        fwrite(&lvl->seed, 4, 1, fp);
        for (int y = 0; y < MAP_HEIGHT; y++) {
            for (int x = 0; x < MAP_WIDTH; x++) {
                const MapCell *c = &lvl->cells[y][x];
                uint8_t ct = (uint8_t)c->type;
                fwrite(&ct, 1, 1, fp);
                fwrite(c->wall_tex, 4, 1, fp);
                fwrite(&c->floor_tex, 1, 1, fp);
                fwrite(&c->ceil_tex, 1, 1, fp);
                fwrite(&c->item_id, 1, 1, fp);
                fwrite(&c->creature_id, 1, 1, fp);
                fwrite(&c->flags, 1, 1, fp);
            }
        }
    }

    fclose(fp);
    return true;
}

bool cross_save_import(void *game_state_ptr, const char *path) {
    GameState *gs = (GameState *)game_state_ptr;
    FILE *fp = fopen(path, "rb");
    if (!fp) return false;

    uint32_t magic, version;
    if (fread(&magic, 4, 1, fp) != 1 || magic != CROSS_SAVE_MAGIC) {
        fclose(fp);
        return false;
    }
    if (fread(&version, 4, 1, fp) != 1 || version != CROSS_SAVE_VERSION) {
        fclose(fp);
        return false;
    }

    uint8_t game_type;
    fread(&game_type, 1, 1, fp);
    gs->game_type = (GameType)game_type;

    int32_t vals[10];
    fread(vals, sizeof(vals), 1, fp);
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
    fread(&gs->mission_seed, 4, 1, fp);
    fread(&gs->tick, 4, 1, fp);

    for (int d = 0; d < 4; d++) {
        Droid *dr = &gs->droids[d];
        fread(dr->name, 16, 1, fp);
        fread(&dr->hp, 2, 1, fp);
        fread(&dr->hp_max, 2, 1, fp);
        fread(&dr->energy, 2, 1, fp);
        fread(&dr->energy_max, 2, 1, fp);
        fread(dr->body_parts, 6, 1, fp);
        fread(dr->weapons, 2, 1, fp);
        fread(dr->items, 10, 1, fp);
        fread(dr->skill_levels, 10, 1, fp);
        fread(&dr->xp, 4, 1, fp);
        fread(&dr->weapon_damage, 2, 1, fp);
    }

    for (int l = 0; l < gs->num_levels && l < MAX_LEVELS; l++) {
        DungeonLevel *lvl = &gs->levels[l];
        fread(&lvl->level, 4, 1, fp);
        fread(&lvl->seed, 4, 1, fp);
        for (int y = 0; y < MAP_HEIGHT; y++) {
            for (int x = 0; x < MAP_WIDTH; x++) {
                MapCell *c = &lvl->cells[y][x];
                uint8_t ct;
                fread(&ct, 1, 1, fp);
                c->type = (CellType)ct;
                fread(c->wall_tex, 4, 1, fp);
                fread(&c->floor_tex, 1, 1, fp);
                fread(&c->ceil_tex, 1, 1, fp);
                fread(&c->item_id, 1, 1, fp);
                fread(&c->creature_id, 1, 1, fp);
                fread(&c->flags, 1, 1, fp);
            }
        }
    }

    fclose(fp);
    return true;
}
