#include "liberation_x3g.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

/* Minimal Objects.x3g-like test data: FORM O3DG with 1 VCDO containing 3 vertices */
static const uint8_t test_x3g[] = {
    'F','O','R','M', 0,0,0,84,  /* FORM, size=84 */
    'O','3','D','G',             /* type */
    'O','F','F','S', 0,0,0,6,   /* OFFS chunk, size=6 */
    0,1, 0,0,0,64,              /* count=1, vftx_offset=64 */
    'F','O','R','M', 0,0,0,58,  /* VCDO FORM, size=58 */
    'V','C','D','O',             /* type */
    'E','X','V','L', 0,0,0,34,  /* EXVL chunk, size=34 */
    0,2,                         /* 2 vertices */
    0,10, 0,20, 0,30, 0,0, 0,0, 0,0, 0,0, 0,0,  /* v0: x=10,y=20,z=30 */
    0xff,0xf6, 0,40, 0xff,0xce, 0,1, 0,0, 0,0, 0,0, 0,0,  /* v1: x=-10,y=40,z=-50,group=1 */
    'P','L','S','T', 0,0,0,4,   /* PLST chunk, size=4 */
    0,0, 0,0,                    /* minimal polygon data */
};

static void test_parse(void) {
    X3gFile x3g;
    assert(x3g_open(&x3g, test_x3g, sizeof(test_x3g)));
    assert(x3g.object_count == 1);
    assert(x3g.objects[0].vertex_count == 2);
    assert(x3g.objects[0].vertices[0].x == 10);
    assert(x3g.objects[0].vertices[0].y == 20);
    assert(x3g.objects[0].vertices[0].z == 30);
    assert(x3g.objects[0].vertices[1].x == -10);
    assert(x3g.objects[0].vertices[1].y == 40);
    assert(x3g.objects[0].vertices[1].z == -50);
    assert(x3g.objects[0].vertices[1].group == 1);
    assert(x3g.objects[0].polygons.data != NULL);
    assert(x3g.objects[0].polygons.size == 4);
    x3g_close(&x3g);
}

static void test_invalid(void) {
    X3gFile x3g;
    assert(!x3g_open(&x3g, NULL, 0));
    assert(!x3g_open(&x3g, (const uint8_t *)"bad", 3));

    uint8_t bad[20] = {'F','O','R','M', 0,0,0,8, 'B','A','D','!', 0,0,0,0, 0,0,0,0};
    assert(!x3g_open(&x3g, bad, sizeof(bad)));
}

static void test_close_null(void) {
    x3g_close(NULL);
    X3gFile x3g;
    memset(&x3g, 0, sizeof(x3g));
    x3g_close(&x3g);
}

int main(void) {
    test_parse();
    test_invalid();
    test_close_null();
    printf("All x3g tests passed\n");
    return 0;
}
