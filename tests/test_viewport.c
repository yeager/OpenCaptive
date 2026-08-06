#include "viewport.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

static Texture test_texture;

const Texture *gfx_get(const GfxData *gfx, int id) {
    if (!gfx || id < 0 || id >= gfx->num_textures) return NULL;
    return &gfx->textures[id];
}

static bool viewport_changed(const uint32_t *framebuffer) {
    for (int y = CAPTIVE_VIEWPORT_Y;
         y < CAPTIVE_VIEWPORT_Y + CAPTIVE_VIEWPORT_HEIGHT; ++y) {
        for (int x = CAPTIVE_VIEWPORT_X;
             x < CAPTIVE_VIEWPORT_X + CAPTIVE_VIEWPORT_WIDTH; ++x) {
            if (framebuffer[y * CAPTIVE_ORIGINAL_WIDTH + x] != 0xFF010203)
                return true;
        }
    }
    return false;
}

int main(void) {
    static uint32_t texture_pixels[320 * 200];
    static uint32_t framebuffer[CAPTIVE_ORIGINAL_WIDTH * CAPTIVE_ORIGINAL_HEIGHT];
    static GameState gs;
    static CreatureList creatures;
    memset(texture_pixels, 0x34, sizeof(texture_pixels));
    memset(&gs, 0, sizeof(gs));
    memset(&creatures, 0, sizeof(creatures));

    test_texture.pixels = texture_pixels;
    test_texture.width = 320;
    test_texture.height = 200;
    test_texture.loaded = true;

    TextureAtlas atlas;
    memset(&atlas, 0, sizeof(atlas));
    atlas.gfx.num_textures = 1;
    atlas.gfx.textures[0] = test_texture;
    atlas.loaded = true;
    for (int i = 0; i < 5; ++i) atlas.wall_sheets[i] = 0;
    atlas.roof_sheet = 0;
    atlas.door_sheet = 0;
    atlas.icon_sheet = 0;
    atlas.object_sheet = 0;
    for (int i = 0; i < 6; ++i) atlas.alien_sheets[i] = 0;

    gs.num_levels = 1;
    gs.current_level = 0;
    gs.party_x = 10;
    gs.party_y = 10;
    gs.party_dir = DIR_NORTH;
    for (int y = 0; y < MAP_HEIGHT; ++y)
        for (int x = 0; x < MAP_WIDTH; ++x)
            gs.levels[0].cells[y][x].type = CELL_FLOOR;
    gs.levels[0].cells[9][10].type = CELL_WALL;

    CaptiveViewWindow window;
    captive_view_window_build(&gs, &window);
    for (size_t i = 0; i < sizeof(framebuffer) / sizeof(framebuffer[0]); ++i)
        framebuffer[i] = 0xFF010203;
    viewport_render(&window, &atlas, framebuffer,
                    CAPTIVE_ORIGINAL_WIDTH, CAPTIVE_ORIGINAL_HEIGHT);
    assert(viewport_changed(framebuffer));

    creatures.num_creatures = 1;
    creatures.creatures[0].type = CREATURE_ALIEN1;
    creatures.creatures[0].active = true;
    creatures.creatures[0].level = 0;
    creatures.creatures[0].x = 11;
    creatures.creatures[0].y = 10;
    for (size_t i = 0; i < sizeof(framebuffer) / sizeof(framebuffer[0]); ++i)
        framebuffer[i] = 0xFF010203;
    viewport_render_creatures(&gs, &creatures, &atlas, framebuffer,
                              CAPTIVE_ORIGINAL_WIDTH,
                              CAPTIVE_ORIGINAL_HEIGHT);
    assert(viewport_changed(framebuffer));

    creatures.creatures[0].x = 10;
    creatures.creatures[0].y = 9; /* Wall directly ahead of the party. */
    for (size_t i = 0; i < sizeof(framebuffer) / sizeof(framebuffer[0]); ++i)
        framebuffer[i] = 0xFF010203;
    viewport_render_creatures(&gs, &creatures, &atlas, framebuffer,
                              CAPTIVE_ORIGINAL_WIDTH,
                              CAPTIVE_ORIGINAL_HEIGHT);
    assert(!viewport_changed(framebuffer));
    return 0;
}
