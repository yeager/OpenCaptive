#ifndef LIBERATION_SAVE_H
#define LIBERATION_SAVE_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "liberation_citygen_grid.h"
#include "liberation_citygen.h"
#include "liberation_city_nav.h"

#define LIB_SAVE_MAGIC "LSAV"
#define LIB_SAVE_VERSION 9
#define LIB_SAVE_PREVIOUS_VERSION 8
#define LIB_SAVE_CRIME_VERSION 9
#define LIB_SAVE_REPUTATION_VERSION 5
#define LIB_SAVE_BODY_PART_VERSION 6
#define LIB_SAVE_SKILLS_VERSION 7
#define LIB_SAVE_SHIELD_VERSION 8
#define LIB_SAVE_XP_VERSION 3
#define LIB_SAVE_SHARED_INVENTORY_VERSION 4
#define LIB_SAVE_LEGACY_VERSION 1
#define LIB_SAVE_OLD_INVENTORY_VERSION 2
#define LIB_SAVE_MAX_DROIDS 4
#define LIB_SAVE_MAX_MISSIONS 256
#define LIB_SAVE_MAX_SHARED_ITEMS 40

typedef struct {
    char name[16];
    int16_t hp, hp_max;
    int16_t energy, energy_max;
    uint8_t level;
    uint32_t xp;
    uint8_t skills[10];
    uint16_t equipment[8];
    uint8_t inventory[10];
    uint8_t body_part_hp[6];
    /* OpenCaptive extension: the original eight equipment fields do not
       include the shared runtime shield slot. */
    uint8_t shield;
    int16_t shield_hp;
} LibSaveDroid;

typedef struct {
    char name[24];
    uint16_t item_type;
} LibSaveSharedItem;

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
    uint8_t shared_inventory_count;
    LibSaveSharedItem shared_inventory[LIB_SAVE_MAX_SHARED_ITEMS];
    uint8_t  mission_complete[LIB_SAVE_MAX_MISSIONS / 8];
    uint16_t generators_destroyed;
    uint16_t generators_total;
    int16_t  reputation;
    uint8_t  crime_level;
    uint8_t  wanted;
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
