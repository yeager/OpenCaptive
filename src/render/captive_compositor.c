#include "captive_compositor.h"
#include "captive_dos_descriptor.h"
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

bool captive_compositor_blit_indices(
    uint32_t *view, int stride, const uint8_t *indices,
    int source_width, int source_height,
    const uint32_t palette[CAPTIVE_COMPOSITOR_PALETTE_SIZE],
    uint16_t destination_offset, uint8_t flags) {
    int destination_x = 0;
    int destination_y = 0;
    if (!view || !indices || !palette ||
        stride < (int)CAPTIVE_DOS_VIEW_STRIDE ||
        source_width <= 0 || source_height <= 0 ||
        !captive_dos_descriptor_destination_xy(destination_offset,
                                                &destination_x,
                                                &destination_y))
        return false;

    for (int y = 0; y < source_height; ++y) {
        int dy = destination_y + y;
        if (dy < 0 || dy >= (int)CAPTIVE_DOS_VIEW_HEIGHT) continue;
        for (int x = 0; x < source_width; ++x) {
            int source_x = (flags & 0x01U) ? source_width - 1 - x : x;
            uint8_t index = indices[y * source_width + source_x];
            if ((flags & 0x04U) && index == 0) continue;
            int dx = destination_x + x;
            if (dx < 0 || dx >= CAPTIVE_VIEWPORT_WIDTH) continue;
            view[dy * stride + dx] = palette[index &
                                              (CAPTIVE_COMPOSITOR_PALETTE_SIZE - 1)];
        }
    }
    return true;
}
