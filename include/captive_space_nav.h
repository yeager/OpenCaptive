#ifndef CAPTIVE_SPACE_NAV_H
#define CAPTIVE_SPACE_NAV_H

#include "game_state.h"
#include "holamap.h"
#include <stdint.h>

#define SPACE_FUEL_MAX      1000.0f
#define SPACE_THRUST        0.04f
#define SPACE_DRAG          0.985f
#define SPACE_ARRIVAL_DIST  8.0f

#define ORBIT_SPEED         0.02f
#define LANDING_TICKS       90

#define STARFIELD_COUNT     128
#define PLANET_RADIUS       24

typedef struct {
    float x, y;
    float brightness;
    float depth;
} Star;

typedef struct {
    Star stars[STARFIELD_COUNT];
    uint32_t seed;
} Starfield;

void starfield_init(Starfield *sf, uint32_t seed);

void space_flight_init(GameState *gs);

void space_flight_update(GameState *gs);

void space_flight_render(const GameState *gs, const Starfield *sf,
                         uint32_t *fb, int fb_w, int fb_h);

void orbit_render(const GameState *gs, const Holamap *hm,
                  uint32_t *fb, int fb_w, int fb_h);

void landing_render(const GameState *gs,
                    uint32_t *fb, int fb_w, int fb_h);

#endif
