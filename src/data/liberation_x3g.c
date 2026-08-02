#include "liberation_x3g.h"
#include <stdlib.h>
#include <string.h>

static uint16_t be16(const uint8_t *p) { return (uint16_t)((p[0] << 8) | p[1]); }
static int16_t  bes16(const uint8_t *p) { return (int16_t)be16(p); }
static uint32_t be32(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | p[3];
}

static bool tag_eq(const uint8_t *p, const char *tag) {
    return p[0] == tag[0] && p[1] == tag[1] && p[2] == tag[2] && p[3] == tag[3];
}

static bool parse_vcdo(X3gObject *obj, const uint8_t *data, size_t size) {
    size_t pos = 0;
    obj->vertex_count = 0;
    obj->vertices = NULL;
    obj->polygons.data = NULL;
    obj->polygons.size = 0;
    obj->extra = NULL;
    obj->extra_size = 0;

    while (pos + 8 <= size) {
        if (tag_eq(data + pos, "EXVL")) {
            uint32_t chunk_size = be32(data + pos + 4);
            if (pos + 8 + chunk_size > size) return false;

            const uint8_t *exvl = data + pos + 8;
            unsigned nv = be16(exvl);
            if (nv > X3G_MAX_VERTICES) return false;

            obj->vertices = calloc(nv, sizeof(X3gVertex));
            if (!obj->vertices) return false;
            obj->vertex_count = nv;

            for (unsigned v = 0; v < nv; v++) {
                size_t voff = 2 + v * 16;
                if (voff + 16 > chunk_size) break;
                obj->vertices[v].x = bes16(exvl + voff);
                obj->vertices[v].y = bes16(exvl + voff + 2);
                obj->vertices[v].z = bes16(exvl + voff + 4);
                obj->vertices[v].group = bes16(exvl + voff + 6);
                for (int i = 0; i < 4; i++)
                    obj->vertices[v].reserved[i] = bes16(exvl + voff + 8 + i * 2);
            }

            size_t vert_end = 2 + nv * 16;
            if (vert_end < chunk_size) {
                obj->extra = exvl + vert_end;
                obj->extra_size = (unsigned)(chunk_size - vert_end);
            }

            pos += 8 + chunk_size;
            if (chunk_size & 1) pos++;
        } else if (tag_eq(data + pos, "PLST")) {
            uint32_t chunk_size = be32(data + pos + 4);
            if (pos + 8 + chunk_size > size) return false;
            obj->polygons.data = data + pos + 8;
            obj->polygons.size = (uint16_t)chunk_size;
            pos += 8 + chunk_size;
            if (chunk_size & 1) pos++;
        } else {
            uint32_t chunk_size = be32(data + pos + 4);
            pos += 8 + chunk_size;
            if (chunk_size & 1) pos++;
        }
    }

    if (obj->polygons.data && obj->polygons.size >= 4) {
        const uint8_t *p = obj->polygons.data;
        unsigned psz = obj->polygons.size;
        unsigned count = 0;
        unsigned off = 0;
        while (off + 4 <= psz) {
            uint16_t rsz = be16(p + off + 2);
            if (be16(p + off) == 0 && rsz == 0) break;
            if (rsz < 38 || off + rsz > psz) break;
            count++;
            off += rsz;
        }
        if (count > 0 && count <= X3G_MAX_POLYGONS) {
            obj->parsed_polys = calloc(count, sizeof(X3gPolygon));
            if (obj->parsed_polys) {
                obj->polygon_count = count;
                off = 0;
                for (unsigned i = 0; i < count; i++) {
                    X3gPolygon *pg = &obj->parsed_polys[i];
                    pg->type = be16(p + off);
                    pg->record_size = be16(p + off + 2);
                    pg->w2 = bes16(p + off + 4);
                    pg->w3 = bes16(p + off + 6);
                    pg->w4 = bes16(p + off + 8);
                    pg->w5 = bes16(p + off + 10);
                    pg->w6 = bes16(p + off + 12);
                    pg->w7 = bes16(p + off + 14);
                    pg->normal_x = bes16(p + off + 16);
                    pg->normal_y = bes16(p + off + 18);
                    pg->flags = be16(p + off + 20);
                    pg->color = be16(p + off + 22);
                    pg->uv_left = be16(p + off + 24);
                    pg->uv_top = be16(p + off + 26);
                    pg->uv_right = be16(p + off + 28);
                    pg->uv_bottom = be16(p + off + 30);
                    pg->w16 = be16(p + off + 32);
                    unsigned nv = (pg->record_size - 38) / 2;
                    if (nv > X3G_MAX_POLY_VERTS) nv = X3G_MAX_POLY_VERTS;
                    pg->vertex_count = (uint8_t)nv;
                    for (unsigned v = 0; v < nv; v++) {
                        uint16_t vref = be16(p + off + 36 + v * 2);
                        pg->vertex_indices[v] = (uint8_t)(vref / 16);
                    }
                    off += pg->record_size;
                }
            }
        }
    }

    return obj->vertex_count > 0;
}

bool x3g_open(X3gFile *x3g, const uint8_t *data, size_t size) {
    if (!x3g || !data || size < 20) return false;
    memset(x3g, 0, sizeof(*x3g));
    x3g->raw_data = data;
    x3g->raw_size = size;

    if (!tag_eq(data, "FORM")) return false;
    uint32_t form_size = be32(data + 4);
    if (!tag_eq(data + 8, "O3DG")) return false;
    (void)form_size;

    size_t pos = 12;

    if (pos + 8 > size || !tag_eq(data + pos, "OFFS")) return false;
    uint32_t offs_size = be32(data + pos + 4);
    if (offs_size < 6 || pos + 8 + offs_size > size) return false;
    unsigned obj_count = be16(data + pos + 8);
    if (obj_count > X3G_MAX_OBJECTS) obj_count = X3G_MAX_OBJECTS;
    pos += 8 + offs_size;
    if (offs_size & 1) pos++;

    for (unsigned i = 0; i < obj_count && pos + 12 <= size; i++) {
        if (!tag_eq(data + pos, "FORM")) break;
        uint32_t vcdo_size = be32(data + pos + 4);
        if (!tag_eq(data + pos + 8, "VCDO")) break;

        X3gObject *obj = &x3g->objects[x3g->object_count];
        if (parse_vcdo(obj, data + pos + 12, vcdo_size - 4))
            x3g->object_count++;

        pos += 8 + vcdo_size;
        if (vcdo_size & 1) pos++;
    }

    return x3g->object_count > 0;
}

void x3g_close(X3gFile *x3g) {
    if (!x3g) return;
    for (unsigned i = 0; i < x3g->object_count; i++) {
        free(x3g->objects[i].vertices);
        free(x3g->objects[i].parsed_polys);
    }
    memset(x3g, 0, sizeof(*x3g));
}
