#include "texture_atlas.h"
#include <string.h>
#include <stdio.h>

bool texture_atlas_load(TextureAtlas *atlas, const char *data_path) {
    memset(atlas, 0, sizeof(*atlas));
    for (int i = 0; i < 5; i++) atlas->wall_sheets[i] = -1;
    atlas->roof_sheet = -1;
    atlas->door_sheet = -1;
    atlas->icon_sheet = -1;
    atlas->object_sheet = -1;
    atlas->gamescrn_sheet = -1;

    if (!gfx_init(&atlas->gfx, data_path)) return false;

    const char *wall_names[] = {"WALLA.PL5","WALLB.PL5","WALLC.PL5","WALLD.PL5","WALLE.PL5"};
    for (int i = 0; i < 5; i++) {
        atlas->wall_sheets[i] = gfx_load_pl5(&atlas->gfx, wall_names[i]);
    }

    atlas->roof_sheet = gfx_load_pl5(&atlas->gfx, "ROOFS.PL5");
    atlas->door_sheet = gfx_load_pl5(&atlas->gfx, "DOORS1.PL5");
    atlas->icon_sheet = gfx_load_pl5(&atlas->gfx, "ICONS.PL5");
    atlas->object_sheet = gfx_load_pl5(&atlas->gfx, "OBJECTS.PL5");
    atlas->gamescrn_sheet = gfx_load_pl5(&atlas->gfx, "GAMESCRN.PL5");

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

TextureRegion texture_get_wall(const TextureAtlas *atlas, int wall_set, int tile) {
    TextureRegion r = {0, 0, WALL_TILE_W, WALL_TILE_H, -1};

    if (wall_set < 0 || wall_set >= 5) wall_set = 0;
    r.sheet_id = atlas->wall_sheets[wall_set];

    // Wall sheets are 320x200. Tiles are arranged as:
    // Row 0: tiles at (0,0), (64,0), (128,0), (192,0), (256,0)  = 5 tiles
    // Row 1: tiles at (0,64), (64,64), ...
    // Row 2: tiles at (0,128), (64,128), ...  (only 72px high)
    int tiles_per_row = 320 / WALL_TILE_W; // 5
    int row = tile / tiles_per_row;
    int col = tile % tiles_per_row;

    r.x = col * WALL_TILE_W;
    r.y = row * WALL_TILE_H;

    // Clamp to sheet bounds
    if (r.x + r.w > 320) r.w = 320 - r.x;
    if (r.y + r.h > 200) r.h = 200 - r.y;

    return r;
}
