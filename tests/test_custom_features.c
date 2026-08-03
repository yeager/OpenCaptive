#include "custom_features.h"
#include "game_state.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static void test_defaults(void) {
    CustomFeatures f;
    custom_features_defaults(&f);
    assert(!f.hd_upscale);
    assert(f.upscale_factor == 2);
    assert(!f.widescreen);
    assert(!f.quicksave);
    assert(!f.minimap);
    assert(f.minimap_opacity > 0.5f && f.minimap_opacity < 0.7f);
    assert(f.minimap_size == 96);
    assert(f.game_speed > 0.9f && f.game_speed < 1.1f);
    assert(f.mouse_sensitivity > 0.9f && f.mouse_sensitivity < 1.1f);
    assert(!f.debug_hud);
    assert(!f.audio_reverb);
    assert(f.reverb_amount > 0.2f && f.reverb_amount < 0.4f);
    assert(f.audio_sample_rate == 44100);
    assert(!f.automap);
    assert(!f.cross_save);
    assert(!f.replay_record);
    assert(!f.texture_filter);
    assert(!f.dynamic_lighting);
}

static void test_save_load(void) {
    CustomFeatures f1, f2;
    custom_features_defaults(&f1);
    f1.hd_upscale = true;
    f1.upscale_factor = 3;
    f1.minimap = true;
    f1.debug_hud = true;
    f1.audio_reverb = true;
    f1.automap = true;
    f1.dynamic_lighting = true;
    f1.game_speed = 2.0f;

    const char *path = "/tmp/test_opencaptive_features.cfg";
    assert(custom_features_save(&f1, path));

    custom_features_defaults(&f2);
    assert(custom_features_load(&f2, path));

    assert(f2.hd_upscale == true);
    assert(f2.upscale_factor == 3);
    assert(f2.minimap == true);
    assert(f2.debug_hud == true);
    assert(f2.audio_reverb == true);
    assert(f2.automap == true);
    assert(f2.dynamic_lighting == true);
    assert(f2.game_speed > 1.9f && f2.game_speed < 2.1f);

    remove(path);
}

static void test_upscale_2x(void) {
    uint32_t src[4] = {0xFF000000, 0xFFFF0000, 0xFF00FF00, 0xFF0000FF};
    uint32_t dst[16];
    memset(dst, 0, sizeof(dst));
    upscale_xbrz_2x(src, 2, 2, dst);

    // Corners should be dominated by their source pixel
    assert((dst[0] & 0xFF000000) == 0xFF000000);
    assert((dst[3] & 0xFF000000) == 0xFF000000);
    assert((dst[12] & 0xFF000000) == 0xFF000000);
    assert((dst[15] & 0xFF000000) == 0xFF000000);
}

static void test_upscale_3x(void) {
    uint32_t src[4] = {0xFFFF0000, 0xFF00FF00, 0xFF0000FF, 0xFFFFFF00};
    uint32_t dst[36];
    memset(dst, 0, sizeof(dst));
    upscale_xbrz_3x(src, 2, 2, dst);
    assert((dst[0] & 0xFF000000) == 0xFF000000);
}

static void test_upscale_4x(void) {
    uint32_t src[4] = {0xFFFFFFFF, 0xFF000000, 0xFF808080, 0xFF404040};
    uint32_t dst[64];
    memset(dst, 0, sizeof(dst));
    upscale_xbrz_4x(src, 2, 2, dst);
    assert((dst[0] & 0xFF000000) == 0xFF000000);
}

static void test_automap(void) {
    Automap am;
    automap_init(&am);

    assert(!automap_is_visited(&am, 0, 0, 0));
    automap_mark(&am, 0, 5, 10);
    assert(automap_is_visited(&am, 0, 5, 10));
    assert(!automap_is_visited(&am, 0, 5, 11));
    assert(!automap_is_visited(&am, 1, 5, 10));

    assert(!automap_is_visited(&am, -1, 0, 0));
    assert(!automap_is_visited(&am, 0, -1, 0));
    assert(!automap_is_visited(&am, 0, MAP_WIDTH, 0));
}

static void test_reverb(void) {
    int16_t buf[256];
    for (int i = 0; i < 256; i++) buf[i] = (int16_t)(i * 100);
    reverb_process(buf, 256, 0.5f);
    // Should not produce silence
    bool all_zero = true;
    for (int i = 0; i < 256; i++) if (buf[i] != 0) all_zero = false;
    assert(!all_zero);
}

static void test_lighting(void) {
    uint32_t c = 0xFF804020;
    uint32_t shaded = lighting_shade_pixel(c, 0.5f);
    assert(((shaded >> 16) & 0xFF) == 64); // 0x80 * 0.5
    assert(((shaded >> 8) & 0xFF) == 32);  // 0x40 * 0.5
    assert((shaded & 0xFF) == 16);          // 0x20 * 0.5

    uint32_t full = lighting_shade_pixel(c, 1.0f);
    assert(full == c);

    uint32_t dark = lighting_shade_pixel(c, 0.0f);
    assert((dark & 0x00FFFFFF) == 0);

    float i0 = lighting_compute_intensity(0, 0);
    float i5 = lighting_compute_intensity(5, 0);
    assert(i0 > i5);
    assert(i0 <= 1.0f && i0 >= 0.1f);
    assert(i5 <= 1.0f && i5 >= 0.1f);
}

static void test_replay(void) {
    ReplaySystem rs;
    replay_init(&rs);

    rs.recording = true;
    rs.seed = 0xDEADBEEF;

    replay_record_input(&rs, 0, 1, 0);
    replay_record_input(&rs, 5, 2, 1);
    replay_record_input(&rs, 10, 3, 2);
    assert(rs.count == 3);

    const char *path = "/tmp/test_opencaptive_replay.ocrp";
    assert(replay_save(&rs, path));

    ReplaySystem rs2;
    replay_init(&rs2);
    assert(replay_load(&rs2, path));
    assert(rs2.count == 3);
    assert(rs2.seed == 0xDEADBEEF);
    assert(rs2.playing == true);

    const ReplayInput *inp = replay_next(&rs2, 0);
    assert(inp != NULL);
    assert(inp->action == 1);

    inp = replay_next(&rs2, 3);
    assert(inp == NULL);

    inp = replay_next(&rs2, 5);
    assert(inp != NULL);
    assert(inp->action == 2);

    remove(path);
}

static void test_cross_save(void) {
    static GameState gs;
    memset(&gs, 0, sizeof(gs));
    gs.game_type = GAME_CAPTIVE;
    gs.party_x = 10;
    gs.party_y = 20;
    gs.party_dir = DIR_SOUTH;
    gs.current_level = 2;
    gs.mission = 5;
    gs.num_levels = 3;
    gs.gold = 1234;
    gs.mission_seed = 0xCAFEBABE;
    gs.tick = 9999;
    strcpy(gs.droids[0].name, "Alpha");
    gs.droids[0].hp = 100;
    gs.droids[0].hp_max = 200;
    gs.levels[0].seed = 42;
    gs.levels[0].cells[5][10].type = CELL_DOOR;

    const char *path = "/tmp/test_opencaptive_cross.ocsv";
    assert(cross_save_export(&gs, path));

    GameState gs2;
    memset(&gs2, 0, sizeof(gs2));
    assert(cross_save_import(&gs2, path));

    assert(gs2.game_type == GAME_CAPTIVE);
    assert(gs2.party_x == 10);
    assert(gs2.party_y == 20);
    assert(gs2.party_dir == DIR_SOUTH);
    assert(gs2.current_level == 2);
    assert(gs2.mission == 5);
    assert(gs2.gold == 1234);
    assert(gs2.mission_seed == 0xCAFEBABE);
    assert(gs2.tick == 9999);
    assert(strcmp(gs2.droids[0].name, "Alpha") == 0);
    assert(gs2.droids[0].hp == 100);
    assert(gs2.droids[0].hp_max == 200);
    assert(gs2.levels[0].seed == 42);
    assert(gs2.levels[0].cells[5][10].type == CELL_DOOR);

    remove(path);
}

static void test_minimap_render(void) {
    DungeonLevel level;
    memset(&level, 0, sizeof(level));
    level.level = 0;
    level.cells[5][5].type = CELL_FLOOR;
    level.cells[5][6].type = CELL_DOOR;

    CustomFeatures feat;
    custom_features_defaults(&feat);
    feat.minimap = true;
    feat.minimap_size = 32;

    uint32_t fb[320 * 200];
    memset(fb, 0, sizeof(fb));
    minimap_render(fb, 320, 200, &level, 5, 5, DIR_NORTH, NULL, &feat);

    // Check that some pixels in the minimap area were written
    bool has_content = false;
    for (int y = 0; y < 36; y++)
        for (int x = 280; x < 316; x++)
            if (fb[y * 320 + x] != 0) has_content = true;
    assert(has_content);
}

static void test_debug_hud_render(void) {
    static GameState gs;
    memset(&gs, 0, sizeof(gs));
    gs.party_x = 10;
    gs.party_y = 20;
    gs.party_dir = DIR_EAST;
    gs.current_level = 0;
    gs.num_levels = 3;
    gs.tick = 42;
    gs.gold = 500;
    gs.generators_total = 5;
    gs.generators_destroyed = 2;
    gs.droids[0].hp = 100;
    gs.droids[0].hp_max = 200;

    CustomFeatures feat;
    custom_features_defaults(&feat);
    feat.debug_hud = true;

    uint32_t fb[320 * 200];
    memset(fb, 0, sizeof(fb));
    debug_hud_render(fb, 320, 200, &gs, &feat);

    bool has_content = false;
    for (int y = 0; y < 80; y++)
        for (int x = 0; x < 100; x++)
            if (fb[y * 320 + x] != 0) has_content = true;
    assert(has_content);
}

int main(void) {
    test_defaults();
    test_save_load();
    test_upscale_2x();
    test_upscale_3x();
    test_upscale_4x();
    test_automap();
    test_reverb();
    test_lighting();
    test_replay();
    test_cross_save();
    test_minimap_render();
    test_debug_hud_render();
    printf("All custom feature tests passed\n");
    return 0;
}
