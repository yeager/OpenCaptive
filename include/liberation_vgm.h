#ifndef LIBERATION_VGM_H
#define LIBERATION_VGM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "amos_sprite.h"

#define VGM_MAX_BANKS 4
#define VGM_MAX_SPRITES 256

typedef struct {
    const uint8_t *bank_data[VGM_MAX_BANKS];
    size_t bank_size[VGM_MAX_BANKS];
    uint16_t bank_count[VGM_MAX_BANKS];
    uint16_t bank_first_index[VGM_MAX_BANKS];
    unsigned num_banks;
    unsigned total_sprites;
} VgmFile;

bool vgm_open(VgmFile *vgm, const uint8_t *data, size_t size);
bool vgm_get_sprite(const VgmFile *vgm, unsigned index, AmosSprite *out);

#define LIBERATION_VGM_WALL_SET_COUNT 71
extern const char *const liberation_vgm_wall_sha256[LIBERATION_VGM_WALL_SET_COUNT];

#endif
