#ifndef GFX_LOADER_H
#define GFX_LOADER_H

#include "pl5_decoder.h"
#include "data_vfs.h"
#include <stdbool.h>

#define MAX_TEXTURES 64

typedef struct {
    uint32_t *pixels;   // ARGB8888, 320x200
    /* The original PL5 colour indices are retained as well as the expanded
       pixels.  Captive's panel copies use colour zero as the transparent
       background; compositor commands can therefore use this array directly
       as a per-pixel mask without guessing from RGB values. */
    uint8_t  *indices;  // Original palette indices, 320x200
    int       width;
    int       height;
    char      name[32];
    bool      loaded;
} Texture;

typedef struct {
    Texture textures[MAX_TEXTURES];
    int     num_textures;
    const DataVFS *vfs;
} GfxData;

bool gfx_init(GfxData *gfx, const DataVFS *vfs);
void gfx_free(GfxData *gfx);
int  gfx_load_pl5_hash(GfxData *gfx, const char expected_sha256[65]);
const Texture *gfx_get(const GfxData *gfx, int id);

#endif
