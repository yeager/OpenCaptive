#include "custom_features.h"
#include "game_state.h"
#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static void test_defaults(void) {
    custom_features_defaults(NULL);
    assert(!custom_features_load(NULL, NULL));
    assert(!custom_features_save(NULL, NULL));
    debug_hud_render(NULL, 0, 0, NULL, NULL);
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
    assert(!f.reveal_map);
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
    f1.reveal_map = true;
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
    assert(f2.reveal_map == true);
    assert(f2.dynamic_lighting == true);
    assert(f2.game_speed > 1.9f && f2.game_speed < 2.1f);

    remove(path);

    FILE *spaced = fopen(path, "w");
    assert(spaced);
    fputs(" minimap = 1\n", spaced);
    fputs(" game_speed = 2.0\n", spaced);
    assert(fclose(spaced) == 0);
    custom_features_defaults(&f2);
    assert(custom_features_load(&f2, path));
    assert(f2.minimap);
    assert(f2.game_speed > 1.9f && f2.game_speed < 2.1f);
    remove(path);

    FILE *malformed = fopen(path, "w");
    assert(malformed);
    fputs("upscale_factor=999999999999999999999999\n", malformed);
    fputs("game_speed=nan\n", malformed);
    fputs("minimap_opacity=2\n", malformed);
    fputs("minimap_size=-1\n", malformed);
    fputs("widescreen_width=999999\n", malformed);
    fputs("audio_sample_rate=12345\n", malformed);
    assert(fclose(malformed) == 0);
    custom_features_defaults(&f2);
    assert(custom_features_load(&f2, path));
    assert(f2.upscale_factor == 2);
    assert(f2.game_speed > 0.9f && f2.game_speed < 1.1f);
    assert(f2.minimap_opacity > 0.5f && f2.minimap_opacity < 0.7f);
    assert(f2.minimap_size == 96);
    assert(f2.widescreen_width == 0);
    assert(f2.audio_sample_rate == 44100);
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

static void test_upscale_rejects_invalid_buffers(void) {
    uint32_t src[4] = {1, 2, 3, 4};
    uint32_t dst[16];
    memset(dst, 0xA5, sizeof(dst));

    upscale_xbrz_2x(NULL, 2, 2, dst);
    upscale_xbrz_3x(src, 0, 2, dst);
    upscale_xbrz_4x(src, 2, 0, dst);
    upscale_xbrz_2x(src, 2, 2, NULL);
    upscale_xbrz_2x(src, INT_MAX, 2, dst);
    upscale_xbrz_3x(src, INT_MAX / 3 + 1, 2, dst);
    upscale_xbrz_4x(src, 2, INT_MAX / 4 + 1, dst);

    for (size_t i = 0; i < sizeof(dst) / sizeof(dst[0]); ++i)
        assert(dst[i] == 0xA5A5A5A5u);
}

static void test_automap(void) {
    Automap am;
    automap_init(NULL);
    automap_mark(NULL, 0, 0, 0);
    assert(!automap_is_visited(NULL, 0, 0, 0));
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
    reverb_process(NULL, 16, 0.5f);
    int16_t buf[256];
    for (int i = 0; i < 256; i++) buf[i] = (int16_t)(i * 100);
    reverb_process(buf, 256, 0.5f);
    // Should not produce silence
    bool all_zero = true;
    for (int i = 0; i < 256; i++) if (buf[i] != 0) all_zero = false;
    assert(!all_zero);

    int16_t before_nan[4] = {100, -100, 200, -200};
    int16_t after_nan[4];
    memcpy(after_nan, before_nan, sizeof(before_nan));
    reverb_process(after_nan, 4, NAN);
    assert(memcmp(after_nan, before_nan, sizeof(before_nan)) == 0);
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
    uint32_t nan_shaded = lighting_shade_pixel(c, NAN);
    assert((nan_shaded & 0x00FFFFFF) == 0);

    float i0 = lighting_compute_intensity(0, 0);
    float i5 = lighting_compute_intensity(5, 0);
    assert(i0 > i5);
    assert(i0 <= 1.0f && i0 >= 0.1f);
    assert(i5 <= 1.0f && i5 >= 0.1f);
    float extreme = lighting_compute_intensity(0, INT_MIN);
    assert(extreme >= 0.1f && extreme <= 1.0f);
}

static void write_test_u32_le(FILE *fp, uint32_t value) {
    uint8_t bytes[4] = {
        (uint8_t)value, (uint8_t)(value >> 8),
        (uint8_t)(value >> 16), (uint8_t)(value >> 24)
    };
    assert(fwrite(bytes, 1, sizeof(bytes), fp) == sizeof(bytes));
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

    FILE *header = fopen(path, "rb");
    assert(header);
    uint8_t header_bytes[16];
    assert(fread(header_bytes, 1, sizeof(header_bytes), header) == sizeof(header_bytes));
    assert(fclose(header) == 0);
    assert(header_bytes[0] == 0x50 && header_bytes[1] == 0x52 &&
           header_bytes[2] == 0x43 && header_bytes[3] == 0x4F);
    assert(header_bytes[4] == 1 && header_bytes[5] == 0 &&
           header_bytes[6] == 0 && header_bytes[7] == 0);

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

    FILE *truncated = fopen(path, "wb");
    assert(truncated);
    uint32_t replay_seed = 0xCAFED00D;
    uint32_t replay_count = 3;
    write_test_u32_le(truncated, 0x4F435250);
    write_test_u32_le(truncated, 1);
    write_test_u32_le(truncated, replay_seed);
    write_test_u32_le(truncated, replay_count);
    assert(fclose(truncated) == 0);

    ReplaySystem preserved;
    replay_init(&preserved);
    preserved.seed = 0x12345678;
    preserved.count = 7;
    assert(!replay_load(&preserved, path));
    assert(preserved.seed == 0x12345678 && preserved.count == 7);

    FILE *unordered = fopen(path, "wb");
    assert(unordered);
    replay_count = 2;
    write_test_u32_le(unordered, 0x4F435250);
    write_test_u32_le(unordered, 1);
    write_test_u32_le(unordered, replay_seed);
    write_test_u32_le(unordered, replay_count);
    uint32_t late_tick = 10, early_tick = 5;
    uint8_t action = 1, param = 0;
    write_test_u32_le(unordered, late_tick);
    assert(fwrite(&action, 1, 1, unordered) == 1);
    assert(fwrite(&param, 1, 1, unordered) == 1);
    write_test_u32_le(unordered, early_tick);
    assert(fwrite(&action, 1, 1, unordered) == 1);
    assert(fwrite(&param, 1, 1, unordered) == 1);
    assert(fclose(unordered) == 0);
    assert(!replay_load(&preserved, path));
    assert(preserved.seed == 0x12345678 && preserved.count == 7);

    ReplaySystem unordered_state;
    replay_init(&unordered_state);
    unordered_state.count = 2;
    unordered_state.inputs[0].tick = 10;
    unordered_state.inputs[1].tick = 5;
    assert(!replay_save(&unordered_state, path));

    assert(replay_save(&rs, path));
    FILE *trailing = fopen(path, "ab");
    assert(trailing && fputc(0xA5, trailing) != EOF);
    assert(fclose(trailing) == 0);
    assert(!replay_load(&preserved, path));
    assert(preserved.seed == 0x12345678 && preserved.count == 7);

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
    for (int level = 0; level < gs.num_levels; level++)
        gs.levels[level].level = level;
    strcpy(gs.droids[0].name, "Alpha");
    gs.droids[0].hp = 100;
    gs.droids[0].hp_max = 200;
    gs.droids[0].body_part_hp[0] = 37;
    gs.levels[0].seed = 42;
    gs.levels[0].cells[5][10].type = CELL_DOOR;

    assert(!cross_save_export(&gs, "/"));

    const char *path = "/tmp/test_opencaptive_cross.ocsv";
    assert(cross_save_export(&gs, path));

    FILE *header = fopen(path, "rb");
    assert(header);
    uint8_t header_bytes[8];
    assert(fread(header_bytes, 1, sizeof(header_bytes), header) == sizeof(header_bytes));
    assert(fclose(header) == 0);
    assert(header_bytes[0] == 0x56 && header_bytes[1] == 0x53 &&
           header_bytes[2] == 0x43 && header_bytes[3] == 0x4F);
    assert(header_bytes[4] == 2 && header_bytes[5] == 0 &&
           header_bytes[6] == 0 && header_bytes[7] == 0);

    GameState gs2;
    memset(&gs2, 0, sizeof(gs2));
    static const char data_path[] = "/tmp/opencaptive-cross-data";
    gs2.config.render_mode = CAPTIVE_RENDER_ENHANCED;
    gs2.config.scanlines = true;
    gs2.config.brightness = 81;
    gs2.config.data_path = data_path;
    assert(cross_save_import(&gs2, path));

    assert(gs2.game_type == GAME_CAPTIVE);
    assert(gs2.mode == STATE_GAME);
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
    assert(gs2.droids[0].body_part_hp[0] == 37);
    assert(gs2.levels[0].seed == 42);
    assert(gs2.levels[0].cells[5][10].type == CELL_DOOR);
    assert(gs2.config.render_mode == CAPTIVE_RENDER_ENHANCED);
    assert(gs2.config.scanlines);
    assert(gs2.config.brightness == 81);
    assert(gs2.config.data_path == data_path);

    /* A level record must retain its array index; accepting a mismatched
       level ID would leave a cross-save with internally inconsistent maps. */
    assert(cross_save_export(&gs, path));
    FILE *bad_level = fopen(path, "r+b");
    assert(bad_level);
    assert(fseek(bad_level, 57L + 4L * 64L, SEEK_SET) == 0);
    uint8_t wrong_level[4] = {1, 0, 0, 0};
    assert(fwrite(wrong_level, 1, sizeof(wrong_level), bad_level) == sizeof(wrong_level));
    assert(fclose(bad_level) == 0);
    assert(!cross_save_import(&gs2, path));
    assert(cross_save_export(&gs, path));

    /* Build a v1 fixture from the v2 export by removing the six-byte armor
       condition field that the old format did not contain. */
    const char *legacy_path = "/tmp/test_opencaptive_cross_v1.ocsv";
    FILE *v2_file = fopen(path, "rb");
    assert(v2_file);
    assert(fseek(v2_file, 0, SEEK_END) == 0);
    long v2_size = ftell(v2_file);
    assert(v2_size > 0 && fseek(v2_file, 0, SEEK_SET) == 0);
    uint8_t *v2_bytes = malloc((size_t)v2_size);
    assert(v2_bytes);
    assert(fread(v2_bytes, 1, (size_t)v2_size, v2_file) == (size_t)v2_size);
    assert(fclose(v2_file) == 0);
    const size_t cross_header_size = 57U;
    const size_t v2_droid_size = 64U;
    const size_t v1_droid_size = 58U;
    const size_t droid_count = 4U;
    const size_t removed_size = (v2_droid_size - v1_droid_size) * droid_count;
    assert((size_t)v2_size > cross_header_size + removed_size);
    size_t v1_size = (size_t)v2_size - removed_size;
    uint8_t *v1_bytes = malloc(v1_size);
    assert(v1_bytes);
    memcpy(v1_bytes, v2_bytes, cross_header_size);
    for (size_t droid = 0; droid < droid_count; ++droid) {
        size_t v2_offset = cross_header_size + droid * v2_droid_size;
        size_t v1_offset = cross_header_size + droid * v1_droid_size;
        memcpy(v1_bytes + v1_offset, v2_bytes + v2_offset, 30U);
        memcpy(v1_bytes + v1_offset + 30U, v2_bytes + v2_offset + 36U, 28U);
    }
    size_t v2_level_offset = cross_header_size + droid_count * v2_droid_size;
    size_t v1_level_offset = cross_header_size + droid_count * v1_droid_size;
    memcpy(v1_bytes + v1_level_offset, v2_bytes + v2_level_offset,
           (size_t)v2_size - v2_level_offset);
    v1_bytes[4] = 1;
    FILE *v1_file = fopen(legacy_path, "wb");
    assert(v1_file);
    assert(fwrite(v1_bytes, 1, v1_size, v1_file) == v1_size);
    assert(fclose(v1_file) == 0);
    free(v1_bytes);
    free(v2_bytes);

    GameState legacy;
    memset(&legacy, 0, sizeof(legacy));
    assert(cross_save_import(&legacy, legacy_path));
    assert(legacy.droids[0].hp == 100);
    assert(legacy.droids[0].body_part_hp[0] == UINT8_MAX);
    remove(legacy_path);

    gs.game_type = GAME_LIBERATION;
    assert(!cross_save_export(&gs, path));
    gs.game_type = GAME_CAPTIVE;

    gs.droids[0].hp = gs.droids[0].hp_max + 1;
    assert(!cross_save_export(&gs, path));
    gs.droids[0].hp = 100;
    gs.levels[0].cells[5][10].type = (CellType)255;
    assert(!cross_save_export(&gs, path));
    gs.levels[0].cells[5][10].type = CELL_DOOR;

    GameState preserved;
    memset(&preserved, 0, sizeof(preserved));
    preserved.mission = 77;
    preserved.party_x = 31;

    assert(cross_save_export(&gs, path));
    FILE *trailing_cross = fopen(path, "ab");
    assert(trailing_cross && fputc(0xA5, trailing_cross) != EOF);
    assert(fclose(trailing_cross) == 0);
    assert(!cross_save_import(&preserved, path));

    gs.game_type = (GameType)-1;
    assert(!cross_save_export(&gs, path));
    gs.game_type = GAME_CAPTIVE;

    FILE *mutated = fopen(path, "r+b");
    assert(mutated);
    const uint8_t invalid[4] = { 0xFF, 0xFF, 0xFF, 0xFF };
    assert(fseek(mutated, 33, SEEK_SET) == 0);
    assert(fwrite(invalid, 1, sizeof(invalid), mutated) == sizeof(invalid));
    assert(fclose(mutated) == 0);
    assert(!cross_save_import(&preserved, path));

    assert(cross_save_export(&gs, path));
    mutated = fopen(path, "r+b");
    assert(mutated);
    /* Header (57 bytes) + four 64-byte v2 droid records. */
    assert(fseek(mutated, 57 + 4 * 64, SEEK_SET) == 0);
    assert(fwrite(invalid, 1, sizeof(invalid), mutated) == sizeof(invalid));
    assert(fclose(mutated) == 0);
    assert(!cross_save_import(&preserved, path));

    assert(cross_save_export(&gs, path));
    mutated = fopen(path, "r+b");
    assert(mutated);
    assert(fseek(mutated, 45, SEEK_SET) == 0);
    assert(fwrite(invalid, 1, sizeof(invalid), mutated) == sizeof(invalid));
    assert(fclose(mutated) == 0);
    assert(!cross_save_import(&preserved, path));

    FILE *corrupt = fopen(path, "r+b");
    assert(corrupt);
    assert(fseek(corrupt, -10, SEEK_END) == 0);
    assert(fputc(0xFF, corrupt) != EOF);
    assert(fclose(corrupt) == 0);

    assert(!cross_save_import(&preserved, path));
    assert(preserved.mission == 77 && preserved.party_x == 31);

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
    feat.minimap_opacity = NAN;
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
    test_upscale_rejects_invalid_buffers();
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
