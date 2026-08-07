#include "liberation_cutscene.h"
#include <string.h>

/* Simple PRNG */
static uint32_t cs_rng(uint32_t *state) {
    *state = *state * 1103515245u + 12345u;
    return (*state >> 16) & 0x7FFF;
}

static void cs_put_px(uint32_t *fb, int w, int h, int x, int y, uint32_t c) {
    if (x >= 0 && x < w && y >= 0 && y < h)
        fb[y * w + x] = c;
}

/* Minimal 5x7 font for cutscene text */
static const uint8_t cs_font[][5] = {
    ['0'] = {0x3E,0x51,0x49,0x45,0x3E}, ['1'] = {0x00,0x42,0x7F,0x40,0x00},
    ['2'] = {0x42,0x61,0x51,0x49,0x46}, ['3'] = {0x21,0x41,0x45,0x4B,0x31},
    ['4'] = {0x18,0x14,0x12,0x7F,0x10}, ['5'] = {0x27,0x45,0x45,0x45,0x39},
    ['6'] = {0x3C,0x4A,0x49,0x49,0x30}, ['7'] = {0x01,0x71,0x09,0x05,0x03},
    ['8'] = {0x36,0x49,0x49,0x49,0x36}, ['9'] = {0x06,0x49,0x49,0x29,0x1E},
    ['A'] = {0x7E,0x11,0x11,0x11,0x7E}, ['B'] = {0x7F,0x49,0x49,0x49,0x36},
    ['C'] = {0x3E,0x41,0x41,0x41,0x22}, ['D'] = {0x7F,0x41,0x41,0x22,0x1C},
    ['E'] = {0x7F,0x49,0x49,0x49,0x41}, ['F'] = {0x7F,0x09,0x09,0x09,0x01},
    ['G'] = {0x3E,0x41,0x49,0x49,0x7A}, ['H'] = {0x7F,0x08,0x08,0x08,0x7F},
    ['I'] = {0x00,0x41,0x7F,0x41,0x00}, ['J'] = {0x20,0x40,0x41,0x3F,0x01},
    ['K'] = {0x7F,0x08,0x14,0x22,0x41}, ['L'] = {0x7F,0x40,0x40,0x40,0x40},
    ['M'] = {0x7F,0x02,0x0C,0x02,0x7F}, ['N'] = {0x7F,0x04,0x08,0x10,0x7F},
    ['O'] = {0x3E,0x41,0x41,0x41,0x3E}, ['P'] = {0x7F,0x09,0x09,0x09,0x06},
    ['Q'] = {0x3E,0x41,0x51,0x21,0x5E}, ['R'] = {0x7F,0x09,0x19,0x29,0x46},
    ['S'] = {0x46,0x49,0x49,0x49,0x31}, ['T'] = {0x01,0x01,0x7F,0x01,0x01},
    ['U'] = {0x3F,0x40,0x40,0x40,0x3F}, ['V'] = {0x1F,0x20,0x40,0x20,0x1F},
    ['W'] = {0x3F,0x40,0x38,0x40,0x3F}, ['X'] = {0x63,0x14,0x08,0x14,0x63},
    ['Y'] = {0x07,0x08,0x70,0x08,0x07}, ['Z'] = {0x61,0x51,0x49,0x45,0x43},
    [' '] = {0x00,0x00,0x00,0x00,0x00}, [':'] = {0x00,0x36,0x36,0x00,0x00},
    ['.'] = {0x00,0x60,0x60,0x00,0x00}, ['!'] = {0x00,0x00,0x6F,0x00,0x00},
    ['-'] = {0x08,0x08,0x08,0x08,0x08}, ['/'] = {0x20,0x10,0x08,0x04,0x02},
};

static void cs_draw_text(uint32_t *fb, int w, int h,
                          int x, int y, const char *text, uint32_t color) {
    if (!fb || !text) return;
    for (int i = 0; text[i]; i++) {
        unsigned char ch = (unsigned char)text[i];
        if (ch >= 'a' && ch <= 'z') ch = ch - 'a' + 'A';
        if (ch >= sizeof(cs_font) / sizeof(cs_font[0])) continue;
        const uint8_t *glyph = cs_font[ch];
        for (int col = 0; col < 5; col++) {
            uint8_t bits = glyph[col];
            for (int row = 0; row < 7; row++) {
                if (bits & (1 << row))
                    cs_put_px(fb, w, h, x + i * 6 + col, y + row, color);
            }
        }
    }
}

static void cs_draw_centered(uint32_t *fb, int w, int h,
                              int y, const char *text, uint32_t color) {
    int len = 0;
    for (const char *p = text; *p; p++) len++;
    cs_draw_text(fb, w, h, (w - len * 6) / 2, y, text, color);
}

/* Draw starfield background */
static void cs_draw_stars(uint32_t *fb, int w, int h, uint32_t seed, int scroll) {
    uint32_t rng = seed;
    for (int i = 0; i < 80; i++) {
        int sx = (int)(cs_rng(&rng) % (uint32_t)w);
        int sy = (int)(cs_rng(&rng) % (uint32_t)h);
        sy = (sy + scroll) % h;
        uint32_t bright = 0xFF404040 + (cs_rng(&rng) % 0xC0) * 0x010101;
        cs_put_px(fb, w, h, sx, sy, bright);
    }
}

/* Mission text templates */
static const char *mission_intros[] = {
    "MISSION BRIEFING",
    "NEW ORDERS RECEIVED",
    "COMMAND DISPATCH",
    "PRIORITY ALERT",
    "TACTICAL UPDATE",
};

static const char *mission_objectives[] = {
    "LOCATE THE MAINFRAME",
    "DESTROY THE GENERATOR",
    "RESCUE THE PRISONERS",
    "DISABLE THE DEFENSES",
    "RETRIEVE THE DATA CORE",
    "NEUTRALIZE THE THREAT",
    "SECURE THE PERIMETER",
    "EXTRACT THE AGENT",
};

static const char *mission_locations[] = {
    "IN THE UNDERGROUND COMPLEX",
    "AT THE ORBITAL STATION",
    "WITHIN THE CITY RUINS",
    "DEEP IN THE WASTELAND",
    "ON THE FACTORY LEVEL",
};

void cutscene_init(CutsceneState *cs, int mission, uint32_t seed) {
    if (!cs) return;
    memset(cs, 0, sizeof(*cs));
    cs->mission = mission;
    cs->seed = seed;
    cs->total_frames = 300;
    cs->done = false;
}

void cutscene_tick(CutsceneState *cs) {
    if (!cs || cs->done) return;
    cs->frame++;
    if (cs->frame >= cs->total_frames) {
        cs->done = true;
    }
}

void cutscene_render(const CutsceneState *cs, uint32_t *fb, int w, int h) {
    if (!cs || !fb) return;

    /* Clear to black */
    memset(fb, 0, (size_t)(w * h) * sizeof(uint32_t));

    /* Starfield */
    cs_draw_stars(fb, w, h, cs->seed, cs->frame);

    /* Scrolling text - 5 lines */
    int m = cs->mission;
    int scroll_y = h - cs->frame;

    const char *line0 = mission_intros[m % 5];
    const char *line1 = "";
    const char *line2 = mission_objectives[m % 8];
    const char *line3 = mission_locations[m % 5];
    const char *line4 = "GOOD LUCK TEAM!";

    /* Use different color for header */
    char mission_num[32];
    {
        int idx = 0;
        const char *prefix = "MISSION ";
        for (int i = 0; prefix[i]; i++) mission_num[idx++] = prefix[i];
        mission_num[idx++] = '0' + (char)((m + 1) / 10);
        mission_num[idx++] = '0' + (char)((m + 1) % 10);
        mission_num[idx] = '\0';
    }

    cs_draw_centered(fb, w, h, scroll_y + 20, mission_num, 0xFFFFFF00);
    cs_draw_centered(fb, w, h, scroll_y + 40, line0, 0xFFFF8800);
    cs_draw_centered(fb, w, h, scroll_y + 60, line1, 0xFFCCCCCC);
    cs_draw_centered(fb, w, h, scroll_y + 80, line2, 0xFFCCCCCC);
    cs_draw_centered(fb, w, h, scroll_y + 100, line3, 0xFFCCCCCC);
    cs_draw_centered(fb, w, h, scroll_y + 120, line4, 0xFF44FF44);
}

/* --- End game cinematic --- */

void endgame_init(CutsceneState *cs) {
    if (!cs) return;
    memset(cs, 0, sizeof(*cs));
    cs->mission = -1;
    cs->seed = 42;
    cs->total_frames = 600;
    cs->done = false;
}

static const char *credits[] = {
    "LIBERATION",
    "",
    "CONGRATULATIONS!",
    "YOU HAVE FREED THE GALAXY",
    "",
    "YOUR RANK:",
    "",  /* placeholder for rank */
    "",
    "CREDITS",
    "",
    "ORIGINAL GAME BY",
    "TONY CROWTHER",
    "",
    "REIMPLEMENTED BY",
    "THE OPENCAPTIVE TEAM",
    "",
    "THANK YOU FOR PLAYING!",
};
#define NUM_CREDITS (int)(sizeof(credits) / sizeof(credits[0]))

void endgame_render(const CutsceneState *cs, uint32_t *fb, int w, int h) {
    if (!cs || !fb) return;

    memset(fb, 0, (size_t)(w * h) * sizeof(uint32_t));

    /* Starfield with slow scroll */
    cs_draw_stars(fb, w, h, cs->seed ^ 0xDEAD, cs->frame / 2);

    /* Determine rank from score (encoded in seed for simplicity) */
    const char *rank;
    uint32_t score = cs->seed;
    if (score >= 10000) rank = "LEGENDARY";
    else if (score >= 5000) rank = "ELITE";
    else if (score >= 2000) rank = "VETERAN";
    else rank = "ROOKIE";

    /* Scrolling credits */
    int scroll_y = h - cs->frame;

    for (int i = 0; i < NUM_CREDITS; i++) {
        int ly = scroll_y + i * 16;
        const char *line = credits[i];
        uint32_t color = 0xFFCCCCCC;

        /* Title in gold */
        if (i == 0) color = 0xFFFFCC00;
        /* Congratulations in green */
        else if (i == 2) color = 0xFF44FF44;
        /* Rank placeholder */
        else if (i == 6) {
            line = rank;
            color = 0xFFFF4444;
        }

        cs_draw_centered(fb, w, h, ly, line, color);
    }

    /* Score display at fixed position */
    if (cs->frame > 60) {
        char score_text[32] = "SCORE: ";
        int idx = 7;
        uint32_t s = score;
        /* Simple number to string */
        char digits[12];
        int nd = 0;
        if (s == 0) digits[nd++] = '0';
        while (s > 0) { digits[nd++] = '0' + (char)(s % 10); s /= 10; }
        for (int d = nd - 1; d >= 0; d--) score_text[idx++] = digits[d];
        score_text[idx] = '\0';
        cs_draw_centered(fb, w, h, 10, score_text, 0xFFFFFFFF);
    }
}
