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

bool music_init(MusicSystem *mus, SoundSystem *snd, const DataVFS *vfs) {
    memset(mus, 0, sizeof(*mus));
    mus->sound = snd;
    mus->enabled = true;
    mus->vfs = vfs;
    return true;
}

void music_play(MusicSystem *mus, MusicTrack track) {
    if (!mus->enabled || track == mus->current_track) return;

    midi_stop(&mus->player);
    mus->current_track = track;

    if (track == MUSIC_NONE || !track_files[track]) return;
    if (!mus->vfs) return;

    char rel_path[256];
    snprintf(rel_path, sizeof(rel_path), "SOUND/%s", track_files[track]);

    size_t size;
    uint8_t *data = vfs_read_file(mus->vfs, rel_path, &size);
    if (!data) return;

    if (midi_load(&mus->player, data, size)) {
        midi_set_volume(&mus->player, 0.3f);
        midi_play(&mus->player, true);
    }
}

void music_stop(MusicSystem *mus) {
    midi_stop(&mus->player);
    mus->current_track = MUSIC_NONE;
}

void music_update(MusicSystem *mus) {
    if (!mus->enabled || !mus->player.playing) return;

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
