#include "texture_atlas.h"
#include <string.h>
#include <stdio.h>

bool texture_atlas_load(TextureAtlas *atlas, const DataVFS *vfs) {
    memset(atlas, 0, sizeof(*atlas));
    for (int i = 0; i < CAPTIVE_VIEW_SOURCE_COUNT; ++i) atlas->view_sheets[i] = -1;
    for (int i = 0; i < 5; i++) atlas->wall_sheets[i] = -1;
    atlas->roof_sheet = -1;
    atlas->door_sheet = -1;
    atlas->icon_sheet = -1;
    atlas->object_sheet = -1;
    atlas->gamescrn_sheet = -1;

    if (!gfx_init(&atlas->gfx, vfs)) return false;

    for (int i = 0; i < CAPTIVE_VIEW_SOURCE_COUNT; ++i)
        atlas->view_sheets[i] = gfx_load_pl5_hash(&atlas->gfx,
                                                  captive_view_source_hashes[i]);

    for (int i = 0; i < 5; ++i) atlas->wall_sheets[i] = atlas->view_sheets[i];
    atlas->roof_sheet = atlas->view_sheets[14];
    atlas->door_sheet = atlas->view_sheets[15];
    atlas->icon_sheet = gfx_load_pl5_hash(&atlas->gfx,
        "ce00ba2bc78f160b934486fe101a90264163356e02e7acbea2a41cf5d125b017");
    atlas->object_sheet = atlas->view_sheets[17];
    atlas->gamescrn_sheet = gfx_load_pl5_hash(&atlas->gfx,
        "dfca77f0e219962242226f11f9697f580f92e8ad24786296a5b2571b20c2b707");

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
    return atlas->loaded;
}

void texture_atlas_free(TextureAtlas *atlas) {
    gfx_free(&atlas->gfx);
    memset(atlas, 0, sizeof(*atlas));
}

uint32_t texture_sample(const TextureAtlas *atlas, int sheet_id,
                        int region_x, int region_y, int region_w, int region_h,
                        float u, float v) {
    const Texture *tex = gfx_get(&atlas->gfx, sheet_id);
    if (!tex) return 0xFF000000;

    // Clamp UVs
    if (u < 0.0f) u = 0.0f;
    if (u > 1.0f) u = 1.0f;
    if (v < 0.0f) v = 0.0f;
    if (v > 1.0f) v = 1.0f;

    int px = region_x + (int)(u * (region_w - 1));
    int py = region_y + (int)(v * (region_h - 1));

    if (px < 0 || px >= tex->width || py < 0 || py >= tex->height)
        return 0xFF000000;

    return tex->pixels[py * tex->width + px];
}
