#include "captive_compositor.h"
#include "opencaptive.h"

bool captive_compositor_blit(uint32_t *view, int stride,
                             const CaptivePanelBlit *panel) {
    if (!view || !panel || !panel->pixels || !panel->mask ||
        stride < CAPTIVE_VIEWPORT_WIDTH ||
        panel->source_width <= 0 || panel->source_height <= 0 ||
        panel->width < 0 || panel->height < 0) {
        return false;
    }

    for (int y = 0; y < panel->height; ++y) {
        const int sy = panel->source_y + y;
        const int dy = panel->destination_y + y;
        if (sy < 0 || sy >= panel->source_height ||
            dy < 0 || dy >= CAPTIVE_VIEWPORT_HEIGHT) {
            continue;
        }
        for (int x = 0; x < panel->width; ++x) {
            const int sx = panel->source_x + x;
            const int dx = panel->destination_x + x;
            if (sx < 0 || sx >= panel->source_width ||
                dx < 0 || dx >= CAPTIVE_VIEWPORT_WIDTH) {
                continue;
            }
            const size_t source = (size_t)sy * (size_t)panel->source_width +
                                  (size_t)sx;
            if (panel->mask[source])
                view[dy * stride + dx] = panel->pixels[source];
        }
    }
    return true;
}

bool captive_compositor_blit_all(uint32_t *view, int stride,
                                 const CaptivePanelBlit *panels,
                                 int panel_count) {
    if (!view || stride < CAPTIVE_VIEWPORT_WIDTH || panel_count < 0 ||
        (panel_count > 0 && !panels)) {
        return false;
    }
    for (int i = 0; i < panel_count; ++i) {
        if (!captive_compositor_blit(view, stride, &panels[i]))
            return false;
    }
    return true;
}
