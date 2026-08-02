#ifndef LIBERATION_X3G_H
#define LIBERATION_X3G_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define X3G_MAX_OBJECTS    64
#define X3G_MAX_VERTICES  256
#define X3G_MAX_POLYGONS  256
#define X3G_MAX_POLY_VERTS 8

typedef struct {
    int16_t x, y, z;
    int16_t group;
    int16_t reserved[4];
} X3gVertex;

typedef struct {
    uint16_t type;
    uint16_t record_size;
    int16_t  w2, w3, w4, w5;
    int16_t  w6, w7;
    int16_t  normal_x, normal_y;
    uint16_t flags;
    uint16_t color;
    uint16_t uv_left, uv_top, uv_right, uv_bottom;
    uint16_t w16;
    uint8_t  vertex_count;
    uint8_t  vertex_indices[X3G_MAX_POLY_VERTS];
} X3gPolygon;

typedef struct {
    const uint8_t *data;
    uint16_t       size;
} X3gPolygonList;

typedef struct {
    X3gVertex      *vertices;
    unsigned        vertex_count;
    X3gPolygonList  polygons;
    X3gPolygon     *parsed_polys;
    unsigned        polygon_count;
    const uint8_t  *extra;
    unsigned        extra_size;
} X3gObject;

typedef struct {
    X3gObject  objects[X3G_MAX_OBJECTS];
    unsigned   object_count;
    const uint8_t *raw_data;
    size_t         raw_size;
} X3gFile;

bool x3g_open(X3gFile *x3g, const uint8_t *data, size_t size);
void x3g_close(X3gFile *x3g);

#endif
