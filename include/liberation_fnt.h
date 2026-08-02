#ifndef LIBERATION_FNT_H
#define LIBERATION_FNT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define FNT_GLYPH_HEIGHT 7
#define FNT_GLYPH_MAX_WIDTH 8
#define FNT_FIRST_CHAR 32
#define FNT_MAX_GLYPHS 128

typedef struct {
    uint8_t width;
    uint8_t plane0[FNT_GLYPH_HEIGHT];
    uint8_t plane1[FNT_GLYPH_HEIGHT];
} FntGlyph;

typedef struct {
    uint16_t glyph_count;
    uint16_t max_width;
    uint16_t num_planes;
    FntGlyph glyphs[FNT_MAX_GLYPHS];
} FntFont;

bool fnt_open(FntFont *font, const uint8_t *data, size_t size);
const FntGlyph *fnt_get_glyph(const FntFont *font, int ch);
int fnt_text_width(const FntFont *font, const char *text);

#endif
