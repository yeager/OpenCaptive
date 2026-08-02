#ifndef LIBERATION_SAVE_H
#define LIBERATION_SAVE_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "liberation_citygen_grid.h"
#include "liberation_citygen.h"
#include "liberation_city_nav.h"

#define LIB_SAVE_MAGIC "LSAV"
#define LIB_SAVE_VERSION 1
#define LIB_SAVE_MAX_DROIDS 4
#define LIB_SAVE_MAX_MISSIONS 256

typedef struct {
    char name[16];
    int16_t hp, hp_max;
    int16_t energy, energy_max;
    uint8_t level;
    uint8_t skills[8];
    uint16_t equipment[8];
} LibSaveDroid;

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t seed_hi, seed_lo;
    uint16_t difficulty;
    uint16_t mission;
    uint32_t gold;
    uint32_t tick;
    int16_t  city_x, city_y;
    uint8_t  facing;
    uint8_t  num_droids;
    LibSaveDroid droids[LIB_SAVE_MAX_DROIDS];
    uint8_t  mission_complete[LIB_SAVE_MAX_MISSIONS / 8];
    uint16_t generators_destroyed;
    uint16_t generators_total;
} LibSaveData;

bool lib_save_write(const LibSaveData *data, const char *path);
bool lib_save_read(LibSaveData *data, const char *path);
void lib_save_from_state(LibSaveData *data, uint16_t seed_hi, uint16_t seed_lo,
                          uint16_t difficulty, uint16_t mission, uint32_t gold,
                          uint32_t tick, const CityNavState *nav,
                          const LibSaveDroid *droids, uint8_t num_droids);
bool lib_save_is_mission_complete(const LibSaveData *data, unsigned mission);
void lib_save_set_mission_complete(LibSaveData *data, unsigned mission);

#endif
