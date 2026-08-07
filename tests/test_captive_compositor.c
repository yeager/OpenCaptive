#include "captive_compositor.h"
#include "pl5_decoder.h"
#include "opencaptive.h"

#include <assert.h>
#include <string.h>

int main(void) {
    uint32_t view[CAPTIVE_DOS_VIEW_STRIDE * CAPTIVE_DOS_VIEW_HEIGHT];
    uint32_t first_pixels[] = { 0xFF112233, 0xFF445566, 0xFF778899, 0xFFAABBCC };
    uint32_t second_pixels[] = { 0xFF000000, 0xFFDEADBE, 0xFF000000, 0xFF000000 };
    uint8_t first_mask[] = { 1, 1, 1, 1 };
    uint8_t second_mask[] = { 0, 1, 0, 0 };
    CaptivePanelBlit panels[] = {
        { first_pixels, first_mask, 2, 2, 0, 0, 2, 2, 1, 1 },
        { second_pixels, second_mask, 2, 2, 0, 0, 2, 2, 1, 1 },
    };

    memset(view, 0, sizeof(view));
    assert(captive_compositor_blit_all(view, CAPTIVE_VIEWPORT_WIDTH, panels, 2));
    assert(view[1 * CAPTIVE_VIEWPORT_WIDTH + 1] == 0xFF112233);
    assert(view[1 * CAPTIVE_VIEWPORT_WIDTH + 2] == 0xFFDEADBE);
    assert(view[2 * CAPTIVE_VIEWPORT_WIDTH + 1] == 0xFF778899);
    assert(view[2 * CAPTIVE_VIEWPORT_WIDTH + 2] == 0xFFAABBCC);

    panels[0].destination_x = -1;
    panels[0].destination_y = -1;
    assert(captive_compositor_blit(view, CAPTIVE_VIEWPORT_WIDTH, &panels[0]));
    assert(view[0] == 0xFFAABBCC);
    assert(!captive_compositor_blit(NULL, CAPTIVE_VIEWPORT_WIDTH, &panels[0]));
    assert(!captive_compositor_blit(view, CAPTIVE_VIEWPORT_WIDTH - 1, &panels[0]));

    uint8_t indices[] = {0, 16, 18, 3};
    uint32_t palette[CAPTIVE_COMPOSITOR_PALETTE_SIZE] = {0};
    palette[3] = 0xFF123456;
    palette[16] = 0xFF000000;
    palette[18] = 0xFF000000;
    memset(view, 0, sizeof(view));
    assert(captive_compositor_blit_indices(view, CAPTIVE_DOS_VIEW_STRIDE,
                                           indices, 4, 1, palette,
                                           2 * CAPTIVE_DOS_VIEW_STRIDE,
                                           0x05));
    assert(view[2 * CAPTIVE_DOS_VIEW_STRIDE + 0] == palette[3]);
    assert(view[2 * CAPTIVE_DOS_VIEW_STRIDE + 1] == palette[18]);
    assert(view[2 * CAPTIVE_DOS_VIEW_STRIDE + 2] == palette[16]);
    assert(view[2 * CAPTIVE_DOS_VIEW_STRIDE + 3] == 0);
    assert(!captive_compositor_blit_indices(view, CAPTIVE_VIEWPORT_WIDTH,
                                            indices, 4, 1, palette, 0, 0));
    return 0;
}
