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

static void fnt_put(uint32_t *dst, int dst_w, int dst_h,
                    int x, int y, int scale, uint32_t color) {
    for (int sy = 0; sy < scale; sy++) {
        int py = y + sy;
        if (py < 0 || py >= dst_h) continue;
        for (int sx = 0; sx < scale; sx++) {
            int px = x + sx;
            if (px < 0 || px >= dst_w) continue;
            dst[py * dst_w + px] = color;
        }
    }
}

int fnt_blit_glyph(const FntFont *font, const FntGlyph *g, uint32_t *dst,
                   int dst_w, int dst_h, int x, int y,
                   uint32_t ink, uint32_t shadow, int scale) {
    if (!font || !g || !dst || dst_w <= 0 || dst_h <= 0) return 0;
    if (scale < 1) scale = 1;
    int cols = g->width;
    if (cols > FNT_GLYPH_MAX_WIDTH) cols = FNT_GLYPH_MAX_WIDTH;
    for (int row = 0; row < FNT_GLYPH_HEIGHT; row++) {
        for (int c = 0; c < cols; c++) {
            int bit = 7 - c;
            /* plane0 is the glyph ink; plane1 is the original outline/shadow
             * plane.  Both come straight from the decoded font — no shape is
             * invented — and ink wins where the two overlap. */
            if (g->plane0[row] & (1 << bit)) {
                fnt_put(dst, dst_w, dst_h, x + c * scale, y + row * scale,
                        scale, ink);
            } else if (shadow != 0u && (g->plane1[row] & (1 << bit))) {
                fnt_put(dst, dst_w, dst_h, x + c * scale, y + row * scale,
                        scale, shadow);
            }
        }
    }
    return g->width * scale;
}

int fnt_blit_text(const FntFont *font, uint32_t *dst, int dst_w, int dst_h,
                  int x, int y, const char *text,
                  uint32_t ink, uint32_t shadow, int scale) {
    if (!font || !dst || !text) return 0;
    if (scale < 1) scale = 1;
    int pen = x;
    for (; *text; text++) {
        const FntGlyph *g = fnt_get_glyph(font, (unsigned char)*text);
        if (!g) { pen += font->max_width * scale; continue; }
        pen += fnt_blit_glyph(font, g, dst, dst_w, dst_h, pen, y,
                              ink, shadow, scale);
    }
    return pen - x;
}
