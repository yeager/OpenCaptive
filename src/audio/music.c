#include "music.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *track_files[] = {
    [MUSIC_NONE]    = NULL,
    [MUSIC_TITLE]   = "MAIN2.MID",
    [MUSIC_BASE]    = "GENBASE.MID",
    [MUSIC_BATTLE]  = "BATT1.MID",
    [MUSIC_SHOP]    = "SHOPKEEP.MID",
    [MUSIC_HOLOMAP] = "HOLOMAP.MID",
    [MUSIC_ESCAPE]  = "ESCAPED.MID",
    [MUSIC_FINAL]   = "FINAL2.MID",
    [MUSIC_TRAPPED] = "TRAPPED.MID",
};

bool music_init(MusicSystem *mus, SoundSystem *snd, const char *data_path) {
    memset(mus, 0, sizeof(*mus));
    mus->sound = snd;
    mus->enabled = true;
    if (data_path)
        strncpy(mus->data_path, data_path, sizeof(mus->data_path) - 1);
    return true;
}

void music_play(MusicSystem *mus, MusicTrack track) {
    if (!mus->enabled || track == mus->current_track) return;

    midi_stop(&mus->player);
    mus->current_track = track;

    if (track == MUSIC_NONE || !track_files[track]) return;

    char path[1024];
    snprintf(path, sizeof(path), "%s/SOUND/%s", mus->data_path, track_files[track]);

    FILE *f = fopen(path, "rb");
    if (!f) return;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    uint8_t *data = malloc(size);
    if (!data) { fclose(f); return; }
    fread(data, 1, size, f);
    fclose(f);

    if (midi_load(&mus->player, data, size)) {
        midi_set_volume(&mus->player, 0.3f);
        midi_play(&mus->player, true);
    }
    // Note: data must persist while playing — leaking intentionally for now
    // TODO: track allocation and free on stop
}

void music_stop(MusicSystem *mus) {
    midi_stop(&mus->player);
    mus->current_track = MUSIC_NONE;
}

void music_update(MusicSystem *mus) {
    if (!mus->enabled || !mus->player.playing) return;

    // Render MIDI to the sound system's audio stream
    int16_t buffer[1024];
    midi_render(&mus->player, buffer, 1024);

    if (mus->sound && mus->sound->stream) {
        SDL_PutAudioStreamData(mus->sound->stream, buffer, sizeof(buffer));
    }
}

void music_shutdown(MusicSystem *mus) {
    midi_stop(&mus->player);
    memset(mus, 0, sizeof(*mus));
}
