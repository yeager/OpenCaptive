#include "liberation_viewport_3d.h"
#include <assert.h>
#include <float.h>
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

    /* Geometry is only filled in colours the palette can resolve. */
    static uint32_t palette[64];
    for (int i = 0; i < 64; i++) palette[i] = 0xFF000000 | (uint32_t)(i + 1);
    palette[0x1F] = 0xFF3366CC;

    lib3d_render_object(&state, &obj, 0, 0, 0, palette, 64);
    assert(state.proj_count == 3);
    assert(state.vis_count == 1);

    static uint32_t dest[320 * 200];
    memset(dest, 0, sizeof(dest));
    lib3d_present(&state, dest, 320, 200, 32, 20);

    /* The centre must carry the triangle's own palette colour.  The polygon
     * has a zero normal, so it is drawn unshaded and the value is exact.
     * Asserting "not black" passed even when nothing was drawn at all. */
    int cx = 32 + LIB3D_VP_WIDTH / 2;
    int cy = 20 + LIB3D_VP_HEIGHT / 2;
    assert(dest[cy * 320 + cx] == 0xFF3366CC);
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

    /* Give the two quads distinct palette colours so the depth order is
     * actually observable.  Both have zero normals, so they are drawn
     * unshaded and the centre pixel equals one palette entry exactly. */
    static uint32_t palette[64];
    for (int i = 0; i < 64; i++) palette[i] = 0xFF000000 | (uint32_t)(i + 1);
    palette[far_poly.color & 0x3F]  = 0xFF0000FF;   /* far  = blue  */
    palette[near_poly.color & 0x3F] = 0xFFFF0000;   /* near = red   */

    lib3d_render_object(&state, &far_obj, 0, 0, 0, palette, 64);
    lib3d_render_object(&state, &near_obj, 0, 0, 0, palette, 64);

    static uint32_t dest[320 * 200];
    memset(dest, 0, sizeof(dest));
    lib3d_present(&state, dest, 320, 200, 0, 0);

    /* The near quad must win the centre.  The old assertion only checked the
     * pixel was not black, so an inverted depth sort — or nothing drawn at
     * all — passed just as happily. */
    int cx = LIB3D_VP_WIDTH / 2;
    int cy = LIB3D_VP_HEIGHT / 2;
    uint32_t center = dest[cy * 320 + cx];
    assert(center == 0xFFFF0000);
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

static void test_nonfinite_vertex_is_rejected(void) {
    Lib3dState state;
    lib3d_init(&state);
    lib3d_set_camera(&state, 0, 0, 0, 0);

    X3gVertex verts[3] = {
        {0, 0, 50, 0, {0,0,0,0}},
        {10, 10, 50, 0, {0,0,0,0}},
        {-10, 10, 50, 0, {0,0,0,0}},
    };
    X3gPolygon poly;
    memset(&poly, 0, sizeof(poly));
    poly.vertex_count = 3;
    poly.vertex_indices[0] = 0;
    poly.vertex_indices[1] = 1;
    poly.vertex_indices[2] = 2;
    X3gObject obj = {0};
    obj.vertices = verts;
    obj.vertex_count = 3;
    obj.parsed_polys = &poly;
    obj.polygon_count = 1;

    state.fov_scale = NAN;
    lib3d_render_object(&state, &obj, 0, 0, 0, NULL, 0);
    assert(state.vis_count == 0);
}

static void test_nonfinite_textured_quad_is_rejected(void) {
    Lib3dState state;
    Lib3dTexture tex = {0};
    lib3d_init(&state);
    lib3d_clear(&state, 0, 0);
    tex.width = 1;
    tex.height = 1;
    state.fov_scale = NAN;
    lib3d_render_textured_quad(&state, 0, 0, 10, 1, 0, 10,
                               1, 1, 10, 0, 1, 10, &tex);
    assert(state.zbuffer[0] > 1e20f);
}

static void test_mixed_depth_textured_quad_has_initialized_fallbacks(void) {
    Lib3dState state;
    Lib3dTexture tex;
    lib3d_init(&state);
    lib3d_clear(&state, 0xFF000000, 0xFF000000);
    tex.width = 2;
    tex.height = 2;
    for (int i = 0; i < 4; ++i) tex.pixels[i] = 0xFFFFFFFF;

    /* One corner is behind the near plane. The rasterizer must not consume
     * uninitialized screen coordinates for this partially visible quad. */
    lib3d_render_textured_quad(&state,
        -20, -20, 2,
         20, -20, 80,
         20,  20, 80,
        -20,  20, 80,
        &tex);
    for (size_t i = 0; i < sizeof(state.zbuffer) / sizeof(state.zbuffer[0]); ++i)
        assert(isfinite(state.zbuffer[i]));
}

static void test_extreme_projection_is_rejected_before_integer_conversion(void) {
    Lib3dState state;
    lib3d_init(&state);
    lib3d_set_camera(&state, 0, 0, 0, 0);

    X3gVertex verts[3] = {
        { 100, -1, 4, 0, {0, 0, 0, 0} },
        { 0, 1, 100, 0, {0, 0, 0, 0} },
        {-1, 1, 100, 0, {0, 0, 0, 0} },
    };
    X3gPolygon poly = {0};
    poly.vertex_count = 3;
    poly.vertex_indices[0] = 0;
    poly.vertex_indices[1] = 1;
    poly.vertex_indices[2] = 2;
    X3gObject obj = {0};
    obj.vertices = verts;
    obj.vertex_count = 3;
    obj.parsed_polys = &poly;
    obj.polygon_count = 1;

    state.fov_scale = FLT_MAX;
    lib3d_render_object(&state, &obj, 0, 0, 0, NULL, 0);
    assert(state.vis_count == 0);
}

static void test_oversized_texture_is_rejected(void) {
    Lib3dState state;
    lib3d_init(&state);
    lib3d_clear(&state, 0, 0);
    Lib3dTexture tex = {0};
    tex.width = LIB3D_MAX_TEX_SIZE + 1;
    tex.height = 1;
    lib3d_render_textured_quad(&state, 0, 0, 10, 1, 0, 10,
                               1, 1, 10, 0, 1, 10, &tex);
    for (size_t i = 0; i < sizeof(state.zbuffer) / sizeof(state.zbuffer[0]); ++i)
        assert(state.zbuffer[i] == 1e30f);
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

static void test_projected_indices_above_255(void) {
    Lib3dState state;
    X3gVertex filler[256];
    X3gPolygon filler_poly;
    X3gVertex triangle_vertices[3] = {
        { -20, -20, 100, 0, {0, 0, 0, 0} },
        {  20, -20, 100, 0, {0, 0, 0, 0} },
        {   0,  20, 100, 0, {0, 0, 0, 0} }
    };
    X3gPolygon triangle_poly;
    X3gObject filler_object;
    X3gObject triangle_object;
    uint32_t dest[320 * 200];

    memset(filler, 0, sizeof(filler));
    for (size_t i = 0; i < 256; ++i) filler[i].z = 100;
    memset(&filler_poly, 0, sizeof(filler_poly));
    memset(&triangle_poly, 0, sizeof(triangle_poly));
    triangle_poly.vertex_count = 3;
    triangle_poly.vertex_indices[0] = 0;
    triangle_poly.vertex_indices[1] = 1;
    triangle_poly.vertex_indices[2] = 2;
    triangle_poly.color = 0x1F;
    memset(&filler_object, 0, sizeof(filler_object));
    filler_object.vertices = filler;
    filler_object.vertex_count = 256;
    filler_object.parsed_polys = &filler_poly;
    memset(&triangle_object, 0, sizeof(triangle_object));
    triangle_object.vertices = triangle_vertices;
    triangle_object.vertex_count = 3;
    triangle_object.parsed_polys = &triangle_poly;
    triangle_object.polygon_count = 1;

    lib3d_init(&state);
    lib3d_set_camera(&state, 0, 0, 0, 0);
    lib3d_clear(&state, 0xFF000000, 0xFF000000);
    static uint32_t palette[64];
    for (int i = 0; i < 64; i++) palette[i] = 0xFF000000 | (uint32_t)(i + 1);
    palette[0x1F] = 0xFF3366CC;

    lib3d_render_object(&state, &filler_object, 0, 0, 0, palette, 64);
    lib3d_render_object(&state, &triangle_object, 0, 0, 0, palette, 64);

    assert(state.proj_count == 259);
    assert(state.vis_count == 1);
    memset(dest, 0, sizeof(dest));
    lib3d_present(&state, dest, 320, 200, 0, 0);
    /* The triangle's vertices sit past index 255, so this pixel proves the
     * high indices resolved to the right projected vertices. */
    assert(dest[100 * 320 + 128] == 0xFF3366CC);
}

static void test_extreme_screen_coordinates_are_clipped(void) {
    Lib3dState state;
    lib3d_init(&state);
    lib3d_set_camera(&state, 0, 0, 0, 0);
    lib3d_clear(&state, 0xFF000000, 0xFF000000);

    X3gVertex vertices[3] = {
        { -32767, -32767, 10, 0, {0, 0, 0, 0} },
        {  32767, -32767, 10, 0, {0, 0, 0, 0} },
        {       0,  32767, 10, 0, {0, 0, 0, 0} },
    };
    X3gPolygon polygon = {0};
    polygon.vertex_count = 3;
    polygon.vertex_indices[0] = 0;
    polygon.vertex_indices[1] = 1;
    polygon.vertex_indices[2] = 2;
    polygon.color = 0x1F;
    X3gObject object = {0};
    object.vertices = vertices;
    object.vertex_count = 3;
    object.parsed_polys = &polygon;
    object.polygon_count = 1;

    lib3d_render_object(&state, &object, 0, 0, 0, NULL, 0);
    uint32_t dest[320 * 200];
    memset(dest, 0, sizeof(dest));
    lib3d_present(&state, dest, 320, 200, 0, 0);
    assert(state.vis_count == 1);

    Lib3dTexture texture = {0};
    texture.width = 1;
    texture.height = 1;
    texture.pixels[0] = 0xFFFFFFFF;
    lib3d_clear(&state, 0xFF000000, 0xFF000000);
    lib3d_render_textured_quad(&state,
        -32767, -32767, 10, 32767, -32767, 10,
         32767,  32767, 10, -32767,  32767, 10, &texture);
    for (size_t i = 0; i < sizeof(state.zbuffer) / sizeof(state.zbuffer[0]); i++)
        assert(isfinite(state.zbuffer[i]));
}

int main(void) {
    test_init();
    test_clear();
    test_render_triangle();
    test_z_sorting();
    test_camera_rotation();
    test_behind_camera();
    test_nonfinite_vertex_is_rejected();
    test_nonfinite_textured_quad_is_rejected();
    test_mixed_depth_textured_quad_has_initialized_fallbacks();
    test_extreme_projection_is_rejected_before_integer_conversion();
    test_oversized_texture_is_rejected();
    test_textured_quad();
    test_textured_quad_behind();
    test_projected_indices_above_255();
    test_extreme_screen_coordinates_are_clipped();
    printf("All liberation_viewport_3d tests passed\n");
    return 0;
}
