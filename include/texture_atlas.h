#ifndef TEXTURE_ATLAS_H
#define TEXTURE_ATLAS_H

#include "gfx_loader.h"
#include <stdbool.h>

typedef struct {
    GfxData gfx;
    /* Hash-verified 320x200 panel sheets.  They contain overlapping,
       preprojected view fragments, not a regular texture-tile atlas. */
    int     wall_sheets[5];
    int     roof_sheet;       // ROOFS texture id
    int     door_sheet;       // DOORS1 texture id
    int     icon_sheet;       // ICONS texture id
    int     object_sheet;     // OBJECTS texture id
    int     gamescrn_sheet;   // GAMESCRN texture id
    bool    loaded;
} TextureAtlas;

bool texture_atlas_load(TextureAtlas *atlas, const DataVFS *vfs);
void texture_atlas_free(TextureAtlas *atlas);

// Sample a pixel from a texture region, with UV coordinates [0,1]
uint32_t texture_sample(const TextureAtlas *atlas, int sheet_id,
                        int region_x, int region_y, int region_w, int region_h,
                        float u, float v);

#endif
