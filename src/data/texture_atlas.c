#include "texture_atlas.h"
#include <math.h>
#include <string.h>
#include <stdio.h>
static const char *const alien_hashes[6] = {
    "4a2bc840d184ff07657f56e630b0293f1d5d7cdbf1d00e4505a1a69dcf721667",
    "2c8db6bfbec2b463856ab4cd9a313f9fbf20be408a2e278d94d498653562f754",
    "0b0d6ee225493c92b534b50e893d9c27e423ce0a4298e1789682b8cf222b7adc",
    "1f1b89e7692dc7c01f9d649677c820e79076304e8bc79835683e14484d68bb5b",
    "fed16e510697e17123d474c08687de548076b26a55f08f1d00fd17e3fcdf9410",
    "63ffa6901b59d463b050088065503d386ca2f3813ed91d8e0833320f9df2fe11",
};

bool texture_atlas_load(TextureAtlas *atlas, const DataVFS *vfs) {
    if (!atlas || !vfs) return false;
    memset(atlas, 0, sizeof(*atlas));
    for (int i = 0; i < CAPTIVE_VIEW_SOURCE_COUNT; ++i) atlas->view_sheets[i] = -1;
    for (int i = 0; i < 5; i++) atlas->wall_sheets[i] = -1;
    atlas->roof_sheet = -1;
    atlas->door_sheet = -1;
    atlas->icon_sheet = -1;
    atlas->object_sheet = -1;
    atlas->gamescrn_sheet = -1;
    for (int i = 0; i < 6; i++) atlas->alien_sheets[i] = -1;

    if (!gfx_init(&atlas->gfx, vfs)) return false;

    for (int i = 0; i < CAPTIVE_VIEW_SOURCE_COUNT; ++i)
        atlas->view_sheets[i] = gfx_load_pl5_hash(&atlas->gfx,
                                                  captive_view_source_hashes[i]);

    for (int i = 0; i < 5; ++i) atlas->wall_sheets[i] = atlas->view_sheets[i];
    atlas->roof_sheet = atlas->view_sheets[14];
    atlas->door_sheet = atlas->view_sheets[15];
    atlas->icon_sheet = atlas->view_sheets[19];
    atlas->object_sheet = atlas->view_sheets[17];
    atlas->gamescrn_sheet = atlas->view_sheets[18];

    for (int i = 0; i < 6; ++i)
        atlas->alien_sheets[i] = gfx_load_pl5_hash(&atlas->gfx, alien_hashes[i]);

    /* Original-mode rendering is an all-or-nothing contract.  A partial
       atlas used to be accepted as soon as the first wall sheet was present,
       which could leave the real GAME SCRN shell next to missing view,
       object, door, or roof layers.  Every lookup above is content-addressed
       and hash-verified; require the complete set before exposing it. */
    atlas->loaded = atlas->gamescrn_sheet >= 0 && atlas->roof_sheet >= 0 &&
        atlas->door_sheet >= 0 && atlas->icon_sheet >= 0 &&
        atlas->object_sheet >= 0;
    for (int i = 0; i < 5; ++i)
        atlas->loaded = atlas->loaded && atlas->wall_sheets[i] >= 0;
    for (int i = 0; i < CAPTIVE_VIEW_SOURCE_COUNT; ++i)
        atlas->loaded = atlas->loaded && atlas->view_sheets[i] >= 0;
    if (!atlas->loaded) {
        /* Loading can fail after several hash-verified sheets have already
         * allocated pixel buffers.  Leave callers with a clean, empty atlas
         * even when they only inspect the boolean return value. */
        texture_atlas_free(atlas);
        return false;
    }
    return atlas->loaded;
}

void texture_atlas_free(TextureAtlas *atlas) {
    if (!atlas) return;
    gfx_free(&atlas->gfx);
    memset(atlas, 0, sizeof(*atlas));
}

uint32_t texture_sample(const TextureAtlas *atlas, int sheet_id,
                        int region_x, int region_y, int region_w, int region_h,
                        float u, float v) {
    if (!atlas || region_w <= 0 || region_h <= 0 ||
        !isfinite(u) || !isfinite(v)) return 0xFF000000;
    const Texture *tex = gfx_get(&atlas->gfx, sheet_id);
    if (!tex) return 0xFF000000;
    /* Validate the rectangle before adding offsets.  Besides rejecting
       outside regions, this prevents signed overflow for corrupt coordinates
       such as INT_MAX. */
    if (region_x < 0 || region_y < 0 || region_x > tex->width ||
        region_y > tex->height || region_w > tex->width - region_x ||
        region_h > tex->height - region_y)
        return 0xFF000000;

    // Clamp UVs
    if (u < 0.0f) u = 0.0f;
    if (u > 1.0f) u = 1.0f;
    if (v < 0.0f) v = 0.0f;
    if (v > 1.0f) v = 1.0f;

    int px = region_x + (int)(u * (region_w - 1));
    int py = region_y + (int)(v * (region_h - 1));

    return tex->pixels[py * tex->width + px];
}
