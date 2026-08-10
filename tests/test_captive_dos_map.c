#include "captive_dos_map.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    uint8_t *memory = calloc(1, 0x100000u);
    assert(memory);
    const uint16_t ds = 0x2942;
    const size_t base = (size_t)ds * 16U;
    memory[base + CAPTIVE_DOS_POSITION_X_OFFSET] = 4;
    memory[base + CAPTIVE_DOS_POSITION_Y_OFFSET] = 5;
    memory[base + CAPTIVE_DOS_FACING_OFFSET] = 0x07;
    memory[base + CAPTIVE_DOS_MAP_OFFSET + 5 * 64 + 4] = 0xB6;

    CaptiveDosMapState state;
    assert(captive_dos_map_decode(memory, 0x100000u, ds, &state));
    assert(state.player_x == 4 && state.player_y == 5);
    assert(state.facing == 3);
    uint8_t raw = 0;
    assert(captive_dos_map_cell(&state, 4, 5, &raw));
    assert(raw == 0xB6);
    assert(!captive_dos_map_cell(&state, -1, 0, &raw));
    for (int y = 0; y < CAPTIVE_DOS_MAP_HEIGHT; ++y)
        for (int x = 0; x < CAPTIVE_DOS_MAP_WIDTH; ++x)
            memory[base + CAPTIVE_DOS_MAP_OFFSET + y * 64 + x] =
                (uint8_t)(y * 64 + x);
    assert(captive_dos_map_decode(memory, 0x100000u, ds, &state));
    CaptiveDosViewWindow window;
    assert(captive_dos_view_window_build(&state, &window));
    assert(window.facing == 3);
    assert(window.raw[0][0] == (uint8_t)(3 * 64 + 8));
    assert(window.raw[0][4] == (uint8_t)(7 * 64 + 8));
    assert(window.raw[4][0] == (uint8_t)(3 * 64 + 4));
    assert(window.raw[4][4] == (uint8_t)(7 * 64 + 4));

    state.player_x = 0;
    state.player_y = 0;
    assert(captive_dos_view_window_build(&state, &window));
    assert(window.outside[0][0]);
    assert(!captive_dos_map_decode(memory, 0xFFFFFu, ds, &state));
    free(memory);
    return 0;
}
