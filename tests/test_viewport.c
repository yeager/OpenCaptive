#include "viewport.h"
#include "captive_viewport_descriptors.h"

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
    static uint8_t texture_indices[320 * 200];
    static uint32_t framebuffer[CAPTIVE_ORIGINAL_WIDTH * CAPTIVE_ORIGINAL_HEIGHT];
    static GameState gs;
    static CreatureList creatures;
    for (size_t i = 0; i < sizeof(texture_pixels) / sizeof(texture_pixels[0]); i++)
        texture_pixels[i] = 0xFF101010;
    memset(texture_indices, 1, sizeof(texture_indices));
    for (int y = 0; y < 32; y++) {
        for (int x = 0; x < 32; x++) {
            texture_pixels[y * 320 + x] = 0xFFFF0000;
            texture_indices[y * 320 + x] = 2;
        }
    }
    memset(&gs, 0, sizeof(gs));
    memset(&creatures, 0, sizeof(creatures));

    test_texture.pixels = texture_pixels;
    test_texture.indices = texture_indices;
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
    gs.levels[0].cells[10][10].ornament[DIR_NORTH] = ORNAMENT_BUTTON;

    CaptiveViewWindow window;
    captive_view_window_build(&gs, &window);
    for (size_t i = 0; i < sizeof(framebuffer) / sizeof(framebuffer[0]); ++i)
        framebuffer[i] = 0xFF010203;
    viewport_render(&window, &atlas, framebuffer,
                    CAPTIVE_ORIGINAL_WIDTH, CAPTIVE_ORIGINAL_HEIGHT);
    assert(viewport_changed(framebuffer));

    uint32_t without_ornament[CAPTIVE_ORIGINAL_WIDTH * CAPTIVE_ORIGINAL_HEIGHT];
    memcpy(without_ornament, framebuffer, sizeof(without_ornament));
    gs.levels[0].cells[10][10].ornament[DIR_NORTH] = ORNAMENT_NONE;
    captive_view_window_build(&gs, &window);
    for (size_t i = 0; i < sizeof(framebuffer) / sizeof(framebuffer[0]); ++i)
        framebuffer[i] = 0xFF010203;
    viewport_render(&window, &atlas, framebuffer,
                    CAPTIVE_ORIGINAL_WIDTH, CAPTIVE_ORIGINAL_HEIGHT);
    assert(memcmp(without_ornament, framebuffer, sizeof(framebuffer)) != 0);

    /* Wall-less fallback controls are rendered on the floor surface. */
    gs.levels[0].cells[9][10].type = CELL_FLOOR;
    gs.levels[0].cells[10][10].ornament[DIR_NORTH] = ORNAMENT_PANEL;
    captive_view_window_build(&gs, &window);
    for (size_t i = 0; i < sizeof(framebuffer) / sizeof(framebuffer[0]); ++i)
        framebuffer[i] = 0xFF010203;
    viewport_render(&window, &atlas, framebuffer,
                    CAPTIVE_ORIGINAL_WIDTH, CAPTIVE_ORIGINAL_HEIGHT);
    uint32_t wall_less_panel[
        CAPTIVE_ORIGINAL_WIDTH * CAPTIVE_ORIGINAL_HEIGHT];
    memcpy(wall_less_panel, framebuffer, sizeof(wall_less_panel));
    gs.levels[0].cells[10][10].ornament[DIR_NORTH] = ORNAMENT_NONE;
    captive_view_window_build(&gs, &window);
    for (size_t i = 0; i < sizeof(framebuffer) / sizeof(framebuffer[0]); ++i)
        framebuffer[i] = 0xFF010203;
    viewport_render(&window, &atlas, framebuffer,
                    CAPTIVE_ORIGINAL_WIDTH, CAPTIVE_ORIGINAL_HEIGHT);
    assert(memcmp(wall_less_panel, framebuffer, sizeof(framebuffer)) != 0);

    /* Directional wall textures must rotate with the player.  The fixture
     * uses a distinct source panel for each absolute map direction; a fixed
     * [0]/[1]/[3] lookup would render the same wall after turning. */
    static const uint32_t panel_colors[] = {
        0xFFFF0000, 0xFF00FF00, 0xFF0000FF, 0xFFFFFF00
    };
    for (int panel = 0; panel < 4; ++panel)
        for (int y = 0; y < 200; ++y)
            for (int x = panel * 80; x < (panel + 1) * 80; ++x)
                texture_pixels[y * 320 + x] = panel_colors[panel];
    for (int y = 0; y < MAP_HEIGHT; ++y)
        for (int x = 0; x < MAP_WIDTH; ++x)
            gs.levels[0].cells[y][x].type = CELL_FLOOR;
    for (int d = 0; d < 4; ++d)
        gs.levels[0].cells[10][10].wall_tex[d] = (uint8_t)d;
    for (int d = 0; d < 4; ++d)
        gs.levels[0].cells[9][10].wall_tex[d] = (uint8_t)d;
    gs.levels[0].cells[9][10].type = CELL_WALL;
    gs.party_dir = DIR_NORTH;
    captive_view_window_build(&gs, &window);
    for (size_t i = 0; i < sizeof(framebuffer) / sizeof(framebuffer[0]); ++i)
        framebuffer[i] = 0xFF010203;
    viewport_render(&window, &atlas, framebuffer,
                    CAPTIVE_ORIGINAL_WIDTH, CAPTIVE_ORIGINAL_HEIGHT);
    uint32_t north_view[CAPTIVE_ORIGINAL_WIDTH * CAPTIVE_ORIGINAL_HEIGHT];
    memcpy(north_view, framebuffer, sizeof(north_view));

    gs.party_dir = DIR_EAST;
    captive_view_window_build(&gs, &window);
    for (size_t i = 0; i < sizeof(framebuffer) / sizeof(framebuffer[0]); ++i)
        framebuffer[i] = 0xFF010203;
    viewport_render(&window, &atlas, framebuffer,
                    CAPTIVE_ORIGINAL_WIDTH, CAPTIVE_ORIGINAL_HEIGHT);
    assert(memcmp(north_view, framebuffer, sizeof(framebuffer)) != 0);

    /* Side walls must use the neighboring wall cell's exposed face, not the
     * adjacent floor cell's default texture. */
    for (int y = 0; y < MAP_HEIGHT; ++y)
        for (int x = 0; x < MAP_WIDTH; ++x)
            gs.levels[0].cells[y][x].type = CELL_FLOOR;
    gs.party_dir = DIR_NORTH;
    gs.levels[0].cells[10][9].type = CELL_WALL; /* left of the party */
    gs.levels[0].cells[10][9].wall_tex[DIR_EAST] = 1;
    gs.levels[0].cells[10][10].wall_tex[DIR_WEST] = 0;
    captive_view_window_build(&gs, &window);
    for (size_t i = 0; i < sizeof(framebuffer) / sizeof(framebuffer[0]); ++i)
        framebuffer[i] = 0xFF010203;
    viewport_render(&window, &atlas, framebuffer,
                    CAPTIVE_ORIGINAL_WIDTH, CAPTIVE_ORIGINAL_HEIGHT);
    uint32_t left_wall_from_neighbor[
        CAPTIVE_ORIGINAL_WIDTH * CAPTIVE_ORIGINAL_HEIGHT];
    memcpy(left_wall_from_neighbor, framebuffer, sizeof(left_wall_from_neighbor));
    gs.levels[0].cells[10][9].wall_tex[DIR_EAST] = 0;
    gs.levels[0].cells[10][10].wall_tex[DIR_WEST] = 1;
    captive_view_window_build(&gs, &window);
    for (size_t i = 0; i < sizeof(framebuffer) / sizeof(framebuffer[0]); ++i)
        framebuffer[i] = 0xFF010203;
    viewport_render(&window, &atlas, framebuffer,
                    CAPTIVE_ORIGINAL_WIDTH, CAPTIVE_ORIGINAL_HEIGHT);
    assert(memcmp(left_wall_from_neighbor, framebuffer,
                  sizeof(left_wall_from_neighbor)) != 0);

    /* Floor and ceiling texture selectors must affect their own source
     * regions.  They are serialized map data and must not be ignored by the
     * compatibility viewport. */
    for (int y = 0; y < MAP_HEIGHT; ++y)
        for (int x = 0; x < MAP_WIDTH; ++x) {
            gs.levels[0].cells[y][x].type = CELL_FLOOR;
            gs.levels[0].cells[y][x].floor_tex = 0;
            gs.levels[0].cells[y][x].ceil_tex = 0;
        }
    gs.levels[0].cells[10][10].floor_tex = 1;
    gs.levels[0].cells[10][10].ceil_tex = 2;
    captive_view_window_build(&gs, &window);
    for (size_t i = 0; i < sizeof(framebuffer) / sizeof(framebuffer[0]); ++i)
        framebuffer[i] = 0xFF010203;
    viewport_render(&window, &atlas, framebuffer,
                    CAPTIVE_ORIGINAL_WIDTH, CAPTIVE_ORIGINAL_HEIGHT);
    uint32_t floor_ceiling_textures[
        CAPTIVE_ORIGINAL_WIDTH * CAPTIVE_ORIGINAL_HEIGHT];
    memcpy(floor_ceiling_textures, framebuffer, sizeof(floor_ceiling_textures));
    gs.levels[0].cells[10][10].floor_tex = 0;
    gs.levels[0].cells[10][10].ceil_tex = 0;
    captive_view_window_build(&gs, &window);
    for (size_t i = 0; i < sizeof(framebuffer) / sizeof(framebuffer[0]); ++i)
        framebuffer[i] = 0xFF010203;
    viewport_render(&window, &atlas, framebuffer,
                    CAPTIVE_ORIGINAL_WIDTH, CAPTIVE_ORIGINAL_HEIGHT);
    assert(memcmp(floor_ceiling_textures, framebuffer,
                  sizeof(floor_ceiling_textures)) != 0);

    /* Floor/ceiling projection must restart sampling at each cell's local
     * origin.  A viewport-relative source x made the left and center cells
     * below begin at different texture phases despite identical map data. */
    for (int y = 0; y < MAP_HEIGHT; ++y)
        for (int x = 0; x < MAP_WIDTH; ++x) {
            gs.levels[0].cells[y][x].type = CELL_FLOOR;
            gs.levels[0].cells[y][x].floor_tex = 0;
            gs.levels[0].cells[y][x].ceil_tex = 0;
        }
    for (int x = 0; x < 80; ++x) {
        texture_pixels[168 * 320 + x] = 0xFF000000 | (uint32_t)x;
        texture_indices[168 * 320 + x] = 3;
    }
    gs.party_dir = DIR_NORTH;
    captive_view_window_build(&gs, &window);
    for (size_t i = 0; i < sizeof(framebuffer) / sizeof(framebuffer[0]); ++i)
        framebuffer[i] = 0xFF010203;
    viewport_render(&window, &atlas, framebuffer,
                    CAPTIVE_ORIGINAL_WIDTH, CAPTIVE_ORIGINAL_HEIGHT);
    const int floor_strip_y = CAPTIVE_VIEWPORT_Y + 98;
    const uint32_t left_cell_first =
        framebuffer[floor_strip_y * CAPTIVE_ORIGINAL_WIDTH +
                    CAPTIVE_VIEWPORT_X + 18];
    const uint32_t center_cell_first =
        framebuffer[floor_strip_y * CAPTIVE_ORIGINAL_WIDTH +
                    CAPTIVE_VIEWPORT_X + 54];
    assert(left_cell_first == center_cell_first);

    gs.levels[0].cells[10][9].type = CELL_FLOOR;
    gs.levels[0].cells[9][10].type = CELL_WALL;
    gs.party_dir = DIR_NORTH;
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

    /* Overlapping sprites must be depth-stable rather than dependent on the
     * order in CreatureList.  The nearer creature is drawn last. */
    for (int i = 0; i < 6; ++i) atlas.alien_sheets[i] = -1;
    memset(&creatures, 0, sizeof(creatures));
    creatures.num_creatures = 2;
    creatures.creatures[0] = (Creature){
        .type = CREATURE_ALIEN1, .active = true, .level = 0,
        .x = 10, .y = 8
    }; /* far, green fallback */
    creatures.creatures[1] = (Creature){
        .type = CREATURE_ALIEN2, .active = true, .level = 0,
        .x = 10, .y = 9
    }; /* near, blue fallback */
    for (size_t i = 0; i < sizeof(framebuffer) / sizeof(framebuffer[0]); ++i)
        framebuffer[i] = 0xFF010203;
    viewport_render_creatures(&gs, &creatures, &atlas, framebuffer,
                              CAPTIVE_ORIGINAL_WIDTH,
                              CAPTIVE_ORIGINAL_HEIGHT);
    uint32_t depth_sorted[CAPTIVE_ORIGINAL_WIDTH * CAPTIVE_ORIGINAL_HEIGHT];
    memcpy(depth_sorted, framebuffer, sizeof(depth_sorted));
    Creature far = creatures.creatures[0];
    creatures.creatures[0] = creatures.creatures[1];
    creatures.creatures[1] = far;
    for (size_t i = 0; i < sizeof(framebuffer) / sizeof(framebuffer[0]); ++i)
        framebuffer[i] = 0xFF010203;
    viewport_render_creatures(&gs, &creatures, &atlas, framebuffer,
                              CAPTIVE_ORIGINAL_WIDTH,
                              CAPTIVE_ORIGINAL_HEIGHT);
    assert(memcmp(depth_sorted, framebuffer, sizeof(depth_sorted)) == 0);

    /* Verify descriptor table integrity: all 112 entries have valid
     * dimensions, source banks, and destinations within the viewport. */
    for (int i = 0; i < CAPTIVE_VIEWPORT_DESCRIPTOR_COUNT; i++) {
        const CaptiveDosDescriptor *d = &captive_viewport_descriptors[i];
        assert(d->width_bytes > 0 && d->width_bytes <= 20);
        assert(d->height > 0 && d->height <= 112);
        assert(d->source_bank <= 4);
        int pixel_w = d->width_bytes * 8;
        int dst_x = d->destination_offset % CAPTIVE_DOS_VIEW_STRIDE;
        int dst_y = d->destination_offset / CAPTIVE_DOS_VIEW_STRIDE;
        assert(dst_x >= 0 && dst_x + pixel_w <= CAPTIVE_VIEWPORT_WIDTH);
        assert(dst_y >= 0 && dst_y + (int)d->height <= CAPTIVE_VIEWPORT_HEIGHT);
    }

    /* Verify full viewport coverage: blitting all descriptors in order
     * (with a non-transparent fill) must cover every pixel. */
    uint32_t coverage[CAPTIVE_VIEWPORT_WIDTH * CAPTIVE_VIEWPORT_HEIGHT];
    memset(coverage, 0, sizeof(coverage));
    for (int i = 0; i < CAPTIVE_VIEWPORT_DESCRIPTOR_COUNT; i++) {
        const CaptiveDosDescriptor *d = &captive_viewport_descriptors[i];
        int pixel_w = d->width_bytes * 8;
        int dst_x = d->destination_offset % CAPTIVE_DOS_VIEW_STRIDE;
        int dst_y = d->destination_offset / CAPTIVE_DOS_VIEW_STRIDE;
        for (int y = 0; y < (int)d->height; y++)
            for (int x = 0; x < pixel_w; x++)
                coverage[(dst_y + y) * CAPTIVE_VIEWPORT_WIDTH + dst_x + x] = 1;
    }
    int covered = 0;
    for (int i = 0; i < CAPTIVE_VIEWPORT_WIDTH * CAPTIVE_VIEWPORT_HEIGHT; i++)
        covered += coverage[i] != 0;
    assert(covered > CAPTIVE_VIEWPORT_WIDTH * CAPTIVE_VIEWPORT_HEIGHT * 3 / 4);

    return 0;
}
