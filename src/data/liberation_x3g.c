#include "liberation_x3g.h"
#include <stdint.h>
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

static void free_object(X3gObject *obj) {
    if (!obj) return;
    free(obj->vertices);
    free(obj->parsed_polys);
    memset(obj, 0, sizeof(*obj));
}

static bool parse_vcdo(X3gObject *obj, const uint8_t *data, size_t size) {
    size_t pos = 0;
    obj->vertex_count = 0;
    obj->vertices = NULL;
    obj->polygons.data = NULL;
    obj->polygons.size = 0;
    obj->extra = NULL;
    obj->extra_size = 0;

    while (size - pos >= 8) {
        if (tag_eq(data + pos, "EXVL")) {
            uint32_t chunk_size = be32(data + pos + 4);
            /* EXVL starts with a 16-bit vertex count.  Check that prefix
             * before reading it; a zero-length chunk otherwise reads past
             * the supplied X3G buffer. */
            if (chunk_size < 2U ||
                (size_t)chunk_size > size - pos - 8U) return false;

            const uint8_t *exvl = data + pos + 8;
            unsigned nv = be16(exvl);
            if (nv > X3G_MAX_VERTICES) return false;

            free(obj->vertices);
            obj->vertices = calloc(nv, sizeof(X3gVertex));
            if (!obj->vertices) return false;
            obj->vertex_count = nv;

            for (unsigned v = 0; v < nv; v++) {
                size_t voff = 2 + v * 16;
                if (voff + 16 > chunk_size) {
                    free(obj->vertices);
                    obj->vertices = NULL;
                    obj->vertex_count = 0;
                    return false;
                }
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
            if (chunk_size & 1) {
                if (pos >= size) return false;
                pos++;
            }
        } else if (tag_eq(data + pos, "PLST")) {
            uint32_t chunk_size = be32(data + pos + 4);
            if (chunk_size > UINT16_MAX ||
                (size_t)chunk_size > size - pos - 8U) return false;
            obj->polygons.data = data + pos + 8;
            obj->polygons.size = (uint16_t)chunk_size;
            pos += 8 + chunk_size;
            if (chunk_size & 1) {
                if (pos >= size) return false;
                pos++;
            }
        } else {
            uint32_t chunk_size = be32(data + pos + 4);
            if (chunk_size > size - pos - 8U) return false;
            pos += 8U + chunk_size;
            if (chunk_size & 1) {
                if (pos >= size) return false;
                pos++;
            }
        }
    }

    if (obj->polygons.data && obj->polygons.size >= 4) {
        const uint8_t *p = obj->polygons.data;
        unsigned psz = obj->polygons.size;
        unsigned count = 0;
        unsigned off = 0;
        bool malformed = false;
        while (off + 4 <= psz) {
            uint16_t rsz = be16(p + off + 2);
            if (be16(p + off) == 0 && rsz == 0) break;
            if (rsz < 38 || off + rsz > psz) {
                malformed = true;
                break;
            }
            count++;
            off += rsz;
        }
        if (off < psz && !(off + 4 <= psz && be16(p + off) == 0 &&
                           be16(p + off + 2) == 0))
            malformed = true;
        if (malformed || count > X3G_MAX_POLYGONS) return false;
        if (count > 0) {
            obj->parsed_polys = calloc(count, sizeof(X3gPolygon));
            if (!obj->parsed_polys) return false;
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
                    unsigned vertex_index = vref / 16U;
                    if ((vref & 15U) != 0U || vertex_index >= obj->vertex_count) {
                        free_object(obj);
                        return false;
                    }
                    pg->vertex_indices[v] = (uint8_t)vertex_index;
                }
                off += pg->record_size;
            }
        }
    }

    return obj->vertex_count > 0;
}

bool x3g_open(X3gFile *x3g, const uint8_t *data, size_t size) {
    if (!x3g) return false;
    memset(x3g, 0, sizeof(*x3g));
    if (!data || size < 20) return false;
    x3g->raw_data = data;
    x3g->raw_size = size;

    if (!tag_eq(data, "FORM")) return false;
    uint32_t form_size = be32(data + 4);
    if (!tag_eq(data + 8, "O3DG")) return false;
    if (form_size < 4U || (uint64_t)form_size + 8U > size) return false;

    size_t pos = 12;

    if (size - pos < 8 || !tag_eq(data + pos, "OFFS")) return false;
    uint32_t offs_size = be32(data + pos + 4);
    if (offs_size < 6 || (size_t)offs_size > size - pos - 8U) return false;
    unsigned obj_count = be16(data + pos + 8);
    if (obj_count > X3G_MAX_OBJECTS) obj_count = X3G_MAX_OBJECTS;
    pos += 8 + offs_size;
    if (offs_size & 1) {
        if (pos >= size) {
            x3g_close(x3g);
            return false;
        }
        pos++;
    }

    for (unsigned i = 0; i < obj_count && size - pos >= 12; i++) {
        if (!tag_eq(data + pos, "FORM")) break;
        uint32_t vcdo_size = be32(data + pos + 4);
        if (vcdo_size < 4U || (size_t)vcdo_size > size - pos - 8U ||
            !tag_eq(data + pos + 8, "VCDO")) break;

        X3gObject *obj = &x3g->objects[x3g->object_count];
        if (parse_vcdo(obj, data + pos + 12, vcdo_size - 4))
            x3g->object_count++;
        else
            free_object(obj);

        pos += 8 + vcdo_size;
        if (vcdo_size & 1) {
            if (pos >= size) {
                x3g_close(x3g);
                return false;
            }
            pos++;
        }
    }

    return x3g->object_count > 0;
}

void x3g_close(X3gFile *x3g) {
    if (!x3g) return;
    for (unsigned i = 0; i < x3g->object_count; i++) {
        free_object(&x3g->objects[i]);
    }
    memset(x3g, 0, sizeof(*x3g));
}
