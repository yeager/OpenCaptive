#include "terminal.h"
#include <assert.h>
#include <string.h>

static void test_inactive_terminal_ignores_input(void) {
    TerminalState ts;
    terminal_init(&ts, 0);
    ts.active = false;
    ts.page = TERM_MAIN;
    ts.cursor = 0;

    assert(!terminal_handle_key(&ts, 0x51));
    assert(ts.cursor == 0);
    assert(!terminal_handle_key(&ts, 0x0D));
    assert(ts.page == TERM_MAIN);
}

static void test_terminal_render_clips_small_framebuffer(void) {
    TerminalState ts;
    GameState gs;
    uint32_t pixels[16 * 16];
    memset(&gs, 0, sizeof(gs));
    gs.current_level = 0;
    gs.num_levels = 1;
    gs.party_x = 1;
    gs.party_y = 1;
    terminal_init(&ts, 0);
    memset(pixels, 0xA5, sizeof(pixels));
    terminal_render(&ts, &gs, pixels, 16, 16);
}

int main(void) {
    test_inactive_terminal_ignores_input();
    test_terminal_render_clips_small_framebuffer();
    return 0;
}
