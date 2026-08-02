#include "liberation_viewport_3d.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

void lib3d_init(Lib3dState *state) {
    memset(state, 0, sizeof(*state));
    state->fov_scale = 200.0f;
}

void lib3d_set_camera(Lib3dState *state, float x, float y, float z, float yaw) {
    state->cam_x = x;
    state->cam_y = y;
    state->cam_z = z;
    state->cam_yaw = yaw;
}

void lib3d_clear(Lib3dState *state, uint32_t sky_color, uint32_t ground_color) {
    int half = LIB3D_VP_HEIGHT / 2;
    for (int y = 0; y < half; y++)
        for (int x = 0; x < LIB3D_VP_WIDTH; x++)
            state->framebuffer[y * LIB3D_VP_WIDTH + x] = sky_color;
    for (int y = half; y < LIB3D_VP_HEIGHT; y++)
        for (int x = 0; x < LIB3D_VP_WIDTH; x++)
            state->framebuffer[y * LIB3D_VP_WIDTH + x] = ground_color;

    for (int i = 0; i < LIB3D_VP_WIDTH * LIB3D_VP_HEIGHT; i++)
        state->zbuffer[i] = 1e30f;

    state->proj_count = 0;
    state->vis_count = 0;
}

static void transform_vertex(const Lib3dState *state, const X3gVertex *v,
                              float obj_x, float obj_y, float obj_z,
                              Lib3dVec3 *out) {
    float wx = (float)v->x + obj_x - state->cam_x;
    float wy = (float)v->y + obj_y - state->cam_y;
    float wz = (float)v->z + obj_z - state->cam_z;

    float cos_y = cosf(state->cam_yaw);
    float sin_y = sinf(state->cam_yaw);

    out->x = wx * cos_y - wz * sin_y;
    out->y = wy;
    out->z = wx * sin_y + wz * cos_y;
}

static bool project(const Lib3dState *state, const Lib3dVec3 *v,
                    Lib3dProjected *out) {
    if (v->z < LIB3D_NEAR_CLIP) return false;

    float inv_z = state->fov_scale / v->z;
    out->sx = (int)(v->x * inv_z) + LIB3D_VP_WIDTH / 2;
    out->sy = (int)(-v->y * inv_z) + LIB3D_VP_HEIGHT / 2;
    out->z = v->z;
    return true;
}

static int poly_compare(const void *a, const void *b) {
    const Lib3dVisPoly *pa = (const Lib3dVisPoly *)a;
    const Lib3dVisPoly *pb = (const Lib3dVisPoly *)b;
    if (pb->avg_z > pa->avg_z) return 1;
    if (pb->avg_z < pa->avg_z) return -1;
    return 0;
}

static void fill_triangle(Lib3dState *state, int x0, int y0, float z0,
                           int x1, int y1, float z1,
                           int x2, int y2, float z2,
                           uint32_t color) {
    if (y0 > y1) { int t; t=x0;x0=x1;x1=t; t=y0;y0=y1;y1=t; float f=z0;z0=z1;z1=f; }
    if (y0 > y2) { int t; t=x0;x0=x2;x2=t; t=y0;y0=y2;y2=t; float f=z0;z0=z2;z2=f; }
    if (y1 > y2) { int t; t=x1;x1=x2;x2=t; t=y1;y1=y2;y2=t; float f=z1;z1=z2;z2=f; }

    int dy_total = y2 - y0;
    if (dy_total == 0) return;

    for (int y = y0; y <= y2; y++) {
        if (y < 0 || y >= LIB3D_VP_HEIGHT) continue;

        bool second_half = (y > y1) || (y1 == y0);
        int seg_height = second_half ? (y2 - y1) : (y1 - y0);
        if (seg_height == 0) seg_height = 1;

        float alpha = (float)(y - y0) / dy_total;
        float beta = second_half
            ? (float)(y - y1) / seg_height
            : (float)(y - y0) / seg_height;

        int xa = x0 + (int)((x2 - x0) * alpha);
        int xb = second_half
            ? x1 + (int)((x2 - x1) * beta)
            : x0 + (int)((x1 - x0) * beta);
        float za = z0 + (z2 - z0) * alpha;
        float zb = second_half ? z1 + (z2 - z1) * beta : z0 + (z1 - z0) * beta;

        if (xa > xb) { int t = xa; xa = xb; xb = t; float f = za; za = zb; zb = f; }

        for (int x = xa; x <= xb; x++) {
            if (x < 0 || x >= LIB3D_VP_WIDTH) continue;
            float t = (xb != xa) ? (float)(x - xa) / (xb - xa) : 0;
            float z = za + (zb - za) * t;
            int idx = y * LIB3D_VP_WIDTH + x;
            if (z < state->zbuffer[idx]) {
                state->zbuffer[idx] = z;
                state->framebuffer[idx] = color;
            }
        }
    }
}

static void fill_polygon(Lib3dState *state, const Lib3dVisPoly *poly,
                          uint32_t color) {
    for (int i = 1; i + 1 < poly->vertex_count; i++) {
        int i0 = poly->vertex_indices[0];
        int i1 = poly->vertex_indices[i];
        int i2 = poly->vertex_indices[i + 1];
        if (i0 >= (int)state->proj_count || i1 >= (int)state->proj_count ||
            i2 >= (int)state->proj_count) continue;

        fill_triangle(state,
            state->projected[i0].sx, state->projected[i0].sy, state->projected[i0].z,
            state->projected[i1].sx, state->projected[i1].sy, state->projected[i1].z,
            state->projected[i2].sx, state->projected[i2].sy, state->projected[i2].z,
            color);
    }
}

void lib3d_render_object(Lib3dState *state, const X3gObject *obj,
                         float obj_x, float obj_y, float obj_z,
                         const uint32_t *palette, unsigned pal_size) {
    if (!state || !obj) return;

    unsigned base_idx = state->proj_count;

    for (unsigned i = 0; i < obj->vertex_count && state->proj_count < LIB3D_MAX_PROJECTED; i++) {
        Lib3dVec3 view;
        transform_vertex(state, &obj->vertices[i], obj_x, obj_y, obj_z, &view);
        Lib3dProjected p = {LIB3D_VP_WIDTH / 2, LIB3D_VP_HEIGHT / 2, view.z};
        if (!project(state, &view, &p)) {
            p.z = view.z;
        }
        state->projected[state->proj_count++] = p;
    }

    for (unsigned i = 0; i < obj->polygon_count && state->vis_count < LIB3D_MAX_VISIBLE_POLYS; i++) {
        const X3gPolygon *poly = &obj->parsed_polys[i];
        if (poly->vertex_count < 3) continue;

        bool all_behind = true;
        float avg_z = 0;
        for (int v = 0; v < poly->vertex_count; v++) {
            unsigned vi = base_idx + poly->vertex_indices[v];
            if (vi < state->proj_count && state->projected[vi].z >= LIB3D_NEAR_CLIP)
                all_behind = false;
            if (vi < state->proj_count)
                avg_z += state->projected[vi].z;
        }
        if (all_behind) continue;
        avg_z /= poly->vertex_count;

        Lib3dVisPoly *vp = &state->visible[state->vis_count++];
        vp->vertex_count = poly->vertex_count;
        for (int v = 0; v < poly->vertex_count; v++)
            vp->vertex_indices[v] = (uint8_t)(base_idx + poly->vertex_indices[v]);
        vp->color = poly->color;
        vp->flags = poly->flags;
        vp->normal_x = poly->normal_x;
        vp->normal_y = poly->normal_y;
        vp->avg_z = avg_z;
    }
}

void lib3d_present(Lib3dState *state, uint32_t *dest, int dest_w, int dest_h,
                   int dest_x, int dest_y) {
    if (!state || !dest) return;

    qsort(state->visible, state->vis_count, sizeof(Lib3dVisPoly), poly_compare);

    for (unsigned i = 0; i < state->vis_count; i++) {
        const Lib3dVisPoly *poly = &state->visible[i];
        uint32_t color;
        unsigned ci = poly->color & 0x3F;
        color = 0xFF000000 | (ci * 4) | ((ci * 4) << 8) | ((63 - ci) * 4) << 16;

        float shade = 1.0f;
        if (poly->normal_x != 0 || poly->normal_y != 0) {
            float nx = (float)poly->normal_x / 32768.0f;
            float ny = (float)poly->normal_y / 32768.0f;
            shade = 0.4f + 0.6f * fabsf(nx * 0.5f + ny * 0.866f);
            if (shade > 1.0f) shade = 1.0f;
        }

        uint8_t r = (uint8_t)(((color >> 16) & 0xFF) * shade);
        uint8_t g = (uint8_t)(((color >> 8) & 0xFF) * shade);
        uint8_t b = (uint8_t)((color & 0xFF) * shade);
        color = 0xFF000000 | (r << 16) | (g << 8) | b;

        fill_polygon(state, poly, color);
    }

    for (int y = 0; y < LIB3D_VP_HEIGHT; y++) {
        int dy = dest_y + y;
        if (dy < 0 || dy >= dest_h) continue;
        for (int x = 0; x < LIB3D_VP_WIDTH; x++) {
            int dx = dest_x + x;
            if (dx < 0 || dx >= dest_w) continue;
            dest[dy * dest_w + dx] = state->framebuffer[y * LIB3D_VP_WIDTH + x];
        }
    }
}
