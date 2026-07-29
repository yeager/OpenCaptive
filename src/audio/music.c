#include "music.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *track_hashes[] = {
    [MUSIC_NONE]    = NULL,
    [MUSIC_TITLE]   = "ef9ec6b8fac6710c99f9ed037dfdb2767a3e20bc9259789ced977d322d3420be",
    [MUSIC_BASE]    = "150db09bf3ba0914501b9be1353c21916a597e89ef711f7b2dff4989313f2810",
    [MUSIC_BATTLE]  = "a4b8a8fdef37602e732af3435b0ade18219de456e5563e0229e0d9f1c5132e6f",
    [MUSIC_SHOP]    = "dfe6b899e1e499d3c9a326c4554c8da3e7e83d395b1d13c81341d2688c50a0bc",
    [MUSIC_HOLOMAP] = "7be1dc97c004fc04f1ead07a2957c6e2a1b97ddf48dafa998964f74db816757f",
    [MUSIC_ESCAPE]  = "2ddacc25ece9e3e6bdd13e3f1e7f926bfce2e180daf0fce8d57a3beb5ec30d1a",
    [MUSIC_FINAL]   = "d8cb990243dcdb885881f09a0bbe0788caee3c7b6ec9f62ed5d70c4c1d411587",
    [MUSIC_TRAPPED] = "8c1ad7905a95dacb8a57d9f34d97beeb8eb42a4f38a2188909d6c769ebdbab2d",
};

bool music_init(MusicSystem *mus, SoundSystem *snd, const DataVFS *vfs) {
    memset(mus, 0, sizeof(*mus));
    mus->sound = snd;
    mus->enabled = true;
    mus->vfs = vfs;
    return true;
}

void music_set_enabled(MusicSystem *mus, bool enabled) {
    if (!mus || mus->enabled == enabled) return;
    mus->enabled = enabled;
    if (!enabled) {
        midi_stop(&mus->player);
        free(mus->owned_data);
        mus->owned_data = NULL;
        mus->current_track = MUSIC_NONE;
    } else if (mus->requested_track != MUSIC_NONE) {
        music_play(mus, mus->requested_track);
    }
}

void music_play(MusicSystem *mus, MusicTrack track) {
    if (!mus) return;
    mus->requested_track = track;
    if (!mus->enabled || track == mus->current_track) return;

    midi_stop(&mus->player);
    free(mus->owned_data);
    mus->owned_data = NULL;
    mus->current_track = track;

    if (track == MUSIC_NONE || !track_hashes[track]) return;
    if (!mus->vfs) return;

    size_t size;
    uint8_t *data = vfs_find_sha256(mus->vfs, track_hashes[track], &size);
    if (!data) return;

    if (midi_load(&mus->player, data, size)) {
        /* MIDI tracks retain pointers into the source buffer while playing. */
        mus->owned_data = data;
        midi_set_volume(&mus->player, 0.3f);
        midi_play(&mus->player, true);
    } else {
        free(data);
    }
}

void music_stop(MusicSystem *mus) {
    midi_stop(&mus->player);
    free(mus->owned_data);
    mus->owned_data = NULL;
    mus->current_track = MUSIC_NONE;
    mus->requested_track = MUSIC_NONE;
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
