#include "custom_features.h"
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int parse_int_or_default(const char *text, int fallback) {
    if (!text || !text[0]) return fallback;
    char *end = NULL;
    errno = 0;
    long value = strtol(text, &end, 10);
    if (errno == ERANGE || end == text || *end != '\0' ||
        value < INT_MIN || value > INT_MAX) return fallback;
    return (int)value;
}

static float parse_float_or_default(const char *text, float fallback) {
    if (!text || !text[0]) return fallback;
    char *end = NULL;
    errno = 0;
    float value = strtof(text, &end);
    if (errno == ERANGE || end == text || *end != '\0' || !isfinite(value)) return fallback;
    return value;
}

void custom_features_defaults(CustomFeatures *f) {
    if (!f) return;
    memset(f, 0, sizeof(*f));
    f->upscale_factor = 2;
    f->minimap_opacity = 0.6f;
    f->minimap_size = 96;
    f->game_speed = 1.0f;
    f->mouse_sensitivity = 1.0f;
    f->reverb_amount = 0.3f;
    f->audio_sample_rate = 44100;
}

bool custom_features_load(CustomFeatures *f, const char *path) {
    if (!f || !path) return false;
    CustomFeatures *destination = f;
    CustomFeatures restored;
    custom_features_defaults(&restored);
    f = &restored;
    FILE *fp = fopen(path, "r");
    if (!fp) return false;

    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        char key[64];
        char val[64];
        if (sscanf(line, " %63[^=]= %63s", key, val) != 2) continue;

        if (strcmp(key, "hd_upscale") == 0) f->hd_upscale = parse_int_or_default(val, 0) != 0;
        else if (strcmp(key, "upscale_factor") == 0) f->upscale_factor = parse_int_or_default(val, f->upscale_factor);
        else if (strcmp(key, "widescreen") == 0) f->widescreen = parse_int_or_default(val, 0) != 0;
        else if (strcmp(key, "widescreen_width") == 0) f->widescreen_width = parse_int_or_default(val, f->widescreen_width);
        else if (strcmp(key, "quicksave") == 0) f->quicksave = parse_int_or_default(val, 0) != 0;
        else if (strcmp(key, "minimap") == 0) f->minimap = parse_int_or_default(val, 0) != 0;
        else if (strcmp(key, "minimap_opacity") == 0) f->minimap_opacity = parse_float_or_default(val, f->minimap_opacity);
        else if (strcmp(key, "minimap_size") == 0) f->minimap_size = parse_int_or_default(val, f->minimap_size);
        else if (strcmp(key, "speed_control") == 0) f->speed_control = parse_int_or_default(val, 0) != 0;
        else if (strcmp(key, "game_speed") == 0) f->game_speed = parse_float_or_default(val, f->game_speed);
        else if (strcmp(key, "fast_travel") == 0) f->fast_travel = parse_int_or_default(val, 0) != 0;
        else if (strcmp(key, "mouse_look") == 0) f->mouse_look = parse_int_or_default(val, 0) != 0;
        else if (strcmp(key, "mouse_sensitivity") == 0) f->mouse_sensitivity = parse_float_or_default(val, f->mouse_sensitivity);
        else if (strcmp(key, "debug_hud") == 0) f->debug_hud = parse_int_or_default(val, 0) != 0;
        else if (strcmp(key, "audio_reverb") == 0) f->audio_reverb = parse_int_or_default(val, 0) != 0;
        else if (strcmp(key, "reverb_amount") == 0) f->reverb_amount = parse_float_or_default(val, f->reverb_amount);
        else if (strcmp(key, "hq_midi") == 0) f->hq_midi = parse_int_or_default(val, 0) != 0;
        else if (strcmp(key, "audio_sample_rate") == 0) f->audio_sample_rate = parse_int_or_default(val, f->audio_sample_rate);
        else if (strcmp(key, "automap") == 0) f->automap = parse_int_or_default(val, 0) != 0;
        else if (strcmp(key, "cross_save") == 0) f->cross_save = parse_int_or_default(val, 0) != 0;
        else if (strcmp(key, "replay_record") == 0) f->replay_record = parse_int_or_default(val, 0) != 0;
        else if (strcmp(key, "texture_filter") == 0) f->texture_filter = parse_int_or_default(val, 0) != 0;
        else if (strcmp(key, "dynamic_lighting") == 0) f->dynamic_lighting = parse_int_or_default(val, 0) != 0;
    }

    bool read_ok = !ferror(fp);
    int close_result = fclose(fp);
    bool ok = read_ok && close_result == 0;
    if (ok) {
        /* Configuration files are user-editable. Keep values within the
         * ranges consumed by the renderer, mixer and menu; parsing a valid
         * number must not make a later subsystem receive an invalid one. */
        if (restored.upscale_factor < 2 || restored.upscale_factor > 4)
            restored.upscale_factor = 2;
        if (restored.widescreen_width < 0)
            restored.widescreen_width = 0;
        if (restored.minimap_opacity < 0.0f || restored.minimap_opacity > 1.0f)
            restored.minimap_opacity = 0.6f;
        if (restored.minimap_size < 32 || restored.minimap_size > 192)
            restored.minimap_size = 96;
        if (restored.game_speed < 0.25f || restored.game_speed > 4.0f)
            restored.game_speed = 1.0f;
        if (restored.mouse_sensitivity < 1.0f || restored.mouse_sensitivity > 10.0f)
            restored.mouse_sensitivity = 1.0f;
        if (restored.reverb_amount < 0.0f || restored.reverb_amount > 1.0f)
            restored.reverb_amount = 0.3f;
        if (restored.audio_sample_rate != 22050 &&
            restored.audio_sample_rate != 44100 &&
            restored.audio_sample_rate != 48000)
            restored.audio_sample_rate = 44100;
        *destination = restored;
    }
    return ok;
}

bool custom_features_save(const CustomFeatures *f, const char *path) {
    if (!f || !path) return false;
    FILE *fp = fopen(path, "w");
    if (!fp) return false;

    fprintf(fp, "hd_upscale=%d\n", f->hd_upscale);
    fprintf(fp, "upscale_factor=%d\n", f->upscale_factor);
    fprintf(fp, "widescreen=%d\n", f->widescreen);
    fprintf(fp, "widescreen_width=%d\n", f->widescreen_width);
    fprintf(fp, "quicksave=%d\n", f->quicksave);
    fprintf(fp, "minimap=%d\n", f->minimap);
    fprintf(fp, "minimap_opacity=%.2f\n", f->minimap_opacity);
    fprintf(fp, "minimap_size=%d\n", f->minimap_size);
    fprintf(fp, "speed_control=%d\n", f->speed_control);
    fprintf(fp, "game_speed=%.2f\n", f->game_speed);
    fprintf(fp, "fast_travel=%d\n", f->fast_travel);
    fprintf(fp, "mouse_look=%d\n", f->mouse_look);
    fprintf(fp, "mouse_sensitivity=%.2f\n", f->mouse_sensitivity);
    fprintf(fp, "debug_hud=%d\n", f->debug_hud);
    fprintf(fp, "audio_reverb=%d\n", f->audio_reverb);
    fprintf(fp, "reverb_amount=%.2f\n", f->reverb_amount);
    fprintf(fp, "hq_midi=%d\n", f->hq_midi);
    fprintf(fp, "audio_sample_rate=%d\n", f->audio_sample_rate);
    fprintf(fp, "automap=%d\n", f->automap);
    fprintf(fp, "cross_save=%d\n", f->cross_save);
    fprintf(fp, "replay_record=%d\n", f->replay_record);
    fprintf(fp, "texture_filter=%d\n", f->texture_filter);
    fprintf(fp, "dynamic_lighting=%d\n", f->dynamic_lighting);

    bool write_ok = !ferror(fp);
    int close_result = fclose(fp);
    return write_ok && close_result == 0;
}
