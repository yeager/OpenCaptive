#include "captive_space_nav.h"
#include <assert.h>
#include <math.h>
#include <string.h>

int main(void) {
    GameState gs;
    memset(&gs, 0, sizeof(gs));
    gs.mission = 1;
    gs.mission_seed = 42;

    space_flight_init(&gs);
    assert(gs.space_x == 0.0f);
    assert(gs.space_y == 0.0f);
    assert(gs.space_fuel == SPACE_FUEL_MAX);
    assert(gs.space_target_x > 0.0f);
    assert(gs.space_target_y > 0.0f);

    gs.space_vx = 1.0f;
    gs.space_vy = 0.5f;
    float old_x = gs.space_x;
    space_flight_update(&gs);
    assert(gs.space_x > old_x);
    assert(fabsf(gs.space_vx) < 1.0f);

    Starfield sf;
    starfield_init(&sf, 42);
    for (int i = 0; i < STARFIELD_COUNT; i++) {
        assert(sf.stars[i].x >= 0.0f && sf.stars[i].x < 320.0f);
        assert(sf.stars[i].y >= 0.0f && sf.stars[i].y < 200.0f);
        assert(sf.stars[i].brightness >= 0.3f);
        assert(sf.stars[i].depth >= 0.2f);
    }

    static uint32_t fb[320 * 200];
    memset(fb, 0, sizeof(fb));
    space_flight_render(&gs, &sf, fb, 320, 200);
    int nonblack = 0;
    for (int i = 0; i < 320 * 200; i++)
        if (fb[i] != 0 && fb[i] != 0xFF000000) nonblack++;
    assert(nonblack > 0);

    memset(fb, 0, sizeof(fb));
    orbit_render(&gs, NULL, fb, 320, 200);
    nonblack = 0;
    for (int i = 0; i < 320 * 200; i++)
        if (fb[i] != 0 && fb[i] != 0xFF000000) nonblack++;
    assert(nonblack > 0);

    gs.landing_tick = LANDING_TICKS / 2;
    memset(fb, 0, sizeof(fb));
    landing_render(&gs, fb, 320, 200);
    nonblack = 0;
    for (int i = 0; i < 320 * 200; i++)
        if (fb[i] != 0 && fb[i] != 0xFF000000) nonblack++;
    assert(nonblack > 0);

    space_flight_init(&gs);
    gs.space_x = gs.space_target_x;
    gs.space_y = gs.space_target_y;
    float dx = gs.space_target_x - gs.space_x;
    float dy = gs.space_target_y - gs.space_y;
    assert(sqrtf(dx * dx + dy * dy) < SPACE_ARRIVAL_DIST);

    return 0;
}
