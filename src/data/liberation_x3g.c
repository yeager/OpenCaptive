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
    for (unsigned i = 0; i < x3g->object_count; i++)
        free(x3g->objects[i].vertices);
    memset(x3g, 0, sizeof(*x3g));
}
