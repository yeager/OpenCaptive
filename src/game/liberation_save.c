#include "liberation_save.h"
#include <stdio.h>
#include <string.h>

static void write_u16(FILE *f, uint16_t v) {
    uint8_t b[2] = { (uint8_t)(v >> 8), (uint8_t)v };
    fwrite(b, 1, 2, f);
}

static void write_u32(FILE *f, uint32_t v) {
    uint8_t b[4] = { (uint8_t)(v >> 24), (uint8_t)(v >> 16),
                      (uint8_t)(v >> 8), (uint8_t)v };
    fwrite(b, 1, 4, f);
}

static uint16_t read_u16(FILE *f) {
    uint8_t b[2];
    fread(b, 1, 2, f);
    return (uint16_t)(b[0] << 8 | b[1]);
}

static uint32_t read_u32(FILE *f) {
    uint8_t b[4];
    fread(b, 1, 4, f);
    return (uint32_t)(b[0] << 24 | b[1] << 16 | b[2] << 8 | b[3]);
}

bool lib_save_write(const LibSaveData *data, const char *path) {
    if (!data || !path) return false;
    FILE *f = fopen(path, "wb");
    if (!f) return false;

    fwrite(LIB_SAVE_MAGIC, 1, 4, f);
    write_u16(f, LIB_SAVE_VERSION);
    write_u16(f, data->seed_hi);
    write_u16(f, data->seed_lo);
    write_u16(f, data->difficulty);
    write_u16(f, data->mission);
    write_u32(f, data->gold);
    write_u32(f, data->tick);
    write_u16(f, (uint16_t)data->city_x);
    write_u16(f, (uint16_t)data->city_y);
    fwrite(&data->facing, 1, 1, f);
    fwrite(&data->num_droids, 1, 1, f);

    for (int i = 0; i < data->num_droids; i++) {
        const LibSaveDroid *d = &data->droids[i];
        fwrite(d->name, 1, 16, f);
        write_u16(f, (uint16_t)d->hp);
        write_u16(f, (uint16_t)d->hp_max);
        write_u16(f, (uint16_t)d->energy);
        write_u16(f, (uint16_t)d->energy_max);
        fwrite(&d->level, 1, 1, f);
        fwrite(d->skills, 1, 8, f);
        for (int j = 0; j < 8; j++) write_u16(f, d->equipment[j]);
    }

    fwrite(data->mission_complete, 1, LIB_SAVE_MAX_MISSIONS / 8, f);
    write_u16(f, data->generators_destroyed);
    write_u16(f, data->generators_total);

    fclose(f);
    return true;
}

bool lib_save_read(LibSaveData *data, const char *path) {
    if (!data || !path) return false;
    FILE *f = fopen(path, "rb");
    if (!f) return false;

    memset(data, 0, sizeof(*data));

    char magic[4];
    if (fread(magic, 1, 4, f) != 4 || memcmp(magic, LIB_SAVE_MAGIC, 4) != 0) {
        fclose(f);
        return false;
    }

    uint16_t ver = read_u16(f);
    if (ver != LIB_SAVE_VERSION) { fclose(f); return false; }

    data->version = ver;
    data->seed_hi = read_u16(f);
    data->seed_lo = read_u16(f);
    data->difficulty = read_u16(f);
    data->mission = read_u16(f);
    data->gold = read_u32(f);
    data->tick = read_u32(f);
    data->city_x = (int16_t)read_u16(f);
    data->city_y = (int16_t)read_u16(f);
    fread(&data->facing, 1, 1, f);
    fread(&data->num_droids, 1, 1, f);

    if (data->num_droids > LIB_SAVE_MAX_DROIDS) {
        fclose(f);
        return false;
    }

    for (int i = 0; i < data->num_droids; i++) {
        LibSaveDroid *d = &data->droids[i];
        fread(d->name, 1, 16, f);
        d->hp = (int16_t)read_u16(f);
        d->hp_max = (int16_t)read_u16(f);
        d->energy = (int16_t)read_u16(f);
        d->energy_max = (int16_t)read_u16(f);
        fread(&d->level, 1, 1, f);
        fread(d->skills, 1, 8, f);
        for (int j = 0; j < 8; j++) d->equipment[j] = read_u16(f);
    }

    fread(data->mission_complete, 1, LIB_SAVE_MAX_MISSIONS / 8, f);
    data->generators_destroyed = read_u16(f);
    data->generators_total = read_u16(f);

    fclose(f);
    return true;
}

void lib_save_from_state(LibSaveData *data, uint16_t seed_hi, uint16_t seed_lo,
                          uint16_t difficulty, uint16_t mission, uint32_t gold,
                          uint32_t tick, const CityNavState *nav,
                          const LibSaveDroid *droids, uint8_t num_droids) {
    memset(data, 0, sizeof(*data));
    data->seed_hi = seed_hi;
    data->seed_lo = seed_lo;
    data->difficulty = difficulty;
    data->mission = mission;
    data->gold = gold;
    data->tick = tick;
    if (nav) {
        data->city_x = (int16_t)nav->cell_x;
        data->city_y = (int16_t)nav->cell_y;
        data->facing = (uint8_t)nav->facing;
    }
    data->num_droids = num_droids;
    if (droids && num_droids <= LIB_SAVE_MAX_DROIDS)
        memcpy(data->droids, droids, num_droids * sizeof(LibSaveDroid));
}

bool lib_save_is_mission_complete(const LibSaveData *data, unsigned mission) {
    if (!data || mission >= LIB_SAVE_MAX_MISSIONS) return false;
    return (data->mission_complete[mission / 8] >> (mission % 8)) & 1;
}

void lib_save_set_mission_complete(LibSaveData *data, unsigned mission) {
    if (!data || mission >= LIB_SAVE_MAX_MISSIONS) return;
    data->mission_complete[mission / 8] |= (1 << (mission % 8));
}
