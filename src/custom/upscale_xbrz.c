#include "custom_features.h"
#include <limits.h>
#include <string.h>
#include <math.h>

static inline uint8_t get_r(uint32_t c) { return (c >> 16) & 0xFF; }
static inline uint8_t get_g(uint32_t c) { return (c >> 8) & 0xFF; }
static inline uint8_t get_b(uint32_t c) { return c & 0xFF; }
static inline uint8_t get_a(uint32_t c) { return (c >> 24) & 0xFF; }

static inline uint32_t make_argb(uint8_t a, uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)a << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

static inline uint32_t blend(uint32_t c1, uint32_t c2, int w1, int w2) {
    int total = w1 + w2;
    return make_argb(
        (uint8_t)((get_a(c1) * w1 + get_a(c2) * w2) / total),
        (uint8_t)((get_r(c1) * w1 + get_r(c2) * w2) / total),
        (uint8_t)((get_g(c1) * w1 + get_g(c2) * w2) / total),
        (uint8_t)((get_b(c1) * w1 + get_b(c2) * w2) / total));
}

static inline uint32_t sample(const uint32_t *src, int w, int h, int x, int y) {
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x >= w) x = w - 1;
    if (y >= h) y = h - 1;
    return src[y * w + x];
}

static inline bool upscale_args_valid(const uint32_t *src, int src_w,
                                      int src_h, uint32_t *dst, int factor) {
    return src != NULL && dst != NULL && src_w > 0 && src_h > 0 &&
           factor > 0 && src_w <= INT_MAX / factor &&
           src_h <= INT_MAX / factor;
}

void upscale_xbrz_2x(const uint32_t *src, int src_w, int src_h,
                      uint32_t *dst) {
    if (!upscale_args_valid(src, src_w, src_h, dst, 2)) return;
    int dst_w = src_w * 2;

    for (int y = 0; y < src_h; y++) {
        for (int x = 0; x < src_w; x++) {
            uint32_t e = sample(src, src_w, src_h, x, y);
            uint32_t b = sample(src, src_w, src_h, x, y - 1);
            uint32_t d = sample(src, src_w, src_h, x - 1, y);
            uint32_t f = sample(src, src_w, src_h, x + 1, y);
            uint32_t h = sample(src, src_w, src_h, x, y + 1);

            uint32_t e0 = e, e1 = e, e2 = e, e3 = e;

            if (b != h && d != f) {
                if (d == b) e0 = blend(e, d, 1, 1);
                if (b == f) e1 = blend(e, f, 1, 1);
                if (d == h) e2 = blend(e, d, 1, 1);
                if (h == f) e3 = blend(e, f, 1, 1);
            }

            int dx = x * 2, dy = y * 2;
            dst[dy * dst_w + dx]     = e0;
            dst[dy * dst_w + dx + 1] = e1;
            dst[(dy + 1) * dst_w + dx]     = e2;
            dst[(dy + 1) * dst_w + dx + 1] = e3;
        }
    }
}

void upscale_xbrz_3x(const uint32_t *src, int src_w, int src_h,
                      uint32_t *dst) {
    if (!upscale_args_valid(src, src_w, src_h, dst, 3)) return;
    int dst_w = src_w * 3;

    for (int y = 0; y < src_h; y++) {
        for (int x = 0; x < src_w; x++) {
            uint32_t e = sample(src, src_w, src_h, x, y);
            uint32_t a = sample(src, src_w, src_h, x - 1, y - 1);
            uint32_t b = sample(src, src_w, src_h, x, y - 1);
            uint32_t c = sample(src, src_w, src_h, x + 1, y - 1);
            uint32_t d = sample(src, src_w, src_h, x - 1, y);
            uint32_t f = sample(src, src_w, src_h, x + 1, y);
            uint32_t g = sample(src, src_w, src_h, x - 1, y + 1);
            uint32_t h = sample(src, src_w, src_h, x, y + 1);
            uint32_t i = sample(src, src_w, src_h, x + 1, y + 1);

            uint32_t p[9] = {e,e,e, e,e,e, e,e,e};

            if (b != h && d != f) {
                if (d == b) { p[0] = d; p[1] = blend(e, b, 1, 1); }
                if (b == f) { p[2] = f; p[1] = blend(e, b, 1, 1); }
                if (d == h) { p[6] = d; p[7] = blend(e, h, 1, 1); }
                if (h == f) { p[8] = f; p[7] = blend(e, h, 1, 1); }
                if (d == b && e != c) p[2] = blend(e, d, 3, 1);
                if (b == f && e != a) p[0] = blend(e, f, 3, 1);
                if (d == h && e != i) p[8] = blend(e, d, 3, 1);
                if (h == f && e != g) p[6] = blend(e, f, 3, 1);
                p[3] = (d == b || d == h) ? blend(e, d, 1, 1) : e;
                p[5] = (b == f || h == f) ? blend(e, f, 1, 1) : e;
            }

            int dx = x * 3, dy = y * 3;
            for (int py = 0; py < 3; py++)
                for (int px = 0; px < 3; px++)
                    dst[(dy + py) * dst_w + dx + px] = p[py * 3 + px];
        }
    }
}

void upscale_xbrz_4x(const uint32_t *src, int src_w, int src_h,
                      uint32_t *dst) {
    if (!upscale_args_valid(src, src_w, src_h, dst, 4)) return;
    int dst_w = src_w * 4;

    for (int y = 0; y < src_h; y++) {
        for (int x = 0; x < src_w; x++) {
            uint32_t e = sample(src, src_w, src_h, x, y);
            uint32_t b = sample(src, src_w, src_h, x, y - 1);
            uint32_t d = sample(src, src_w, src_h, x - 1, y);
            uint32_t f = sample(src, src_w, src_h, x + 1, y);
            uint32_t h = sample(src, src_w, src_h, x, y + 1);
            uint32_t p[16];
            for (int k = 0; k < 16; k++) p[k] = e;

            if (b != h && d != f) {
                if (d == b) {
                    p[0] = d;
                    p[1] = blend(d, e, 3, 1);
                    p[4] = blend(d, e, 3, 1);
                    p[5] = blend(d, e, 1, 1);
                }
                if (b == f) {
                    p[2] = blend(f, e, 1, 1);
                    p[3] = f;
                    p[6] = blend(f, e, 1, 1);
                    p[7] = blend(f, e, 3, 1);
                }
                if (d == h) {
                    p[8] = blend(d, e, 3, 1);
                    p[9] = blend(d, e, 1, 1);
                    p[12] = d;
                    p[13] = blend(d, e, 3, 1);
                }
                if (h == f) {
                    p[10] = blend(f, e, 1, 1);
                    p[11] = blend(f, e, 3, 1);
                    p[14] = blend(f, e, 3, 1);
                    p[15] = f;
                }
            }

            int dx = x * 4, dy = y * 4;
            for (int py = 0; py < 4; py++)
                for (int px = 0; px < 4; px++)
                    dst[(dy + py) * dst_w + dx + px] = p[py * 4 + px];
        }
    }
}
