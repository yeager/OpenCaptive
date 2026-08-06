#include "music.h"
#include "captive_data.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* SHA-256 hashes of all 63 MIDI files from the DOS release.
 * Categories with multiple variants are selected randomly using the game PRNG,
 * matching original behavior. */

typedef struct {
    const char *hash;
} TrackVariant;

typedef struct {
    const TrackVariant *variants;
    int count;
} TrackCategory;

static const TrackVariant battle_variants[] = {
    {"a4b8a8fdef37602e732af3435b0ade18219de456e5563e0229e0d9f1c5132e6f"},
    {"2e87ad1e558769396021490ecbd32f7397b6a83706a5f9c092791e12e0a43cb6"},
    {"c4518d8d2c9e9f7067f1b652a7abd477ce3f8481d2644b3c8acc3c6a63d23215"},
    {"dd3ea6ba9ad1c02cf03f9677ec018de0c73ffad3b595e7aa5970e4042daf1e7f"},
    {"954d1888d68ecd31f85be328f5ceda892e3ddcfee83141fe7f6a4a62b3653bcc"},
    {"0c0c7cacbc3451af9e3f8e486de4d871db54368896e741e92193ca5e95521c9f"},
    {"8250807c02dc0ba728e4b0a62dbfada4c55808ce8babe0c604c8d6a25cadd8e7"},
    {"dcb5fe4fed1b5805c497f4bff01e53c3bba29c550a053e42bf5dc6d57e9e81b0"},
    {"95e6b5d502b112abcbcb8dbe09b80792f108d60b84328a86ea1196f4475840f4"},
    {"a9d1cf0fa350cdf303be4e515b9210887fd13fc87f9faa1be3043132a3e4cf17"},
    {"0559a3d99499fcc28613fc422fdc514810e7d541a8302a93a5910c1d9f213495"},
};

static const TrackVariant fcbase_variants[] = {
    {"fae1e13e736cbd33a52deb82d5fdeba36ff747b8be20db22c79ee86ad2dbfb9d"},
    {"a8daccde837d7abacb362c1c881c8f1f9c373bdf1df56bb6bf1c6101d43f693f"},
    {"444d4db3e65197e2867bfd2006efbe63d26593a97bad435dd4dd5bd34e915b46"},
    {"c88ae9aa8a2811e85b5c29e4762f61f58ba778ebf7b1f7c866f3f3c1b5e7b375"},
    {"5421c5e41b1960a3aa0d3e06fd0bcb5e0c2ee451d72edff8d075e5c48db23348"},
    {"4b48150f8846580a6f0b05625b9d77501c8259051b9553f7709da564c3107898"},
    {"12e6b639b6471ba82470ef233a7835a5c140ffb42916e351fc07aa69b9be8b6c"},
    {"d68ac713e898990fbb363026e50a35c8b4a34b4fb1266f35a6aebd5f8727c927"},
    {"741898b98d906b8fccfd40dffe18b14e28414d4adfcba39c2d9ab67ac1f7e98d"},
    {"06f10accc26816a39ed1cd0717f97aa4ce89116ffdaffc8b56f286036fb3f408"},
    {"97c4d36f20daf3bd52070e8eb959d796f7c5a54c878e207e1ad178556125dc46"},
};

static const TrackVariant vcbase_variants[] = {
    {"a4abbaebff0075bc31162e137ae9407bad7758f599eed8b7a00ca25b101b3ecd"},
    {"20c9de37403f8875528f4f110d1428b45632c651ba6e81d3093f8f6a44dc6ed6"},
    {"f27abc0a513339678623edc92263a7c2d0e86b319f72c4e983da4130caffddf6"},
    {"4ffaa023ba81551f256e52695c8657d72e2691c2bfce95af1c6b9099c91ebf61"},
    {"f60449540412a25d8dd79987d274f5a86466778a9e1418e64f039bdf28e030a0"},
    {"876eebaa640b91e50c38873d1bb2ca4255bd1c1ff7e3a1e0697e1c4f6fcff221"},
    {"6b7e598c36a0a0047d08ccdf6b4583ed6d0ba261c6675e3c65073110e80bde63"},
    {"ee386c45a5f57fddd8ade3efac549b9005f9e58387e3e9e5eb977cf0ce5c1ad6"},
    {"b360f41bcaae28984fb6a8a9f24f81ec2e36c240c1a402c1cd14d2f32276856f"},
    {"a3674cef974ac0cdab16f8bb88b50c8d568035fa2acf7bbda13db809c9afcdb0"},
    {"bf3eefaae7aca849e1e4bf56da31cebc00e5bda37f1525590cb22c1288f56b4a"},
};

static const TrackVariant longnt_variants[] = {
    {"f3a58bbce3dbfbc664a536277006a5ffd68d1a7782b5d0fb0f75bdf787e9d970"},
    {"0118b352d0aef8a89e51158f89c9e54c6a68e19dd1e839013a3aca7103d81b85"},
    {"086dc9f73a929a2ee9573cc6c02804f942e2b8fa1f4b3e767f7cd49decf8bfdb"},
    {"b4427ba9efb025b1ff673a5498d561dbce8a67e4b16dc4e44d2b625adcfaf778"},
    {"b2b0a1796f85df3372d858c2f3d0e19c62bcd96a4a24564838bf68b6e5cab106"},
    {"6ceb60823bca99d228d353adb0c3d99d61b5902f1e55f88d93168bd4e180fbf9"},
    {"e972e163c3d6e7c5a23ac2297ab5c430d5c3d80d4fbfcccc11256a74b747357f"},
    {"9646f1b1ee0ea9c2f0b348524a53f016632442dcc693531ac271c263000669bf"},
    {"fd4e6079bb17cff54628cdb53e229af798d4595292ff7c253788a6c4263140d4"},
    {"d43c6df8efe7223f884624a6ffe605e13828b7a80d0bcfdc4ddab83b2971133e"},
    {"633953cd3d9dc45a3b8556004e3812937d7cac73f1c56376f1fd4326c8244b0f"},
};

static const TrackVariant w_variants[] = {
    {"40a0885758aaaef87cdc34e4ec78d9e20a72e318f15d8e94060c3e8d1f16fdec"},
    {"ba753add264cd7127b0333ab619ab044b190cc2d2857bb13af94b070fc092e21"},
    {"f76aabdc5f6a1dc0306456fafad889ae31a4106c7406a77549984413afe356a0"},
    {"a1b73dce47ff3752a5a676bc63bd2bb1b50333a6697a40e02618458bd2b7a578"},
    {"ed82ee74bf0b9d0987e9398b76b9a9bec073b3fee54d3765c09e2621563f0a7c"},
    {"682a379b693990ffe50820bad71cc3396f5724e0c3a04f3d0ae1740210ad6bec"},
    {"0c50f8ccce6895b79c2853cf293512846eb959599395d4f3935718fa8429af11"},
    {"0addca6c4db7c425c3dbe66131b62ea942aeb06909a718d39691a5cf189f33af"},
    {"948c4595192e04d00d964dc881710bd923eba3cba040db0f2030e8ed708d34e7"},
    {"a89cfc29287f1a4f1844a4a8bade6d3a296443785416769be96a84cae6d096a6"},
    {"b9bbf82846547954bc667120b8085bdf2d10c88d357820833b4c21056f693731"},
};

static const TrackVariant title_variant[] = {
    {"ef9ec6b8fac6710c99f9ed037dfdb2767a3e20bc9259789ced977d322d3420be"},
};
static const TrackVariant genbase_variant[] = {
    {"150db09bf3ba0914501b9be1353c21916a597e89ef711f7b2dff4989313f2810"},
};
static const TrackVariant shop_variant[] = {
    {"dfe6b899e1e499d3c9a326c4554c8da3e7e83d395b1d13c81341d2688c50a0bc"},
};
static const TrackVariant holomap_variant[] = {
    {"7be1dc97c004fc04f1ead07a2957c6e2a1b97ddf48dafa998964f74db816757f"},
};
static const TrackVariant escaped_variant[] = {
    {"2ddacc25ece9e3e6bdd13e3f1e7f926bfce2e180daf0fce8d57a3beb5ec30d1a"},
};
static const TrackVariant final_variant[] = {
    {"d8cb990243dcdb885881f09a0bbe0788caee3c7b6ec9f62ed5d70c4c1d411587"},
};
static const TrackVariant trapped_variant[] = {
    {"8c1ad7905a95dacb8a57d9f34d97beeb8eb42a4f38a2188909d6c769ebdbab2d"},
};
static const TrackVariant comproom_variant[] = {
    {"8c5c51bb02897cf31acf7c148f5bdb11c7697de588a02868350f95c7421e01b6"},
};
static const TrackVariant running_variant[] = {
    {"91d32961ecb73186633113e50826249316e7261472805a5999fdf7336e2a1a01"},
};

#define CAT(v) { v, sizeof(v)/sizeof(v[0]) }

static const TrackCategory categories[] = {
    [MUSIC_NONE]     = { NULL, 0 },
    [MUSIC_TITLE]    = CAT(title_variant),
    [MUSIC_BASE]     = CAT(genbase_variant),
    [MUSIC_BATTLE]   = CAT(battle_variants),
    [MUSIC_SHOP]     = CAT(shop_variant),
    [MUSIC_HOLOMAP]  = CAT(holomap_variant),
    [MUSIC_ESCAPE]   = CAT(escaped_variant),
    [MUSIC_FINAL]    = CAT(final_variant),
    [MUSIC_TRAPPED]  = CAT(trapped_variant),
    [MUSIC_FCBASE]   = CAT(fcbase_variants),
    [MUSIC_VCBASE]   = CAT(vcbase_variants),
    [MUSIC_LONGNT]   = CAT(longnt_variants),
    [MUSIC_W]        = CAT(w_variants),
    [MUSIC_COMPROOM] = CAT(comproom_variant),
    [MUSIC_RUNNING]  = CAT(running_variant),
};

bool music_init(MusicSystem *mus, SoundSystem *snd, const DataVFS *vfs,
                int sample_rate, bool high_quality) {
    if (!mus) return false;
    if (sample_rate <= 0) sample_rate = SOUND_SAMPLE_RATE;
    memset(mus, 0, sizeof(*mus));
    mus->sound = snd;
    mus->enabled = true;
    mus->vfs = vfs;
    mus->prng_state = 0x12345678;
    mus->sample_rate = sample_rate;
    mus->master_volume = 1.0f;
    mus->high_quality = high_quality;
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

    if (track < MUSIC_NONE || track >= MUSIC_TRACK_COUNT) return;
    const TrackCategory *cat = &categories[track];
    if (!cat->variants || cat->count == 0) return;
    if (!mus->vfs) return;

    int idx = 0;
    if (cat->count > 1)
        idx = captive_prng(&mus->prng_state) % cat->count;

    size_t size;
    uint8_t *data = vfs_find_sha256(mus->vfs, cat->variants[idx].hash, &size);
    if (!data) return;

    if (midi_load(&mus->player, data, size)) {
        mus->owned_data = data;
        midi_set_sample_rate(&mus->player, mus->sample_rate);
        midi_set_high_quality(&mus->player, mus->high_quality);
        midi_set_volume(&mus->player, 0.3f * mus->master_volume);
        midi_play(&mus->player, true);
    } else {
        free(data);
    }
}

void music_set_volume(MusicSystem *mus, float volume) {
    if (!mus || !isfinite(volume)) return;
    if (volume < 0.0f) volume = 0.0f;
    if (volume > 1.0f) volume = 1.0f;
    mus->master_volume = volume;
    midi_set_volume(&mus->player, 0.3f * volume);
}

void music_stop(MusicSystem *mus) {
    if (!mus) return;
    midi_stop(&mus->player);
    free(mus->owned_data);
    mus->owned_data = NULL;
    mus->current_track = MUSIC_NONE;
    mus->requested_track = MUSIC_NONE;
}

void music_update(MusicSystem *mus) {
    if (!mus || !mus->enabled || !mus->player.playing) return;

    int16_t buffer[1024];
    midi_render(&mus->player, buffer, 1024);

    if (mus->sound && mus->sound->stream) {
        SDL_PutAudioStreamData(mus->sound->stream, buffer, sizeof(buffer));
    }
}

void music_shutdown(MusicSystem *mus) {
    if (!mus) return;
    midi_stop(&mus->player);
    free(mus->owned_data);
    mus->owned_data = NULL;
    memset(mus, 0, sizeof(*mus));
}
