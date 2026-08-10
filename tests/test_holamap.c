#include "holamap.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_init(void) {
    Holamap hm;
    holamap_init(&hm, 12345);
    /* No planet/base data is invented until the original source table is
     * decoded from Captive media. */
    assert(hm.num_bases == 0);
    assert(hm.cursor_x == HOLAMAP_WIDTH / 2);
    assert(hm.cursor_y == HOLAMAP_HEIGHT / 2);
    assert(hm.zoom_level == 1);
}

static void test_zoom_is_clamped(void) {
    Holamap hm;
    holamap_init(&hm, 1);
    holamap_zoom_out(&hm);
    assert(hm.zoom_level == 0);
    for (int i = 0; i < 20; i++) holamap_zoom_in(&hm);
    assert(hm.zoom_level == 2);
    for (int i = 0; i < 20; i++) holamap_zoom_out(&hm);
    assert(hm.zoom_level == 0);
}

static void test_deterministic(void) {
    Holamap a, b;
    holamap_init(&a, 42);
    holamap_init(&b, 42);
    assert(a.num_bases == b.num_bases);
    assert(memcmp(a.surface, b.surface, sizeof(a.surface)) == 0);
    for (int i = 0; i < a.num_bases; i++) {
        assert(a.bases[i].x == b.bases[i].x);
        assert(a.bases[i].y == b.bases[i].y);
    }
}

static void test_invalid_base_count_does_not_escape_array(void) {
    Holamap hm = {0};
    uint32_t pixel = 0;
    hm.num_bases = 1000;
    holamap_render(&hm, &pixel, 1, 1);
    holamap_reveal_base(&hm, HOLAMAP_MAX_BASES);
    holamap_render(NULL, &pixel, 1, 1);
    holamap_render(&hm, NULL, 1, 1);
}

static void test_reveal(void) {
    Holamap hm;
    holamap_init(&hm, 99);
    assert(hm.num_bases == 0);
    holamap_reveal_base(&hm, 0);
    holamap_reveal_base(&hm, -1);
    holamap_reveal_base(&hm, 999);
}

static void test_cursor_movement_is_clamped(void) {
    Holamap hm;
    holamap_init(&hm, 99);
    holamap_move_cursor(&hm, -1000, -1000);
    assert(hm.cursor_x == 0);
    assert(hm.cursor_y == 0);
    holamap_move_cursor(&hm, 1000, 1000);
    assert(hm.cursor_x == HOLAMAP_WIDTH - 1);
    assert(hm.cursor_y == HOLAMAP_HEIGHT - 1);
}

static void test_pyramid_centers_cursor(void) {
    Holamap hm;
    holamap_init(&hm, 99);
    holamap_move_cursor(&hm, -1000, -1000);
    holamap_center_cursor(&hm);
    assert(hm.cursor_x == HOLAMAP_WIDTH / 2);
    assert(hm.cursor_y == HOLAMAP_HEIGHT / 2);
}

static void test_render(void) {
    Holamap hm;
    holamap_init(&hm, 55);
    holamap_reveal_base(&hm, 0);
    uint32_t fb[320 * 200];
    memset(fb, 0, sizeof(fb));
    holamap_render(&hm, fb, 320, 200);
    int nonzero = 0;
    for (int i = 0; i < 320 * 200; i++)
        if (fb[i] != 0) nonzero++;
    assert(nonzero == 0);
}

static void test_reference_frame(void) {
    Holamap hm = {0};
    uint8_t rgba[8] = { 0x12, 0x34, 0x56, 0xFF,
                        0xA0, 0xB0, 0xC0, 0xFF };
    uint32_t fb[2] = {0, 0};
    holamap_set_reference_frame(&hm, rgba, 2, 1);
    holamap_render(&hm, fb, 2, 1);
    assert(fb[0] == 0xFF123456U);
    assert(fb[1] == 0xFFA0B0C0U);
}

static void test_zoom_reference_frames_are_selected_without_resampling(void) {
    Holamap hm;
    uint8_t normal[4] = {0x10, 0x20, 0x30, 0xFF};
    uint8_t zoom_out[4] = {0x40, 0x50, 0x60, 0xFF};
    uint8_t zoom_in[4] = {0x70, 0x80, 0x90, 0xFF};
    uint32_t pixel = 0;
    holamap_init(&hm, 1);
    holamap_set_reference_frame(&hm, normal, 1, 1);
    holamap_set_zoom_reference_frames(&hm, zoom_out, 1, 1,
                                      zoom_in, 1, 1);
    holamap_render(&hm, &pixel, 1, 1);
    assert(pixel == 0xFF102030U);
    holamap_zoom_in(&hm);
    holamap_render(&hm, &pixel, 1, 1);
    assert(pixel == 0xFF708090U);
    holamap_zoom_out(&hm);
    holamap_zoom_out(&hm);
    holamap_render(&hm, &pixel, 1, 1);
    assert(pixel == 0xFF405060U);
}

static void test_standalone_reference_frame(void) {
    uint8_t rgba[4] = { 0x12, 0x34, 0x56, 0xFF };
    uint32_t fb = 0;
    holamap_render_reference_frame(rgba, 1, 1, &fb, 1, 1);
    assert(fb == 0xFF123456U);
}

static void test_surface_variety(void) {
    Holamap hm;
    holamap_init(&hm, 777);
    int nonzero = 0;
    for (int y = 0; y < HOLAMAP_HEIGHT; y++)
        for (int x = 0; x < HOLAMAP_WIDTH; x++)
            nonzero += hm.surface[y][x] != 0;
    assert(nonzero == 0);
}

static void test_base_coordinates_seed_sweep(void) {
    for (uint32_t seed = 0; seed < 512; seed++) {
        Holamap hm;
        holamap_init(&hm, seed);
        assert(hm.num_bases == 0);
    }
}

int main(void) {
    test_init();
    test_deterministic();
    test_invalid_base_count_does_not_escape_array();
    test_reveal();
    test_cursor_movement_is_clamped();
    test_pyramid_centers_cursor();
    test_zoom_is_clamped();
    test_render();
    test_reference_frame();
    test_zoom_reference_frames_are_selected_without_resampling();
    test_standalone_reference_frame();
    test_surface_variety();
    test_base_coordinates_seed_sweep();
    printf("All holamap tests passed.\n");
    return 0;
}
