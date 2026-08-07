#include "captive_space_nav.h"
#include "captive_data.h"
#include "i18n.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

void starfield_init(Starfield *sf, uint32_t seed) {
    if (!sf) return;
    sf->seed = seed;
    uint32_t s = seed;
    for (int i = 0; i < STARFIELD_COUNT; i++) {
        sf->stars[i].x = (float)(captive_prng(&s) % 320);
        sf->stars[i].y = (float)(captive_prng(&s) % 200);
        sf->stars[i].brightness = 0.3f + 0.7f * (float)(captive_prng(&s) % 100) / 100.0f;
        sf->stars[i].depth = 0.2f + 0.8f * (float)(captive_prng(&s) % 100) / 100.0f;
    }
}

void space_flight_init(GameState *gs) {
    if (!gs) return;
    gs->space_x = 0.0f;
    gs->space_y = 0.0f;
    gs->space_vx = 0.0f;
    gs->space_vy = 0.0f;
    gs->space_fuel = SPACE_FUEL_MAX;
    uint32_t s = gs->mission_seed + (uint32_t)gs->mission * 7;
    gs->space_target_x = 40.0f + (float)(captive_prng(&s) % 200);
    gs->space_target_y = 30.0f + (float)(captive_prng(&s) % 120);
    gs->orbit_angle = 0.0f;
    gs->landing_tick = 0;
}

void space_flight_update(GameState *gs) {
    if (!gs) return;
    gs->space_x += gs->space_vx;
    gs->space_y += gs->space_vy;
    gs->space_vx *= SPACE_DRAG;
    gs->space_vy *= SPACE_DRAG;
}

static void put_px(uint32_t *fb, int w, int h, int x, int y, uint32_t c) {
    if (x >= 0 && x < w && y >= 0 && y < h)
        fb[y * w + x] = c;
}

static void fill_rect(uint32_t *fb, int w, int h,
                       int rx, int ry, int rw, int rh, uint32_t c) {
    for (int y = ry; y < ry + rh; y++)
        for (int x = rx; x < rx + rw; x++)
            put_px(fb, w, h, x, y, c);
}

static void draw_text_at(uint32_t *fb, int w, int h,
                          int x, int y, const char *text, uint32_t color);

static void draw_circle(uint32_t *fb, int w, int h,
                         int cx, int cy, int r, uint32_t color) {
    for (int dy = -r; dy <= r; dy++) {
        for (int dx = -r; dx <= r; dx++) {
            if (dx * dx + dy * dy <= r * r)
                put_px(fb, w, h, cx + dx, cy + dy, color);
        }
    }
}

static void draw_circle_outline(uint32_t *fb, int w, int h,
                                 int cx, int cy, int r, uint32_t color) {
    for (int dy = -r; dy <= r; dy++) {
        for (int dx = -r; dx <= r; dx++) {
            int d2 = dx * dx + dy * dy;
            if (d2 >= (r - 1) * (r - 1) && d2 <= r * r)
                put_px(fb, w, h, cx + dx, cy + dy, color);
        }
    }
}

/* Minimal 5x7 single-character draw using the same glyph table as main.c.
 * Only uppercase, digits, and a few punctuation marks are supported. */
static const uint8_t nav_font[][5] = {
    ['0'] = {0x3E,0x51,0x49,0x45,0x3E}, ['1'] = {0x00,0x42,0x7F,0x40,0x00},
    ['2'] = {0x42,0x61,0x51,0x49,0x46}, ['3'] = {0x21,0x41,0x45,0x4B,0x31},
    ['4'] = {0x18,0x14,0x12,0x7F,0x10}, ['5'] = {0x27,0x45,0x45,0x45,0x39},
    ['6'] = {0x3C,0x4A,0x49,0x49,0x30}, ['7'] = {0x01,0x71,0x09,0x05,0x03},
    ['8'] = {0x36,0x49,0x49,0x49,0x36}, ['9'] = {0x06,0x49,0x49,0x29,0x1E},
    ['A'] = {0x7E,0x11,0x11,0x11,0x7E}, ['B'] = {0x7F,0x49,0x49,0x49,0x36},
    ['C'] = {0x3E,0x41,0x41,0x41,0x22}, ['D'] = {0x7F,0x41,0x41,0x22,0x1C},
    ['E'] = {0x7F,0x49,0x49,0x49,0x41}, ['F'] = {0x7F,0x09,0x09,0x09,0x01},
    ['G'] = {0x3E,0x41,0x49,0x49,0x7A}, ['H'] = {0x7F,0x08,0x08,0x08,0x7F},
    ['I'] = {0x00,0x41,0x7F,0x41,0x00}, ['K'] = {0x7F,0x08,0x14,0x22,0x41},
    ['L'] = {0x7F,0x40,0x40,0x40,0x40}, ['M'] = {0x7F,0x02,0x0C,0x02,0x7F},
    ['N'] = {0x7F,0x04,0x08,0x10,0x7F}, ['O'] = {0x3E,0x41,0x41,0x41,0x3E},
    ['P'] = {0x7F,0x09,0x09,0x09,0x06}, ['R'] = {0x7F,0x09,0x19,0x29,0x46},
    ['S'] = {0x46,0x49,0x49,0x49,0x31}, ['T'] = {0x01,0x01,0x7F,0x01,0x01},
    ['U'] = {0x3F,0x40,0x40,0x40,0x3F}, ['V'] = {0x1F,0x20,0x40,0x20,0x1F},
    ['W'] = {0x3F,0x40,0x38,0x40,0x3F}, ['X'] = {0x63,0x14,0x08,0x14,0x63},
    ['Y'] = {0x07,0x08,0x70,0x08,0x07}, ['Z'] = {0x61,0x51,0x49,0x45,0x43},
    [' '] = {0x00,0x00,0x00,0x00,0x00}, [':'] = {0x00,0x36,0x36,0x00,0x00},
    ['.'] = {0x00,0x60,0x60,0x00,0x00}, ['%'] = {0x23,0x13,0x08,0x64,0x62},
    ['-'] = {0x08,0x08,0x08,0x08,0x08}, ['/'] = {0x20,0x10,0x08,0x04,0x02},
    ['+'] = {0x08,0x08,0x3E,0x08,0x08},
    ['J'] = {0x20,0x40,0x41,0x3F,0x01}, ['Q'] = {0x3E,0x41,0x51,0x21,0x5E},
};

static void draw_text_at(uint32_t *fb, int w, int h,
                          int x, int y, const char *text, uint32_t color) {
    if (!fb || !text) return;
    for (int i = 0; text[i]; i++) {
        unsigned char ch = (unsigned char)text[i];
        if (ch >= 'a' && ch <= 'z') ch = ch - 'a' + 'A';
        if (ch >= sizeof(nav_font) / sizeof(nav_font[0])) continue;
        const uint8_t *glyph = nav_font[ch];
        if (!glyph[0] && !glyph[1] && !glyph[2] && !glyph[3] && !glyph[4]
            && ch != ' ')
            continue;
        for (int col = 0; col < 5; col++) {
            uint8_t bits = glyph[col];
            for (int row = 0; row < 7; row++) {
                if (bits & (1 << row))
                    put_px(fb, w, h, x + i * 6 + col, y + row, color);
            }
        }
    }
}

static void draw_centered_text(uint32_t *fb, int w, int h,
                                int y, const char *text, uint32_t color) {
    int len = 0;
    for (const char *p = text; *p; p++) len++;
    draw_text_at(fb, w, h, (w - len * 6) / 2, y, text, color);
}

/* Cockpit frame: instrument panel border and HUD elements */
static void draw_cockpit(uint32_t *fb, int w, int h, const GameState *gs) {
    /* Side panels */
    fill_rect(fb, w, h, 0, 0, 24, h, 0xFF1A1A2E);
    fill_rect(fb, w, h, w - 24, 0, 24, h, 0xFF1A1A2E);

    /* Top instrument bar */
    fill_rect(fb, w, h, 0, 0, w, 16, 0xFF111128);
    for (int x = 0; x < w; x++)
        put_px(fb, w, h, x, 16, 0xFF3333AA);

    /* Bottom console */
    fill_rect(fb, w, h, 0, h - 30, w, 30, 0xFF111128);
    for (int x = 0; x < w; x++)
        put_px(fb, w, h, x, h - 31, 0xFF3333AA);

    /* Panel edge lines */
    for (int y = 16; y < h - 30; y++) {
        put_px(fb, w, h, 23, y, 0xFF3333AA);
        put_px(fb, w, h, w - 24, y, 0xFF3333AA);
    }

    /* Fuel gauge */
    draw_text_at(fb, w, h, 2, 2, "FUEL", 0xFF44FF44);
    int fuel_w = 50;
    int fuel_fill = (int)(gs->space_fuel / SPACE_FUEL_MAX * fuel_w);
    if (fuel_fill < 0) fuel_fill = 0;
    if (fuel_fill > fuel_w) fuel_fill = fuel_w;
    fill_rect(fb, w, h, 30, 3, fuel_w, 5, 0xFF222222);
    uint32_t fuel_color = fuel_fill > fuel_w / 4 ? 0xFF44FF44 : 0xFFFF4444;
    fill_rect(fb, w, h, 30, 3, fuel_fill, 5, fuel_color);

    /* Velocity indicator */
    float speed = sqrtf(gs->space_vx * gs->space_vx + gs->space_vy * gs->space_vy);
    char spd[16];
    snprintf(spd, sizeof(spd), "SPD:%d", (int)(speed * 100));
    draw_text_at(fb, w, h, 100, 2, spd, 0xFF44CCFF);

    /* Distance to target */
    float dx = gs->space_target_x - gs->space_x;
    float dy = gs->space_target_y - gs->space_y;
    float dist = sqrtf(dx * dx + dy * dy);
    char dst[24];
    snprintf(dst, sizeof(dst), "DIST:%d", (int)dist);
    draw_text_at(fb, w, h, 180, 2, dst, 0xFFFFCC44);

    /* Navigation hint */
    char dir[32];
    if (dist > SPACE_ARRIVAL_DIST) {
        float angle = atan2f(dy, dx) * 180.0f / 3.14159f;
        snprintf(dir, sizeof(dir), "HDG:%d", ((int)angle + 360) % 360);
    } else {
        snprintf(dir, sizeof(dir), "ARRIVING");
    }
    draw_text_at(fb, w, h, 260, 2, dir, 0xFFFFCC44);

    /* Bottom controls */
    draw_text_at(fb, w, h, 30, h - 20,
                 "ARROWS:THRUST  SPACE:BOOST", 0xFF666688);
    draw_text_at(fb, w, h, 30, h - 10,
                 "ESC:ABORT", 0xFF666688);
}

void space_flight_render(const GameState *gs, const Starfield *sf,
                         uint32_t *fb, int fb_w, int fb_h) {
    if (!gs || !sf || !fb || fb_w <= 0 || fb_h <= 0) return;

    memset(fb, 0, (size_t)fb_w * (size_t)fb_h * sizeof(uint32_t));

    /* Starfield with parallax from velocity */
    for (int i = 0; i < STARFIELD_COUNT; i++) {
        float sx = sf->stars[i].x - gs->space_vx * sf->stars[i].depth * 30.0f;
        float sy = sf->stars[i].y - gs->space_vy * sf->stars[i].depth * 30.0f;
        /* Wrap around viewport (inside cockpit window) */
        int vx0 = 24, vy0 = 17, vw = fb_w - 48, vh = fb_h - 47;
        sx = sx - vx0;
        sy = sy - vy0;
        if (vw > 0) {
            sx = fmodf(sx, (float)vw);
            if (sx < 0) sx += vw;
        }
        if (vh > 0) {
            sy = fmodf(sy, (float)vh);
            if (sy < 0) sy += vh;
        }
        int px = vx0 + (int)sx;
        int py = vy0 + (int)sy;
        uint8_t bright = (uint8_t)(sf->stars[i].brightness * 255);
        uint32_t sc = 0xFF000000 | ((uint32_t)bright << 16) |
                      ((uint32_t)bright << 8) | bright;
        put_px(fb, fb_w, fb_h, px, py, sc);
        if (sf->stars[i].brightness > 0.8f)
            put_px(fb, fb_w, fb_h, px + 1, py, sc);
    }

    /* Planet in the distance when close to target */
    float dx = gs->space_target_x - gs->space_x;
    float dy = gs->space_target_y - gs->space_y;
    float dist = sqrtf(dx * dx + dy * dy);
    if (dist < 120.0f) {
        float t = 1.0f - dist / 120.0f;
        int r = (int)(3 + t * PLANET_RADIUS);
        int pcx = fb_w / 2 + (int)(dx * 2);
        int pcy = fb_h / 2 + (int)(dy * 2);
        if (pcx < 24) pcx = 24;
        if (pcx > fb_w - 24) pcx = fb_w - 24;
        if (pcy < 17) pcy = 17;
        if (pcy > fb_h - 31) pcy = fb_h - 31;
        /* Planet body with terrain-like shading */
        for (int pyy = -r; pyy <= r; pyy++) {
            for (int pxx = -r; pxx <= r; pxx++) {
                if (pxx * pxx + pyy * pyy > r * r) continue;
                float nx = (float)pxx / r;
                float shade = 0.5f + 0.5f * (1.0f - nx);
                uint8_t g = (uint8_t)(100 * shade);
                uint8_t b = (uint8_t)(60 * shade);
                uint32_t pc = 0xFF000000 | ((uint32_t)(30) << 16) |
                              ((uint32_t)g << 8) | b;
                put_px(fb, fb_w, fb_h, pcx + pxx, pcy + pyy, pc);
            }
        }
    }

    draw_cockpit(fb, fb_w, fb_h, gs);
}

void orbit_render(const GameState *gs, const Holamap *hm,
                  uint32_t *fb, int fb_w, int fb_h) {
    if (!gs || !fb || fb_w <= 0 || fb_h <= 0) return;

    memset(fb, 0, (size_t)fb_w * (size_t)fb_h * sizeof(uint32_t));

    /* Sparse background stars */
    uint32_t s = gs->mission_seed;
    for (int i = 0; i < 60; i++) {
        int sx = captive_prng(&s) % fb_w;
        int sy = captive_prng(&s) % fb_h;
        uint8_t bright = 100 + (captive_prng(&s) % 155);
        put_px(fb, fb_w, fb_h, sx, sy,
               0xFF000000 | ((uint32_t)bright << 16) |
               ((uint32_t)bright << 8) | bright);
    }

    /* Planet (large, centered) */
    int pr = 50;
    int pcx = fb_w / 2, pcy = fb_h / 2;
    if (hm) {
        for (int dy = -pr; dy <= pr; dy++) {
            for (int dx = -pr; dx <= pr; dx++) {
                if (dx * dx + dy * dy > pr * pr) continue;
                float nx = (float)dx / pr;
                float ny = (float)dy / pr;
                float shade = 0.6f + 0.4f * (1.0f - nx);
                /* Sample terrain from holamap */
                int mx = (int)((nx * 0.5f + 0.5f) * HOLAMAP_WIDTH);
                int my = (int)((ny * 0.5f + 0.5f) * HOLAMAP_HEIGHT);
                if (mx < 0) mx = 0;
                if (mx >= HOLAMAP_WIDTH) mx = HOLAMAP_WIDTH - 1;
                if (my < 0) my = 0;
                if (my >= HOLAMAP_HEIGHT) my = HOLAMAP_HEIGHT - 1;
                uint8_t terrain = hm->surface[my][mx];
                uint32_t base;
                switch (terrain) {
                    case 0: base = 0xFF1A3366; break;
                    case 1: base = 0xFF2D6B30; break;
                    case 2: base = 0xFF4A8C3F; break;
                    case 3: base = 0xFF8B7355; break;
                    default: base = 0xFFAAAAAA; break;
                }
                uint8_t rr = (uint8_t)(((base >> 16) & 0xFF) * shade);
                uint8_t gg = (uint8_t)(((base >> 8) & 0xFF) * shade);
                uint8_t bb = (uint8_t)((base & 0xFF) * shade);
                put_px(fb, fb_w, fb_h, pcx + dx, pcy + dy,
                       0xFF000000 | ((uint32_t)rr << 16) |
                       ((uint32_t)gg << 8) | bb);
            }
        }
    } else {
        draw_circle(fb, fb_w, fb_h, pcx, pcy, pr, 0xFF2D6B30);
    }

    /* Atmosphere glow */
    draw_circle_outline(fb, fb_w, fb_h, pcx, pcy, pr + 1, 0xFF4488CC);
    draw_circle_outline(fb, fb_w, fb_h, pcx, pcy, pr + 2, 0xFF224466);

    /* Orbiting probe marker */
    float angle = gs->orbit_angle;
    int orbit_r = pr + 12;
    int mx = pcx + (int)(cosf(angle) * orbit_r);
    int my = pcy + (int)(sinf(angle) * orbit_r);
    put_px(fb, fb_w, fb_h, mx, my, 0xFFFFFFFF);
    put_px(fb, fb_w, fb_h, mx + 1, my, 0xFFFFFFFF);
    put_px(fb, fb_w, fb_h, mx, my + 1, 0xFFFFFFFF);
    put_px(fb, fb_w, fb_h, mx + 1, my + 1, 0xFFFFFFFF);

    /* Landing cursor on planet surface */
    float cursor_angle = angle + 3.14159f;
    int land_x = pcx + (int)(cosf(cursor_angle) * pr * 0.6f);
    int land_y = pcy + (int)(sinf(cursor_angle) * pr * 0.6f);
    for (int d = -2; d <= 2; d++) {
        if (d == 0) continue;
        put_px(fb, fb_w, fb_h, land_x + d, land_y, 0xFFFF4444);
        put_px(fb, fb_w, fb_h, land_x, land_y + d, 0xFFFF4444);
    }

    /* Planet name */
    char name[32];
    uint32_t pname_seed = gs->mission_seed + (uint32_t)gs->mission;
    captive_generate_planet_name(&pname_seed, name, sizeof(name));
    draw_centered_text(fb, fb_w, fb_h, 8, name, 0xFFCCCCFF);

    /* HUD */
    draw_centered_text(fb, fb_w, fb_h, fb_h - 20,
                       "ENTER:LAND  ARROWS:ORBIT  ESC:ABORT", 0xFF666688);
}

void landing_render(const GameState *gs,
                    uint32_t *fb, int fb_w, int fb_h) {
    if (!gs || !fb || fb_w <= 0 || fb_h <= 0) return;

    memset(fb, 0, (size_t)fb_w * (size_t)fb_h * sizeof(uint32_t));

    float progress = (float)gs->landing_tick / LANDING_TICKS;
    if (progress > 1.0f) progress = 1.0f;

    /* Planet surface grows from center */
    int horizon_y = (int)(fb_h * (0.3f + 0.5f * progress));
    for (int y = horizon_y; y < fb_h; y++) {
        float t = (float)(y - horizon_y) / (fb_h - horizon_y);
        uint8_t g = (uint8_t)(40 + 60 * t);
        uint8_t b = (uint8_t)(20 + 30 * t);
        uint32_t c = 0xFF000000 | ((uint32_t)20 << 16) |
                     ((uint32_t)g << 8) | b;
        for (int x = 0; x < fb_w; x++)
            put_px(fb, fb_w, fb_h, x, y, c);
    }

    /* Sky gradient */
    for (int y = 0; y < horizon_y; y++) {
        float t = (float)y / (horizon_y > 1 ? horizon_y : 1);
        uint8_t r = (uint8_t)(t * 20 * progress);
        uint8_t g = (uint8_t)(t * 40 * progress);
        uint8_t b = (uint8_t)(40 + t * 80 * progress);
        for (int x = 0; x < fb_w; x++)
            put_px(fb, fb_w, fb_h, x, y,
                   0xFF000000 | ((uint32_t)r << 16) |
                   ((uint32_t)g << 8) | b);
    }

    /* Stars fade out during descent */
    if (progress < 0.7f) {
        uint32_t s = gs->mission_seed + 0x1234;
        int star_count = (int)(40 * (1.0f - progress / 0.7f));
        for (int i = 0; i < star_count; i++) {
            int sx = captive_prng(&s) % fb_w;
            int sy = captive_prng(&s) % (horizon_y > 0 ? horizon_y : 1);
            uint8_t bright = (uint8_t)(200 * (1.0f - progress / 0.7f));
            put_px(fb, fb_w, fb_h, sx, sy,
                   0xFF000000 | ((uint32_t)bright << 16) |
                   ((uint32_t)bright << 8) | bright);
        }
    }

    /* Landing pad indicator */
    if (progress > 0.5f) {
        float pad_t = (progress - 0.5f) * 2.0f;
        int pad_w = (int)(10 + 40 * pad_t);
        int pad_h = (int)(2 + 6 * pad_t);
        int pad_x = fb_w / 2 - pad_w / 2;
        int pad_y = fb_h - 20 - (int)(40 * (1.0f - pad_t));
        fill_rect(fb, fb_w, fb_h, pad_x, pad_y, pad_w, pad_h, 0xFF555555);
        /* Pad markings */
        fill_rect(fb, fb_w, fb_h, pad_x + pad_w / 2 - 1, pad_y,
                  2, pad_h, 0xFFFFFF00);
    }

    /* Descent altitude */
    char alt[24];
    int altitude = (int)((1.0f - progress) * 5000);
    snprintf(alt, sizeof(alt), "ALT: %d M", altitude);
    draw_centered_text(fb, fb_w, fb_h, 8, alt, 0xFF44FF44);

    if (progress >= 1.0f)
        draw_centered_text(fb, fb_w, fb_h, fb_h / 2, "TOUCHDOWN", 0xFFFFFF44);
}
