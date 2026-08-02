#include "liberation_viewport_3d.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static void test_init(void) {
    Lib3dState state;
    lib3d_init(&state);
    assert(state.fov_scale > 0);
    assert(state.proj_count == 0);
    assert(state.vis_count == 0);
}

static void test_clear(void) {
    Lib3dState state;
    lib3d_init(&state);
    lib3d_clear(&state, 0xFF4488CC, 0xFF224400);

    assert(state.framebuffer[0] == 0xFF4488CC);
    assert(state.framebuffer[LIB3D_VP_WIDTH * (LIB3D_VP_HEIGHT - 1)] == 0xFF224400);
    assert(state.zbuffer[0] > 1e20f);
}

static void test_render_triangle(void) {
    Lib3dState state;
    lib3d_init(&state);
    lib3d_set_camera(&state, 0, 0, 0, 0);
    lib3d_clear(&state, 0xFF000000, 0xFF000000);

    X3gVertex verts[3] = {
        { -20, -20, 100, 0, {0,0,0,0} },
        {  20, -20, 100, 0, {0,0,0,0} },
        {   0,  20, 100, 0, {0,0,0,0} },
    };

    X3gPolygon poly;
    memset(&poly, 0, sizeof(poly));
    poly.vertex_count = 3;
    poly.vertex_indices[0] = 0;
    poly.vertex_indices[1] = 1;
    poly.vertex_indices[2] = 2;
    poly.color = 0x1F;

    X3gObject obj;
    memset(&obj, 0, sizeof(obj));
    obj.vertices = verts;
    obj.vertex_count = 3;
    obj.parsed_polys = &poly;
    obj.polygon_count = 1;

    lib3d_render_object(&state, &obj, 0, 0, 0, NULL, 0);
    assert(state.proj_count == 3);
    assert(state.vis_count == 1);

    uint32_t dest[320 * 200];
    memset(dest, 0, sizeof(dest));
    lib3d_present(&state, dest, 320, 200, 32, 20);

    /* Center of viewport should have the triangle */
    int cx = 32 + LIB3D_VP_WIDTH / 2;
    int cy = 20 + LIB3D_VP_HEIGHT / 2;
    assert(dest[cy * 320 + cx] != 0xFF000000);
}

static void test_z_sorting(void) {
    Lib3dState state;
    lib3d_init(&state);
    lib3d_set_camera(&state, 0, 0, 0, 0);
    lib3d_clear(&state, 0xFF000000, 0xFF000000);

    /* Far quad (z=200) */
    X3gVertex far_verts[4] = {
        { -50, -50, 200, 0, {0,0,0,0} },
        {  50, -50, 200, 0, {0,0,0,0} },
        {  50,  50, 200, 0, {0,0,0,0} },
        { -50,  50, 200, 0, {0,0,0,0} },
    };
    X3gPolygon far_poly;
    memset(&far_poly, 0, sizeof(far_poly));
    far_poly.vertex_count = 4;
    for (int i = 0; i < 4; i++) far_poly.vertex_indices[i] = i;
    far_poly.color = 0x10;

    X3gObject far_obj;
    memset(&far_obj, 0, sizeof(far_obj));
    far_obj.vertices = far_verts;
    far_obj.vertex_count = 4;
    far_obj.parsed_polys = &far_poly;
    far_obj.polygon_count = 1;

    /* Near quad (z=50) */
    X3gVertex near_verts[4] = {
        { -10, -10, 50, 0, {0,0,0,0} },
        {  10, -10, 50, 0, {0,0,0,0} },
        {  10,  10, 50, 0, {0,0,0,0} },
        { -10,  10, 50, 0, {0,0,0,0} },
    };
    X3gPolygon near_poly;
    memset(&near_poly, 0, sizeof(near_poly));
    near_poly.vertex_count = 4;
    for (int i = 0; i < 4; i++) near_poly.vertex_indices[i] = i;
    near_poly.color = 0x3F;

    X3gObject near_obj;
    memset(&near_obj, 0, sizeof(near_obj));
    near_obj.vertices = near_verts;
    near_obj.vertex_count = 4;
    near_obj.parsed_polys = &near_poly;
    near_obj.polygon_count = 1;

    lib3d_render_object(&state, &far_obj, 0, 0, 0, NULL, 0);
    lib3d_render_object(&state, &near_obj, 0, 0, 0, NULL, 0);

    uint32_t dest[320 * 200];
    memset(dest, 0, sizeof(dest));
    lib3d_present(&state, dest, 320, 200, 0, 0);

    /* Center pixel should be the near (brighter) polygon */
    int cx = LIB3D_VP_WIDTH / 2;
    int cy = LIB3D_VP_HEIGHT / 2;
    uint32_t center = dest[cy * 320 + cx];
    assert(center != 0xFF000000);
}

static void test_camera_rotation(void) {
    Lib3dState state;
    lib3d_init(&state);

    /* Object directly ahead */
    lib3d_set_camera(&state, 0, 0, 0, 0);
    lib3d_clear(&state, 0xFF000000, 0xFF000000);

    X3gVertex verts[3] = {
        { 0, -10, 50, 0, {0,0,0,0} },
        { 10, 10, 50, 0, {0,0,0,0} },
        { -10, 10, 50, 0, {0,0,0,0} },
    };
    X3gPolygon poly;
    memset(&poly, 0, sizeof(poly));
    poly.vertex_count = 3;
    poly.vertex_indices[0] = 0;
    poly.vertex_indices[1] = 1;
    poly.vertex_indices[2] = 2;
    poly.color = 0x20;

    X3gObject obj;
    memset(&obj, 0, sizeof(obj));
    obj.vertices = verts;
    obj.vertex_count = 3;
    obj.parsed_polys = &poly;
    obj.polygon_count = 1;

    lib3d_render_object(&state, &obj, 0, 0, 0, NULL, 0);
    assert(state.vis_count == 1);

    /* Turn 180 degrees — object should be behind us */
    lib3d_set_camera(&state, 0, 0, 0, (float)M_PI);
    lib3d_clear(&state, 0xFF000000, 0xFF000000);
    lib3d_render_object(&state, &obj, 0, 0, 0, NULL, 0);
    assert(state.vis_count == 0);
}

static void test_behind_camera(void) {
    Lib3dState state;
    lib3d_init(&state);
    lib3d_set_camera(&state, 0, 0, 100, 0);
    lib3d_clear(&state, 0xFF000000, 0xFF000000);

    X3gVertex verts[3] = {
        { 0, 0, 50, 0, {0,0,0,0} },
        { 10, 10, 50, 0, {0,0,0,0} },
        { -10, 10, 50, 0, {0,0,0,0} },
    };
    X3gPolygon poly;
    memset(&poly, 0, sizeof(poly));
    poly.vertex_count = 3;
    poly.vertex_indices[0] = 0;
    poly.vertex_indices[1] = 1;
    poly.vertex_indices[2] = 2;
    poly.color = 0x10;

    X3gObject obj;
    memset(&obj, 0, sizeof(obj));
    obj.vertices = verts;
    obj.vertex_count = 3;
    obj.parsed_polys = &poly;
    obj.polygon_count = 1;

    lib3d_render_object(&state, &obj, 0, 0, 0, NULL, 0);
    assert(state.vis_count == 0);
}

static void test_textured_quad(void) {
    Lib3dState state;
    lib3d_init(&state);
    lib3d_set_camera(&state, 0, 0, 0, 0);
    lib3d_clear(&state, 0xFF000000, 0xFF000000);

    Lib3dTexture tex;
    tex.width = 4;
    tex.height = 4;
    for (int i = 0; i < 16; i++)
        tex.pixels[i] = 0xFFFF0000 + (i * 16);

    lib3d_render_textured_quad(&state,
        -40, -40, 80,
         40, -40, 80,
         40,  40, 80,
        -40,  40, 80,
        &tex);

    int cx = LIB3D_VP_WIDTH / 2;
    int cy = LIB3D_VP_HEIGHT / 2;
    uint32_t center = state.framebuffer[cy * LIB3D_VP_WIDTH + cx];
    assert(center != 0xFF000000);
    assert((center >> 24) == 0xFF);
}

static void test_textured_quad_behind(void) {
    Lib3dState state;
    lib3d_init(&state);
    lib3d_set_camera(&state, 0, 0, 100, 0);
    lib3d_clear(&state, 0xFF000000, 0xFF000000);

    Lib3dTexture tex;
    tex.width = 2;
    tex.height = 2;
    for (int i = 0; i < 4; i++) tex.pixels[i] = 0xFF00FF00;

    lib3d_render_textured_quad(&state,
        -10, -10, 50,
         10, -10, 50,
         10,  10, 50,
        -10,  10, 50,
        &tex);

    int cx = LIB3D_VP_WIDTH / 2;
    int cy = LIB3D_VP_HEIGHT / 2;
    assert(state.framebuffer[cy * LIB3D_VP_WIDTH + cx] == 0xFF000000);
}

int main(void) {
    test_init();
    test_clear();
    test_render_triangle();
    test_z_sorting();
    test_camera_rotation();
    test_behind_camera();
    test_textured_quad();
    test_textured_quad_behind();
    printf("All liberation_viewport_3d tests passed\n");
    return 0;
}
