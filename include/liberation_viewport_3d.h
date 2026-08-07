#ifndef LIBERATION_VIEWPORT_3D_H
#define LIBERATION_VIEWPORT_3D_H

#include <stdbool.h>
#include <stdint.h>
#include "liberation_x3g.h"

#define LIB3D_VP_WIDTH  256
#define LIB3D_VP_HEIGHT 160
/* The city viewport submits one small object per visible ground/wall cell.
 * Keep enough persistent projection slots for the complete 17x17 view, not
 * just the first few objects encountered by the renderer. */
#define LIB3D_MAX_PROJECTED 8192
#define LIB3D_MAX_VISIBLE_POLYS 4096
#define LIB3D_NEAR_CLIP 4
#define LIB3D_MAX_TEX_SIZE 128

typedef struct {
    uint32_t pixels[LIB3D_MAX_TEX_SIZE * LIB3D_MAX_TEX_SIZE];
    int width, height;
} Lib3dTexture;

typedef struct {
    float x, y, z;
} Lib3dVec3;

typedef struct {
    int sx, sy;
    float z;
} Lib3dProjected;

typedef struct {
    uint8_t  vertex_count;
    uint16_t vertex_indices[8];
    uint16_t color;
    uint16_t flags;
    int16_t  normal_x, normal_y;
    float    avg_z;
} Lib3dVisPoly;

typedef struct {
    float cam_x, cam_y, cam_z;
    float cam_yaw;
    float fov_scale;

    uint32_t framebuffer[LIB3D_VP_WIDTH * LIB3D_VP_HEIGHT];
    float    zbuffer[LIB3D_VP_WIDTH * LIB3D_VP_HEIGHT];

    Lib3dProjected projected[LIB3D_MAX_PROJECTED];
    unsigned       proj_count;

    Lib3dVisPoly   visible[LIB3D_MAX_VISIBLE_POLYS];
    unsigned       vis_count;

    uint32_t sky_color;
    uint32_t ground_color;
    uint8_t  wall_color_base;
    bool     texture_filter;
    bool     dynamic_lighting;
    const uint32_t *palette;
    unsigned        pal_size;
} Lib3dState;

void lib3d_init(Lib3dState *state);
void lib3d_set_texture_filter(Lib3dState *state, bool enabled);
void lib3d_set_dynamic_lighting(Lib3dState *state, bool enabled);
void lib3d_set_camera(Lib3dState *state, float x, float y, float z, float yaw);
void lib3d_clear(Lib3dState *state, uint32_t sky_color, uint32_t ground_color);
void lib3d_render_object(Lib3dState *state, const X3gObject *obj,
                         float obj_x, float obj_y, float obj_z,
                         const uint32_t *palette, unsigned pal_size);
void lib3d_render_textured_quad(Lib3dState *state,
                                float x0, float y0, float z0,
                                float x1, float y1, float z1,
                                float x2, float y2, float z2,
                                float x3, float y3, float z3,
                                const Lib3dTexture *tex);
void lib3d_present(Lib3dState *state, uint32_t *dest, int dest_w, int dest_h,
                   int dest_x, int dest_y);

#endif
