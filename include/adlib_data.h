#ifndef ADLIB_DATA_H
#define ADLIB_DATA_H

#include <stdint.h>
#include <stddef.h>

/* OPL2 instrument patches and SFX sequences extracted from CAP_A.BIN
 * in the verified DOS release of Captive. The AdLib driver generates all
 * sound effects via FM synthesis — no PCM samples are used. */

#define CAPTIVE_OPL2_PATCH_COUNT 26
#define CAPTIVE_SFX_COUNT 49

extern const uint8_t captive_opl2_patches[CAPTIVE_OPL2_PATCH_COUNT][16];

typedef struct {
    const uint8_t *data;
    size_t size;
} CaptiveSfxEntry;

extern const CaptiveSfxEntry captive_sfx_table[CAPTIVE_SFX_COUNT];

extern const uint16_t captive_opl2_ftable[128];

#endif
