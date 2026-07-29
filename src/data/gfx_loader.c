#include "gfx_loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool gfx_init(GfxData *gfx, const char *data_path) {
    memset(gfx, 0, sizeof(*gfx));
    strncpy(gfx->data_path, data_path, sizeof(gfx->data_path) - 1);
    return true;
}

void gfx_free(GfxData *gfx) {
    for (int i = 0; i < gfx->num_textures; i++) {
        free(gfx->textures[i].pixels);
    }
    memset(gfx, 0, sizeof(*gfx));
}

int gfx_load_pl5(GfxData *gfx, const char *filename) {
    if (gfx->num_textures >= MAX_TEXTURES) return -1;

    char path[1024];
    snprintf(path, sizeof(path), "%s/CAPICS/%s", gfx->data_path, filename);

    FILE *f = fopen(path, "rb");
    if (!f) return -1;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (size != PL5_FILE_SIZE) {
        fclose(f);
        return -1;
    }

    uint8_t *raw = malloc(PL5_FILE_SIZE);
    if (!raw) { fclose(f); return -1; }
    fread(raw, 1, PL5_FILE_SIZE, f);
    fclose(f);

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

    // Convert indexed pixels to ARGB8888 using palette
    for (int i = 0; i < PL5_PIXEL_COUNT; i++) {
        uint8_t idx = img.pixel_data[i];
        tex->pixels[i] = (idx < PL5_COLORS) ? img.palette[idx] : 0xFF000000;
    }
    pl5_free(&img);

    tex->width = PL5_WIDTH;
    tex->height = PL5_HEIGHT;
    tex->loaded = true;
    strncpy(tex->name, filename, sizeof(tex->name) - 1);

    return id;
}

const Texture *gfx_get(const GfxData *gfx, int id) {
    if (id < 0 || id >= gfx->num_textures) return NULL;
    if (!gfx->textures[id].loaded) return NULL;
    return &gfx->textures[id];
}
