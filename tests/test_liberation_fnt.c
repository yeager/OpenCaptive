#include "liberation_fnt.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
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

int main(void) {
    test_open();
    test_bad_magic();
    test_get_glyph();
    test_text_width();
    printf("All liberation_fnt tests passed\n");
    return 0;
}
