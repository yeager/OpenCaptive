#include "custom_features.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void custom_features_defaults(CustomFeatures *f) {
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
    FILE *fp = fopen(path, "r");
    if (!fp) return false;

    custom_features_defaults(f);

    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        char key[64];
        char val[64];
        if (sscanf(line, " %63[^=]= %63s", key, val) != 2) continue;

        if (strcmp(key, "hd_upscale") == 0) f->hd_upscale = atoi(val) != 0;
        else if (strcmp(key, "upscale_factor") == 0) f->upscale_factor = atoi(val);
        else if (strcmp(key, "widescreen") == 0) f->widescreen = atoi(val) != 0;
        else if (strcmp(key, "widescreen_width") == 0) f->widescreen_width = atoi(val);
        else if (strcmp(key, "quicksave") == 0) f->quicksave = atoi(val) != 0;
        else if (strcmp(key, "minimap") == 0) f->minimap = atoi(val) != 0;
        else if (strcmp(key, "minimap_opacity") == 0) f->minimap_opacity = (float)atof(val);
        else if (strcmp(key, "minimap_size") == 0) f->minimap_size = atoi(val);
        else if (strcmp(key, "speed_control") == 0) f->speed_control = atoi(val) != 0;
        else if (strcmp(key, "game_speed") == 0) f->game_speed = (float)atof(val);
        else if (strcmp(key, "fast_travel") == 0) f->fast_travel = atoi(val) != 0;
        else if (strcmp(key, "mouse_look") == 0) f->mouse_look = atoi(val) != 0;
        else if (strcmp(key, "mouse_sensitivity") == 0) f->mouse_sensitivity = (float)atof(val);
        else if (strcmp(key, "debug_hud") == 0) f->debug_hud = atoi(val) != 0;
        else if (strcmp(key, "audio_reverb") == 0) f->audio_reverb = atoi(val) != 0;
        else if (strcmp(key, "reverb_amount") == 0) f->reverb_amount = (float)atof(val);
        else if (strcmp(key, "hq_midi") == 0) f->hq_midi = atoi(val) != 0;
        else if (strcmp(key, "audio_sample_rate") == 0) f->audio_sample_rate = atoi(val);
        else if (strcmp(key, "automap") == 0) f->automap = atoi(val) != 0;
        else if (strcmp(key, "cross_save") == 0) f->cross_save = atoi(val) != 0;
        else if (strcmp(key, "replay_record") == 0) f->replay_record = atoi(val) != 0;
        else if (strcmp(key, "texture_filter") == 0) f->texture_filter = atoi(val) != 0;
        else if (strcmp(key, "dynamic_lighting") == 0) f->dynamic_lighting = atoi(val) != 0;
    }

    fclose(fp);
    return true;
}

bool custom_features_save(const CustomFeatures *f, const char *path) {
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

    fclose(fp);
    return true;
}
