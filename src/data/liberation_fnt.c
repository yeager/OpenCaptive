#include "liberation_fnt.h"
#include <limits.h>
#include <string.h>

static uint16_t read_be16(const uint8_t *p) {
    return (uint16_t)(p[0] << 8 | p[1]);
}

bool fnt_open(FntFont *font, const uint8_t *data, size_t size) {
    if (!font) return false;
    memset(font, 0, sizeof(*font));
    if (!data || size < 12) return false;
    if (memcmp(data, "CHAR", 4) != 0) return false;

    font->glyph_count = read_be16(data + 4);
    font->max_width = read_be16(data + 8);
    font->num_planes = read_be16(data + 10);

    if (font->glyph_count > FNT_MAX_GLYPHS) return false;
    if (font->num_planes != 2) return false;
    if (12 + (size_t)font->glyph_count * 16 > size) return false;

    for (unsigned i = 0; i < font->glyph_count; i++) {
        const uint8_t *g = data + 12 + i * 16;
        font->glyphs[i].width = g[0];
        memcpy(font->glyphs[i].plane0, g + 1, FNT_GLYPH_HEIGHT);
        memcpy(font->glyphs[i].plane1, g + 9, FNT_GLYPH_HEIGHT);
    }

    return true;
}

const FntGlyph *fnt_get_glyph(const FntFont *font, int ch) {
    if (!font) return NULL;
    int idx = ch - FNT_FIRST_CHAR;
    if (idx < 0 || idx >= (int)font->glyph_count) return NULL;
    return &font->glyphs[idx];
}

int fnt_text_width(const FntFont *font, const char *text) {
    if (!font || !text) return 0;
    int w = 0;
    for (; *text; text++) {
        const FntGlyph *g = fnt_get_glyph(font, (unsigned char)*text);
        int advance = g ? g->width : font->max_width;
        if (advance > INT_MAX - w) return INT_MAX;
        w += advance;
    }
    return w;
}
