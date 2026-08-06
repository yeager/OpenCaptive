#include "liberation_x3g.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

/* Test data: FORM O3DG with 1 VCDO containing 3 vertices and 1 triangle polygon */
static const uint8_t test_x3g[] = {
    'F','O','R','M', 0,0,0,144, /* FORM, size=144 */
    'O','3','D','G',             /* type */
    'O','F','F','S', 0,0,0,6,   /* OFFS chunk, size=6 */
    0,1, 0,0,0,64,              /* count=1, vftx_offset=64 */
    'F','O','R','M', 0,0,0,118, /* VCDO FORM, size=118 */
    'V','C','D','O',             /* type */
    'E','X','V','L', 0,0,0,50,  /* EXVL chunk, size=50 */
    0,3,                         /* 3 vertices */
    0,10, 0,20, 0,30, 0,0, 0,0, 0,0, 0,0, 0,0,  /* v0 */
    0xff,0xf6, 0,40, 0xff,0xce, 0,1, 0,0, 0,0, 0,0, 0,0,  /* v1 */
    0,50, 0,60, 0,70, 0,2, 0,0, 0,0, 0,0, 0,0,  /* v2 */
    'P','L','S','T', 0,0,0,48,  /* PLST chunk, size=48 (44 record + 4 terminator) */
    /* polygon record: type=0, size=44 */
    0,0, 0,44,                   /* type=0, size=44 */
    0,0, 0,0, 0,0, 0,0,         /* w2-w5 */
    0x17,0x00, 0x17,0x00,        /* w6, w7 */
    0,48, 0,48,                  /* normal_x=48, normal_y=48 */
    0x17,0x00,                   /* flags=0x1700 */
    0,100,                       /* color=100 */
    0,50, 0,160, 0,84, 0,0,     /* UV rect */
    0,1,                         /* w16=1 */
    0,3,                         /* vertex count=3 (word 17) */
    /* 3 vertex refs + closing (byte offsets: 0, 16, 32, 0) */
    0,0, 0,16, 0,32, 0,0,
    /* terminator */
    0,0, 0,0,
};

static void test_parse(void) {
    X3gFile x3g;
    assert(x3g_open(&x3g, test_x3g, sizeof(test_x3g)));
    assert(x3g.object_count == 1);
    assert(x3g.objects[0].vertex_count == 3);
    assert(x3g.objects[0].vertices[0].x == 10);
    assert(x3g.objects[0].vertices[0].y == 20);
    assert(x3g.objects[0].vertices[0].z == 30);
    assert(x3g.objects[0].vertices[1].x == -10);
    assert(x3g.objects[0].vertices[1].y == 40);
    assert(x3g.objects[0].vertices[1].z == -50);
    assert(x3g.objects[0].vertices[1].group == 1);
    assert(x3g.objects[0].polygons.data != NULL);
    x3g_close(&x3g);
}

static void test_polygon_decode(void) {
    X3gFile x3g;
    assert(x3g_open(&x3g, test_x3g, sizeof(test_x3g)));
    assert(x3g.objects[0].polygon_count == 1);

    const X3gPolygon *pg = &x3g.objects[0].parsed_polys[0];
    assert(pg->type == 0);
    assert(pg->record_size == 44);
    assert(pg->normal_x == 48);
    assert(pg->normal_y == 48);
    assert(pg->flags == 0x1700);
    assert(pg->color == 100);
    assert(pg->uv_left == 50);
    assert(pg->uv_top == 160);
    assert(pg->uv_right == 84);
    assert(pg->uv_bottom == 0);
    assert(pg->vertex_count == 3);
    assert(pg->vertex_indices[0] == 0);
    assert(pg->vertex_indices[1] == 1);
    assert(pg->vertex_indices[2] == 2);
    x3g_close(&x3g);
}

static void test_invalid(void) {
    X3gFile x3g;
    assert(!x3g_open(&x3g, NULL, 0));
    assert(!x3g_open(&x3g, (const uint8_t *)"bad", 3));

    /* An empty EXVL chunk must be rejected before its missing vertex count
     * is read. */
    static const uint8_t empty_exvl[] = {
        'F','O','R','M', 0,0,0,38, 'O','3','D','G',
        'O','F','F','S', 0,0,0,6, 0,1, 0,0, 0,0,
        'F','O','R','M', 0,0,0,12, 'V','C','D','O',
        'E','X','V','L', 0,0,0,0
    };
    assert(!x3g_open(&x3g, empty_exvl, sizeof(empty_exvl)));

    uint8_t bad[20] = {'F','O','R','M', 0,0,0,8, 'B','A','D','!', 0,0,0,0, 0,0,0,0};
    assert(!x3g_open(&x3g, bad, sizeof(bad)));

    assert(!x3g_open(&x3g, NULL, 0));
    assert(x3g.raw_data == NULL && x3g.raw_size == 0 && x3g.object_count == 0);
}

static void test_close_null(void) {
    x3g_close(NULL);
    X3gFile x3g;
    memset(&x3g, 0, sizeof(x3g));
    x3g_close(&x3g);
}

int main(void) {
    test_parse();
    test_polygon_decode();
    test_invalid();
    test_close_null();
    printf("All x3g tests passed\n");
    return 0;
}
