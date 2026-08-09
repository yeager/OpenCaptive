#include "liberation_fnt.h"
#include "liberation_data.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Minimal FNT: CHAR header + 3 glyphs (space, !, ") */
static const uint8_t test_fnt[] = {
    'C','H','A','R',
    0x00, 0x03,             /* 3 glyphs */
    0x00, 0x01,             /* version 1 */
    0x00, 0x08,             /* max width 8 */
    0x00, 0x02,             /* 2 planes */

    /* Glyph 0 (space): width=4, blank */
    0x04, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x04, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,

    /* Glyph 1 (!): width=4, center column */
    0x04, 0x40,0x40,0x40,0x40,0x00,0x40,0x00,
    0x04, 0xA0,0xA0,0xA0,0x00,0x00,0x00,0x00,

    /* Glyph 2 ("): width=6, two dots */
    0x06, 0xD8,0x48,0x00,0x00,0x00,0x00,0x00,
    0x06, 0x00,0x90,0x00,0x00,0x00,0x00,0x00,
};

static void test_open(void) {
    FntFont font;
    assert(fnt_open(&font, test_fnt, sizeof(test_fnt)));
    assert(font.glyph_count == 3);
    assert(font.max_width == 8);
    assert(font.num_planes == 2);
}

static void test_bad_magic(void) {
    uint8_t bad[12] = {'N','O','P','E', 0,1, 0,1, 0,8, 0,2};
    FntFont font;
    assert(!fnt_open(&font, bad, sizeof(bad)));
}

static void test_get_glyph(void) {
    FntFont font;
    assert(fnt_open(&font, test_fnt, sizeof(test_fnt)));

    const FntGlyph *space = fnt_get_glyph(&font, ' ');
    assert(space != NULL);
    assert(space->width == 4);
    for (int i = 0; i < FNT_GLYPH_HEIGHT; i++)
        assert(space->plane0[i] == 0);

    const FntGlyph *bang = fnt_get_glyph(&font, '!');
    assert(bang != NULL);
    assert(bang->width == 4);
    assert(bang->plane0[0] == 0x40);
    assert(bang->plane0[5] == 0x40);
    assert(bang->plane1[0] == 0xA0);

    assert(fnt_get_glyph(&font, '#') == NULL);
    assert(fnt_get_glyph(&font, 0) == NULL);
}

static void test_text_width(void) {
    FntFont font;
    assert(fnt_open(&font, test_fnt, sizeof(test_fnt)));

    assert(fnt_text_width(&font, " ") == 4);
    assert(fnt_text_width(&font, "! ") == 8);
    assert(fnt_text_width(&font, "\"") == 6);
    assert(fnt_text_width(&font, "") == 0);

    font.max_width = UINT16_MAX;
    char long_text[40001];
    memset(long_text, 'x', sizeof(long_text) - 1);
    long_text[sizeof(long_text) - 1] = '\0';
    assert(fnt_text_width(&font, long_text) == INT_MAX);
}

/* The fonts are OPTIONAL resources.  Every source-verification loop requires
 * only the resources below REQUIRED_COUNT, so a manifest gap in an optional
 * entry must never make an otherwise complete install fail to load.  Moving a
 * font above this line would break Amiga floppy loading outright, since no
 * Amiga font hash is known. */
static void test_font_resources_are_optional(void) {
    assert(LIBERATION_RESOURCE_REQUIRED_COUNT == 7);
    assert(LIBERATION_RESOURCE_FONT_0 == LIBERATION_RESOURCE_REQUIRED_COUNT);
    assert(LIBERATION_RESOURCE_FONT_1 == LIBERATION_RESOURCE_FONT_0 + 1);
    assert(LIBERATION_RESOURCE_COUNT == LIBERATION_RESOURCE_FONT_1 + 1);
    assert(LIBERATION_RESOURCE_MISSION_MENU < LIBERATION_RESOURCE_REQUIRED_COUNT);
}

/* Decode the real 0Liberation.FNT when a path to it is supplied.  The bundled
 * fixture above is a hand-built 3-glyph header; this is the only check that
 * the decoder agrees with the original file. */
static void test_real_font_decodes_if_available(void) {
    const char *path = getenv("OPENCAPTIVE_TEST_FNT");
    if (!path) {
        printf("SKIP: real_font (set OPENCAPTIVE_TEST_FNT to a .FNT path)\n");
        return;
    }
    FILE *f = fopen(path, "rb");
    if (!f) {
        printf("SKIP: real_font (%s not readable)\n", path);
        return;
    }
    static uint8_t buf[65536];
    size_t n = fread(buf, 1, sizeof(buf), f);
    fclose(f);

    FntFont font;
    assert(fnt_open(&font, buf, n));
    /* The original CD32 font: 114 proportional glyphs in two bitplanes. */
    assert(font.glyph_count == 114);
    assert(font.num_planes == 2);
    assert(font.max_width == 8);
    /* Every printable ASCII glyph resolves, lowercase included — the invented
     * 5x7 tables this font is meant to replace cannot render lowercase at all. */
    for (int c = 32; c < 127; c++) assert(fnt_get_glyph(&font, c) != NULL);
    /* Proportional, not fixed width. */
    assert(fnt_text_width(&font, "CAPTIVE") != fnt_text_width(&font, "Captive"));
    printf("PASS: real_font (%u glyphs)\n", font.glyph_count);
}

int main(void) {
    test_open();
    test_bad_magic();
    test_get_glyph();
    test_text_width();
    test_font_resources_are_optional();
    test_real_font_decodes_if_available();
    printf("All liberation_fnt tests passed\n");
    return 0;
}
