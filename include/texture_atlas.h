#ifndef TEXTURE_ATLAS_H
#define TEXTURE_ATLAS_H

#include "gfx_loader.h"
#include <stdbool.h>

// Wall texture regions within PL5 sheets
// Each WALL?.PL5 contains multiple wall panel textures arranged as a sprite sheet

typedef struct {
    int x, y, w, h;   // region within the PL5 sheet
    int sheet_id;      // which texture sheet (GfxData texture index)
} TextureRegion;

// Predefined wall texture tile size
#define WALL_TILE_W 64
#define WALL_TILE_H 64

typedef enum {
    TEX_WALL_FRONT = 0,
    TEX_WALL_SIDE,
    TEX_WALL_ORNAMENT,
    TEX_FLOOR,
    TEX_CEILING,
    TEX_DOOR_CLOSED,
    TEX_DOOR_OPEN,
    TEX_DOOR_IRIS,
    TEX_STAIRS,
    TEX_GENERATOR,
    TEX_SHOP_FRONT,
    TEX_COUNT,
} TextureType;

typedef struct {
    GfxData gfx;
    int     wall_sheets[5];   // WALLA..WALLE texture ids
    int     roof_sheet;       // ROOFS texture id
    int     door_sheet;       // DOORS1 texture id
    int     icon_sheet;       // ICONS texture id
    int     object_sheet;     // OBJECTS texture id
    int     gamescrn_sheet;   // GAMESCRN texture id
    bool    loaded;
} TextureAtlas;

bool texture_atlas_load(TextureAtlas *atlas, const char *data_path);
void texture_atlas_free(TextureAtlas *atlas);

// Sample a pixel from a texture region, with UV coordinates [0,1]
uint32_t texture_sample(const TextureAtlas *atlas, int sheet_id,
                        int region_x, int region_y, int region_w, int region_h,
                        float u, float v);

// Get wall texture region for a given wall set (0-4) and tile index
TextureRegion texture_get_wall(const TextureAtlas *atlas, int wall_set, int tile);

#endif
