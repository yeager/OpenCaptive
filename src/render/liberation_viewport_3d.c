#include "liberation_viewport_3d.h"
#include "custom_features.h"
#include <limits.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

void lib3d_init(Lib3dState *state) {
    if (!state) return;
    memset(state, 0, sizeof(*state));
    state->fov_scale = 200.0f;
}

void lib3d_set_texture_filter(Lib3dState *state, bool enabled) {
    if (!state) return;
    state->texture_filter = enabled;
}

void lib3d_set_dynamic_lighting(Lib3dState *state, bool enabled) {
    if (!state) return;
    state->dynamic_lighting = enabled;
}

void lib3d_set_camera(Lib3dState *state, float x, float y, float z, float yaw) {
    if (!state) return;
    state->cam_x = x;
    state->cam_y = y;
    state->cam_z = z;
    state->cam_yaw = yaw;
}

void lib3d_clear(Lib3dState *state, uint32_t sky_color, uint32_t ground_color) {
    if (!state) return;
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
    if (!state || !v || !out || !isfinite(v->x) || !isfinite(v->y) ||
        !isfinite(v->z) || !isfinite(state->fov_scale) ||
        v->z < LIB3D_NEAR_CLIP) return false;

    double inv_z = (double)state->fov_scale / (double)v->z;
    double sx = (double)v->x * inv_z + LIB3D_VP_WIDTH / 2.0;
    double sy = -(double)v->y * inv_z + LIB3D_VP_HEIGHT / 2.0;
    /* A finite input can still project outside the representable int range.
     * Check the result before the float-to-int conversion; converting an
     * out-of-range floating value to int is undefined in C. */
    if (!isfinite(sx) || !isfinite(sy) ||
        sx < (double)INT_MIN || sx > (double)INT_MAX ||
        sy < (double)INT_MIN || sy > (double)INT_MAX)
        return false;
    out->sx = (int)sx;
    out->sy = (int)sy;
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

static bool clip_horizontal_span(double left, double right,
                                 int *out_left, int *out_right) {
    if (!out_left || !out_right || !isfinite(left) || !isfinite(right) ||
        (left < 0.0 && right < 0.0) ||
        (left >= LIB3D_VP_WIDTH && right >= LIB3D_VP_WIDTH)) return false;
    if (left < 0.0) left = 0.0;
    if (right < 0.0) right = 0.0;
    if (left >= LIB3D_VP_WIDTH) left = LIB3D_VP_WIDTH - 1;
    if (right >= LIB3D_VP_WIDTH) right = LIB3D_VP_WIDTH - 1;
    *out_left = (int)left;
    *out_right = (int)right;
    return true;
}

static void fill_triangle(Lib3dState *state, int x0, int y0, float z0,
                           int x1, int y1, float z1,
                           int x2, int y2, float z2,
                           uint32_t color) {
    if (y0 > y1) { int t; t=x0;x0=x1;x1=t; t=y0;y0=y1;y1=t; float f=z0;z0=z1;z1=f; }
    if (y0 > y2) { int t; t=x0;x0=x2;x2=t; t=y0;y0=y2;y2=t; float f=z0;z0=z2;z2=f; }
    if (y1 > y2) { int t; t=x1;x1=x2;x2=t; t=y1;y1=y2;y2=t; float f=z1;z1=z2;z2=f; }

    double dy_total = (double)y2 - (double)y0;
    if (dy_total == 0.0 || y2 < 0 || y0 >= LIB3D_VP_HEIGHT) return;
    int first_y = y0 < 0 ? 0 : y0;
    int last_y = y2 >= LIB3D_VP_HEIGHT ? LIB3D_VP_HEIGHT - 1 : y2;

    for (int y = first_y; y <= last_y; y++) {
        bool second_half = (y > y1) || (y1 == y0);
        double seg_height = second_half ? (double)y2 - y1 : (double)y1 - y0;
        if (seg_height == 0.0) seg_height = 1.0;

        double alpha = ((double)y - y0) / dy_total;
        double beta = second_half
            ? ((double)y - y1) / seg_height
            : ((double)y - y0) / seg_height;

        double xa_d = (double)x0 + ((double)x2 - x0) * alpha;
        double xb_d = second_half
            ? (double)x1 + ((double)x2 - x1) * beta
            : (double)x0 + ((double)x1 - x0) * beta;
        int xa, xb;
        if (!clip_horizontal_span(xa_d, xb_d, &xa, &xb)) continue;
        float za = z0 + (float)((z2 - z0) * alpha);
        float zb = second_half ? z1 + (float)((z2 - z1) * beta) :
                                z0 + (float)((z1 - z0) * beta);

        if (xa > xb) { int t = xa; xa = xb; xb = t; float f = za; za = zb; zb = f; }

        for (int x = xa; x <= xb; x++) {
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
    if (!state || !obj || !obj->vertices || !obj->parsed_polys) return;
    if (palette && pal_size > 0) {
        state->palette = palette;
        state->pal_size = pal_size;
    }

    unsigned base_idx = state->proj_count;

    for (unsigned i = 0; i < obj->vertex_count && state->proj_count < LIB3D_MAX_PROJECTED; i++) {
        Lib3dVec3 view;
        transform_vertex(state, &obj->vertices[i], obj_x, obj_y, obj_z, &view);
        Lib3dProjected p = {LIB3D_VP_WIDTH / 2, LIB3D_VP_HEIGHT / 2, view.z};
        if (!project(state, &view, &p)) {
            p.z = -INFINITY;
        }
        state->projected[state->proj_count++] = p;
    }

    for (unsigned i = 0; i < obj->polygon_count && state->vis_count < LIB3D_MAX_VISIBLE_POLYS; i++) {
        const X3gPolygon *poly = &obj->parsed_polys[i];
        if (poly->vertex_count < 3 || poly->vertex_count > 8) continue;

        bool all_behind = true;
        bool invalid_vertex = false;
        float avg_z = 0;
        for (int v = 0; v < poly->vertex_count; v++) {
            unsigned object_vertex = poly->vertex_indices[v];
            if (object_vertex >= obj->vertex_count) {
                invalid_vertex = true;
                continue;
            }
            unsigned vi = base_idx + object_vertex;
            if (vi >= state->proj_count ||
                !isfinite(state->projected[vi].z)) {
                invalid_vertex = true;
                continue;
            }
            if (state->projected[vi].z >= LIB3D_NEAR_CLIP)
                all_behind = false;
            avg_z += state->projected[vi].z;
        }
        if (all_behind || invalid_vertex || !isfinite(avg_z)) continue;
        avg_z /= poly->vertex_count;

        Lib3dVisPoly *vp = &state->visible[state->vis_count++];
        vp->vertex_count = poly->vertex_count;
        for (int v = 0; v < poly->vertex_count; v++)
            vp->vertex_indices[v] = (uint16_t)(base_idx + poly->vertex_indices[v]);
        vp->color = poly->color;
        vp->flags = poly->flags;
        vp->normal_x = poly->normal_x;
        vp->normal_y = poly->normal_y;
        vp->avg_z = avg_z;
    }
}

static void textured_scanline(Lib3dState *state, int y,
                               int xa, float za, float ua, float va,
                               int xb, float zb, float ub, float vb,
                               const Lib3dTexture *tex) {
    if (y < 0 || y >= LIB3D_VP_HEIGHT || !tex ||
        tex->width <= 0 || tex->height <= 0 ||
        tex->width > LIB3D_MAX_TEX_SIZE || tex->height > LIB3D_MAX_TEX_SIZE ||
        !isfinite(za) || !isfinite(zb) ||
        !isfinite(ua) || !isfinite(va) || !isfinite(ub) || !isfinite(vb)) return;
    if (xa > xb) {
        int ti = xa; xa = xb; xb = ti;
        float tf;
        tf = za; za = zb; zb = tf;
        tf = ua; ua = ub; ub = tf;
        tf = va; va = vb; vb = tf;
    }
    if ((xa < 0 && xb < 0) ||
        (xa >= LIB3D_VP_WIDTH && xb >= LIB3D_VP_WIDTH)) return;
    if (xa < 0) xa = 0;
    if (xb < 0) xb = 0;
    if (xa >= LIB3D_VP_WIDTH) xa = LIB3D_VP_WIDTH - 1;
    if (xb >= LIB3D_VP_WIDTH) xb = LIB3D_VP_WIDTH - 1;
    if (xa > xb) {
        int ti = xa; xa = xb; xb = ti;
        float tf;
        tf = za; za = zb; zb = tf;
        tf = ua; ua = ub; ub = tf;
        tf = va; va = vb; vb = tf;
    }
    float inv_za = (za > 0.001f) ? 1.0f / za : 0;
    float inv_zb = (zb > 0.001f) ? 1.0f / zb : 0;
    float uoz_a = ua * inv_za, voz_a = va * inv_za;
    float uoz_b = ub * inv_zb, voz_b = vb * inv_zb;

    for (int x = xa; x <= xb; x++) {
        if (x < 0 || x >= LIB3D_VP_WIDTH) continue;
        float t = (xb != xa) ? (float)(x - xa) / (xb - xa) : 0;
        float inv_z = inv_za + (inv_zb - inv_za) * t;
        float z = (inv_z > 0.0001f) ? 1.0f / inv_z : 1e30f;
        int idx = y * LIB3D_VP_WIDTH + x;
        if (z >= state->zbuffer[idx]) continue;

        float u_over_z = uoz_a + (uoz_b - uoz_a) * t;
        float v_over_z = voz_a + (voz_b - voz_a) * t;
        float u = u_over_z * z;
        float v = v_over_z * z;
        float tex_u = u * tex->width;
        float tex_v = v * tex->height;
        if (!isfinite(tex_u) || !isfinite(tex_v)) continue;

        /* fmodf keeps the conversion bounded even when perspective math
           produces very large, but still finite, texture coordinates. */
        float wrapped_u = fmodf(tex_u, (float)tex->width);
        float wrapped_v = fmodf(tex_v, (float)tex->height);
        int tx = (int)wrapped_u;
        int ty = (int)wrapped_v;
        if (tx < 0) tx += tex->width;
        if (ty < 0) ty += tex->height;

        uint32_t texel;
        if (!state->texture_filter) {
            texel = tex->pixels[ty * tex->width + tx];
        } else {
            /* Bilinear filtering is optional because the original renderer
             * is nearest-neighbour.  Wrap at texture edges, matching the
             * nearest-neighbour path's repeat behaviour. */
            float fu = wrapped_u - floorf(wrapped_u);
            float fv = wrapped_v - floorf(wrapped_v);
            int tx1 = (tx + 1) % tex->width;
            int ty1 = (ty + 1) % tex->height;
            uint32_t c00 = tex->pixels[ty * tex->width + tx];
            uint32_t c10 = tex->pixels[ty * tex->width + tx1];
            uint32_t c01 = tex->pixels[ty1 * tex->width + tx];
            uint32_t c11 = tex->pixels[ty1 * tex->width + tx1];
            float w00 = (1.0f - fu) * (1.0f - fv);
            float w10 = fu * (1.0f - fv);
            float w01 = (1.0f - fu) * fv;
            float w11 = fu * fv;
            uint8_t a = (uint8_t)(((c00 >> 24) & 0xFF) * w00 +
                                  ((c10 >> 24) & 0xFF) * w10 +
                                  ((c01 >> 24) & 0xFF) * w01 +
                                  ((c11 >> 24) & 0xFF) * w11);
            uint8_t r = (uint8_t)(((c00 >> 16) & 0xFF) * w00 +
                                  ((c10 >> 16) & 0xFF) * w10 +
                                  ((c01 >> 16) & 0xFF) * w01 +
                                  ((c11 >> 16) & 0xFF) * w11);
            uint8_t g = (uint8_t)(((c00 >> 8) & 0xFF) * w00 +
                                  ((c10 >> 8) & 0xFF) * w10 +
                                  ((c01 >> 8) & 0xFF) * w01 +
                                  ((c11 >> 8) & 0xFF) * w11);
            uint8_t b = (uint8_t)((c00 & 0xFF) * w00 +
                                  (c10 & 0xFF) * w10 +
                                  (c01 & 0xFF) * w01 +
                                  (c11 & 0xFF) * w11);
            texel = ((uint32_t)a << 24) | ((uint32_t)r << 16) |
                    ((uint32_t)g << 8) | b;
        }
        if (state->dynamic_lighting) {
            int distance = z > (float)INT_MAX ? INT_MAX :
                           z < 0.0f ? 0 : (int)z;
            texel = lighting_shade_pixel(
                texel, lighting_compute_intensity(distance, 127));
        }
        if ((texel >> 24) == 0) continue;

        state->zbuffer[idx] = z;
        state->framebuffer[idx] = texel;
    }
}

void lib3d_render_textured_quad(Lib3dState *state,
                                float x0, float y0, float z0,
                                float x1, float y1, float z1,
                                float x2, float y2, float z2,
                                float x3, float y3, float z3,
                                const Lib3dTexture *tex) {
    if (!state || !tex || tex->width <= 0 || tex->height <= 0 ||
        tex->width > LIB3D_MAX_TEX_SIZE || tex->height > LIB3D_MAX_TEX_SIZE ||
        !isfinite(state->cam_x) || !isfinite(state->cam_y) ||
        !isfinite(state->cam_z) || !isfinite(state->cam_yaw) ||
        !isfinite(state->fov_scale) ||
        !isfinite(x0) || !isfinite(y0) || !isfinite(z0) ||
        !isfinite(x1) || !isfinite(y1) || !isfinite(z1) ||
        !isfinite(x2) || !isfinite(y2) || !isfinite(z2) ||
        !isfinite(x3) || !isfinite(y3) || !isfinite(z3)) return;

    Lib3dVec3 view[4];
    float wx[4] = {x0, x1, x2, x3};
    float wy[4] = {y0, y1, y2, y3};
    float wz[4] = {z0, z1, z2, z3};

    float cos_y = cosf(state->cam_yaw);
    float sin_y = sinf(state->cam_yaw);

    for (int i = 0; i < 4; i++) {
        float dx = wx[i] - state->cam_x;
        float dy = wy[i] - state->cam_y;
        float dz = wz[i] - state->cam_z;
        view[i].x = dx * cos_y - dz * sin_y;
        view[i].y = dy;
        view[i].z = dx * sin_y + dz * cos_y;
    }

    static const float uv[4][2] = {{0,0},{1,0},{1,1},{0,1}};
    Lib3dProjected proj[4];
    bool behind[4];
    for (int i = 0; i < 4; i++) {
        /* project() intentionally leaves its output untouched on rejection.
         * Mixed front/behind quads still reach the rasterizer, so initialize
         * the fallback coordinates before attempting the projection. */
        proj[i] = (Lib3dProjected){
            LIB3D_VP_WIDTH / 2, LIB3D_VP_HEIGHT / 2, view[i].z
        };
        behind[i] = !project(state, &view[i], &proj[i]);
        if (behind[i]) proj[i].z = view[i].z;
    }

    int tris[2][3] = {{0,1,2},{0,2,3}};
    for (int t = 0; t < 2; t++) {
        int i0 = tris[t][0], i1 = tris[t][1], i2 = tris[t][2];
        if (behind[i0] && behind[i1] && behind[i2]) continue;

        int sy0 = proj[i0].sy, sy1 = proj[i1].sy, sy2 = proj[i2].sy;
        float sz0 = proj[i0].z, sz1 = proj[i1].z, sz2 = proj[i2].z;
        int sx0 = proj[i0].sx, sx1 = proj[i1].sx, sx2 = proj[i2].sx;
        float u0 = uv[i0][0], v0 = uv[i0][1];
        float u1 = uv[i1][0], v1 = uv[i1][1];
        float u2 = uv[i2][0], v2 = uv[i2][1];

        if (sy0 > sy1) { int tt; tt=sx0;sx0=sx1;sx1=tt; tt=sy0;sy0=sy1;sy1=tt;
                          float ff; ff=sz0;sz0=sz1;sz1=ff; ff=u0;u0=u1;u1=ff; ff=v0;v0=v1;v1=ff; }
        if (sy0 > sy2) { int tt; tt=sx0;sx0=sx2;sx2=tt; tt=sy0;sy0=sy2;sy2=tt;
                          float ff; ff=sz0;sz0=sz2;sz2=ff; ff=u0;u0=u2;u2=ff; ff=v0;v0=v2;v2=ff; }
        if (sy1 > sy2) { int tt; tt=sx1;sx1=sx2;sx2=tt; tt=sy1;sy1=sy2;sy2=tt;
                          float ff; ff=sz1;sz1=sz2;sz2=ff; ff=u1;u1=u2;u2=ff; ff=v1;v1=v2;v2=ff; }

        double dy_total = (double)sy2 - (double)sy0;
        if (dy_total == 0.0 || sy2 < 0 || sy0 >= LIB3D_VP_HEIGHT) continue;
        int first_y = sy0 < 0 ? 0 : sy0;
        int last_y = sy2 >= LIB3D_VP_HEIGHT ? LIB3D_VP_HEIGHT - 1 : sy2;

        for (int y = first_y; y <= last_y; y++) {
            bool second = (y > sy1) || (sy1 == sy0);
            double seg = second ? (double)sy2 - sy1 : (double)sy1 - sy0;
            if (seg == 0.0) seg = 1.0;

            double alpha = ((double)y - sy0) / dy_total;
            double beta = second ? ((double)y - sy1) / seg :
                                   ((double)y - sy0) / seg;

            double xa_d = (double)sx0 + ((double)sx2 - sx0) * alpha;
            double xb_d = second ? (double)sx1 + ((double)sx2 - sx1) * beta :
                                   (double)sx0 + ((double)sx1 - sx0) * beta;
            int xa, xb;
            if (!clip_horizontal_span(xa_d, xb_d, &xa, &xb)) continue;
            float za = sz0 + (float)((sz2 - sz0) * alpha);
            float zb = second ? sz1 + (float)((sz2 - sz1) * beta) :
                                sz0 + (float)((sz1 - sz0) * beta);
            float ua_l = u0 + (float)((u2 - u0) * alpha);
            float va_l = v0 + (float)((v2 - v0) * alpha);
            float ub_l = second ? u1 + (float)((u2 - u1) * beta) :
                                  u0 + (float)((u1 - u0) * beta);
            float vb_l = second ? v1 + (float)((v2 - v1) * beta) :
                                  v0 + (float)((v1 - v0) * beta);

            textured_scanline(state, y, xa, za, ua_l, va_l, xb, zb, ub_l, vb_l, tex);
        }
    }
}

void lib3d_present(Lib3dState *state, uint32_t *dest, int dest_w, int dest_h,
                   int dest_x, int dest_y) {
    if (!state || !dest || dest_w <= 0 || dest_h <= 0 ||
        state->vis_count > LIB3D_MAX_VISIBLE_POLYS) return;

    qsort(state->visible, state->vis_count, sizeof(Lib3dVisPoly), poly_compare);

    for (unsigned i = 0; i < state->vis_count; i++) {
        const Lib3dVisPoly *poly = &state->visible[i];
        unsigned ci = poly->color & 0x3F;
        /* Colours come from the original palette or the polygon is not drawn.
         * The previous synthetic ramp invented a colour for every polygon
         * whenever the palette was absent or shorter than the index. */
        if (!state->palette || ci >= state->pal_size) continue;
        uint32_t color = state->palette[ci];

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
        int64_t dy = (int64_t)dest_y + y;
        if (dy < 0 || dy >= dest_h) continue;
        for (int x = 0; x < LIB3D_VP_WIDTH; x++) {
            int64_t dx = (int64_t)dest_x + x;
            if (dx < 0 || dx >= dest_w) continue;
            dest[dy * dest_w + dx] = state->framebuffer[y * LIB3D_VP_WIDTH + x];
        }
    }
}
