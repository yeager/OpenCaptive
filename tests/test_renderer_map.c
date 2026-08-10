/* Regression tests for the mouse window->canvas mapping.  The bug this guards
 * against: on a Retina MacBook the standalone mapping used window points while
 * the renderer scaled in output pixels and floored the scale to an integer for
 * the 320x200 game canvas, so clicks on the tiny navigation arrows never
 * landed.  renderer_map_point() is the shared inverse of the presentation
 * transform; these checks pin the cases the old math got wrong. */
#include "renderer.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

static int near(float a, float b) { return fabsf(a - b) < 0.5f; }

/* Retina, integer-scaled game canvas: 1280x800-point window, 2560x1600-pixel
 * output, 320x200 canvas == texture, integer scaling on.  A click at the canvas
 * centre (window point 640,400) must map to canvas (160,100), not drift by the
 * 2x HiDPI factor. */
static void test_retina_integer_scaled_game_canvas(void) {
    float cx = -1, cy = -1;
    bool ok = renderer_map_point(1280, 800, 2560, 1600, 320, 200, 320, 200,
                                 true, 640.0f, 400.0f, &cx, &cy);
    assert(ok);
    assert(near(cx, 160.0f) && near(cy, 100.0f));

    /* The up-arrow hit box (canvas x 218..236, y 65..83) must be reachable.
     * 1 canvas pixel == 8 output pixels == 4 window points here, so its centre
     * (227,74) sits at window point (227*4, 74*4) = (908,296). */
    ok = renderer_map_point(1280, 800, 2560, 1600, 320, 200, 320, 200,
                            true, 908.0f, 296.0f, &cx, &cy);
    assert(ok);
    assert(cx >= 218.0f && cx < 236.0f && cy >= 65.0f && cy < 83.0f);
}

/* Non-integer window: 1000x700-point window, same in pixels (no HiDPI), 320x200
 * integer-scaled canvas.  Scale floors 700/200=3.5 -> 3, so the image is
 * 960x600 letterboxed at offset (20,50).  A click in the letterbox bar must be
 * rejected, and a click at the image's top-left must map to canvas (0,0). */
static void test_letterbox_integer_scale_offset(void) {
    float cx = -1, cy = -1;
    /* Top-left of the presented image is at window (20,50). */
    bool ok = renderer_map_point(1000, 700, 1000, 700, 320, 200, 320, 200,
                                 true, 20.0f, 50.0f, &cx, &cy);
    assert(ok);
    assert(near(cx, 0.0f) && near(cy, 0.0f));

    /* A point in the top letterbox bar (above y=50) is outside the image. */
    ok = renderer_map_point(1000, 700, 1000, 700, 320, 200, 320, 200,
                            true, 500.0f, 10.0f, &cx, &cy);
    assert(!ok);
}

/* The 960x600 launcher canvas is larger than 640x400, so integer flooring must
 * NOT apply even when integer_scaling is on: it fills the window fractionally.
 * A 1280x800 window has scale 1.333; the canvas centre maps to (480,300). */
static void test_launcher_canvas_not_floored(void) {
    float cx = -1, cy = -1;
    bool ok = renderer_map_point(1280, 800, 1280, 800, 960, 600, 960, 600,
                                 true, 640.0f, 400.0f, &cx, &cy);
    assert(ok);
    assert(near(cx, 480.0f) && near(cy, 300.0f));
}

/* Widescreen: the texture is wider than the canvas, pillar-boxing the 320-wide
 * canvas inside a 384-wide texture.  A canvas hit box must still resolve to
 * native 320-space coordinates. */
static void test_widescreen_texture_maps_to_canvas(void) {
    float cx = -1, cy = -1;
    /* texture 384x200 at 4x -> 1536x800 output, centred in a 1536x800 window.
     * Canvas centre (160,100) sits at texture (160 + 32, 100) = (192,100),
     * i.e. window (192*4, 100*4) = (768,400). */
    bool ok = renderer_map_point(1536, 800, 1536, 800, 384, 200, 320, 200,
                                 false, 768.0f, 400.0f, &cx, &cy);
    assert(ok);
    assert(near(cx, 160.0f) && near(cy, 100.0f));
}

/* The actual Captive HD path uploads a 3x xBRZ framebuffer.  The navigation
 * controls remain in the original 320x200 coordinate system. */
static void test_hd_upscale_maps_to_canvas(void) {
    float cx = -1, cy = -1;
    bool ok = renderer_map_point(960, 600, 960, 600, 960, 600, 320, 200,
                                 true, 681.0f, 222.0f, &cx, &cy);
    assert(ok);
    assert(cx >= 218.0f && cx < 236.0f && cy >= 65.0f && cy < 83.0f);
}

int main(void) {
    test_retina_integer_scaled_game_canvas();
    test_letterbox_integer_scale_offset();
    test_launcher_canvas_not_floored();
    test_widescreen_texture_maps_to_canvas();
    test_hd_upscale_maps_to_canvas();
    printf("All renderer_map tests passed\n");
    return 0;
}
