#include "gfx_loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool gfx_init(GfxData *gfx, const DataVFS *vfs) {
    memset(gfx, 0, sizeof(*gfx));
    gfx->vfs = vfs;
    return true;
}

void gfx_free(GfxData *gfx) {
    for (int i = 0; i < gfx->num_textures; i++) {
        free(gfx->textures[i].pixels);
    }
    memset(gfx, 0, sizeof(*gfx));
}

int gfx_load_pl5_hash(GfxData *gfx, const char expected_sha256[65]) {
    if (gfx->num_textures >= MAX_TEXTURES) return -1;

    size_t size;
    uint8_t *raw = vfs_find_sha256(gfx->vfs, expected_sha256, &size);
    if (!raw) return -1;

    if (size != PL5_FILE_SIZE) {
        free(raw);
        return -1;
    }

    PL5Image img;
    if (!pl5_decode(raw, PL5_FILE_SIZE, &img)) {
        free(raw);
        return -1;
    }
    free(raw);

    int id = gfx->num_textures++;
    Texture *tex = &gfx->textures[id];

    tex->pixels = malloc(PL5_PIXEL_COUNT * sizeof(uint32_t));
    if (!tex->pixels) {
        pl5_free(&img);
        gfx->num_textures--;
        return -1;
    }

    for (int i = 0; i < PL5_PIXEL_COUNT; i++) {
        uint8_t idx = img.pixel_data[i];
        tex->pixels[i] = (idx < PL5_COLORS) ? img.palette[idx] : 0xFF000000;
    }
    pl5_free(&img);

    tex->width = PL5_WIDTH;
    tex->height = PL5_HEIGHT;
    tex->loaded = true;
    memcpy(tex->name, expected_sha256, 16);
    tex->name[16] = '\0';

    return id;
}

const Texture *gfx_get(const GfxData *gfx, int id) {
    if (id < 0 || id >= gfx->num_textures) return NULL;
    if (!gfx->textures[id].loaded) return NULL;
    return &gfx->textures[id];
}
