#ifndef GFX_LOADER_H
#define GFX_LOADER_H

#include "pl5_decoder.h"
#include <stdbool.h>

#define MAX_TEXTURES 64

typedef struct {
    uint32_t *pixels;   // ARGB8888, 320x200
    int       width;
    int       height;
    char      name[32];
    bool      loaded;
} Texture;

typedef struct {
    Texture textures[MAX_TEXTURES];
    int     num_textures;
    char    data_path[512];
} GfxData;

bool gfx_init(GfxData *gfx, const char *data_path);
void gfx_free(GfxData *gfx);
int  gfx_load_pl5(GfxData *gfx, const char *filename);
const Texture *gfx_get(const GfxData *gfx, int id);

#endif
