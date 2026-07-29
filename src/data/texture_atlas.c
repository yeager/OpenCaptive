#include "texture_atlas.h"
#include <string.h>
#include <stdio.h>

bool texture_atlas_load(TextureAtlas *atlas, const DataVFS *vfs) {
    memset(atlas, 0, sizeof(*atlas));
    for (int i = 0; i < 5; i++) atlas->wall_sheets[i] = -1;
    atlas->roof_sheet = -1;
    atlas->door_sheet = -1;
    atlas->icon_sheet = -1;
    atlas->object_sheet = -1;
    atlas->gamescrn_sheet = -1;

    if (!gfx_init(&atlas->gfx, vfs)) return false;

    static const char *const wall_hashes[] = {
        "47ad15b4a593c37880d0306b6a0f51b7a9f20615cf6a188f23716d5b48315524",
        "43833e4a8df622f84d53698a76c6d18f910c1cca79c6b89cbfacc563f695356c",
        "8b7301fc6c302fd673a81d23e7a99d715aa02d5b404c1e1edea19ceccccc9681",
        "519d3ef4494f0e868479a90c8a47249b840598e382c7ba3272f417ce3daf5936",
        "7edb8ee856a91e835ea86dda00af49fda3dae730d694bd7234b7fa96d711e296",
    };
    for (int i = 0; i < 5; i++) {
        atlas->wall_sheets[i] = gfx_load_pl5_hash(&atlas->gfx, wall_hashes[i]);
    }

    atlas->roof_sheet = gfx_load_pl5_hash(&atlas->gfx,
        "978d18857d5ffcf6fb7b91fb22c02b85079db0171caeac3d290a69b276cf098f");
    atlas->door_sheet = gfx_load_pl5_hash(&atlas->gfx,
        "dec7143f063c98459ab2f267ed135204cdee1b521eda9810b219e8c10e05c7e8");
    atlas->icon_sheet = gfx_load_pl5_hash(&atlas->gfx,
        "ce00ba2bc78f160b934486fe101a90264163356e02e7acbea2a41cf5d125b017");
    atlas->object_sheet = gfx_load_pl5_hash(&atlas->gfx,
        "21db7daf64cff3b0cae19c3e7eb2057762df9110055e7253175024ecb146fb6b");
    atlas->gamescrn_sheet = gfx_load_pl5_hash(&atlas->gfx,
        "dfca77f0e219962242226f11f9697f580f92e8ad24786296a5b2571b20c2b707");

    // At least one wall sheet must have loaded
    atlas->loaded = (atlas->wall_sheets[0] >= 0);
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
