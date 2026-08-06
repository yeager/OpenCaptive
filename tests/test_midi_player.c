#include "midi_player.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static int failures = 0;
#define ASSERT(cond, msg) do { \
    if (!(cond)) { printf("FAIL: %s\n", msg); failures++; } \
} while(0)

/* Minimal MIDI format 0: MThd + one MTrk with a single note */
static const uint8_t test_midi[] = {
    /* MThd */
    'M','T','h','d',
    0x00,0x00,0x00,0x06, /* header length = 6 */
    0x00,0x00,             /* format 0 */
    0x00,0x01,             /* 1 track */
    0x00,0x60,             /* 96 ticks per beat */
    /* MTrk */
    'M','T','r','k',
    0x00,0x00,0x00,0x11, /* track length = 17 bytes */
    /* delta=0, tempo meta: 500000 us/beat (120 BPM) */
    0x00, 0xFF, 0x51, 0x03, 0x07,0xA1,0x20,
    /* delta=0, program change ch0 to patch 0 */
    0x00, 0xC0, 0x00,
    /* delta=0, note on ch0, note 60, velocity 100 */
    0x00, 0x90, 0x3C, 0x64,
    /* delta=96, end of track */
    0x60, 0xFF, 0x2F, 0x00,
};

static void test_load(void) {
    MIDIPlayer mp;
    bool ok = midi_load(&mp, test_midi, sizeof(test_midi));
    ASSERT(ok, "midi_load succeeds");
    ASSERT(mp.num_tracks == 1, "1 track");
    ASSERT(mp.ticks_per_beat == 96, "96 tpb");
}

static void test_invalid(void) {
    MIDIPlayer mp;
    assert(!midi_load(NULL, test_midi, sizeof(test_midi)));
    assert(!midi_load(&mp, NULL, sizeof(test_midi)));
    ASSERT(!midi_load(&mp, (const uint8_t*)"NOPE", 4), "reject non-MIDI");
    ASSERT(!midi_load(&mp, test_midi, 5), "reject truncated");
    uint8_t truncated[sizeof(test_midi)];
    memcpy(truncated, test_midi, sizeof(truncated));
    truncated[18] = 0x7F; /* MTrk length exceeds the supplied buffer. */
    assert(!midi_load(&mp, truncated, sizeof(truncated)));

    uint8_t zero_tpb[sizeof(test_midi)];
    memcpy(zero_tpb, test_midi, sizeof(zero_tpb));
    zero_tpb[12] = 0;
    zero_tpb[13] = 0;
    assert(!midi_load(&mp, zero_tpb, sizeof(zero_tpb)));
}

static void test_render(void) {
    MIDIPlayer mp;
    midi_load(&mp, test_midi, sizeof(test_midi));
    midi_play(&mp, false);

    int16_t buf[2048];
    memset(buf, 0, sizeof(buf));
    midi_render(&mp, buf, 2048);

    /* Should produce some non-zero audio from OPL2 */
    bool has_signal = false;
    for (int i = 0; i < 2048; i++) {
        if (buf[i] != 0) { has_signal = true; break; }
    }
    ASSERT(has_signal, "OPL2 produces audio output");
}

static void test_stop(void) {
    MIDIPlayer mp;
    midi_load(&mp, test_midi, sizeof(test_midi));
    midi_play(&mp, false);

    int16_t buf[512];
    midi_render(&mp, buf, 512);
    midi_stop(&mp);
    ASSERT(!mp.playing, "stopped after midi_stop");
}

static void test_master_volume(void) {
    MIDIPlayer mp;
    assert(midi_load(&mp, test_midi, sizeof(test_midi)));
    assert(mp.master_volume == 1.0f);
    midi_set_volume(&mp, 0.0f);
    assert(mp.master_volume == 0.0f);
    midi_set_volume(&mp, 2.0f);
    assert(mp.master_volume == 1.0f);
    midi_set_volume(&mp, NAN);
    assert(mp.master_volume == 1.0f);
}

static void test_sample_rate(void) {
    MIDIPlayer mp;
    assert(midi_load(&mp, test_midi, sizeof(test_midi)));
    ASSERT(mp.sample_rate == 22050, "MIDI defaults to 22050 Hz");
    int original_ticks = mp.samples_per_tick;
    midi_set_sample_rate(&mp, 44100);
    ASSERT(mp.sample_rate == 44100, "MIDI sample rate changes");
    ASSERT(mp.samples_per_tick > original_ticks &&
           mp.samples_per_tick >= original_ticks * 2 - 1,
           "MIDI tick timing follows sample rate");
    midi_set_sample_rate(&mp, 0);
    ASSERT(mp.sample_rate == 44100, "invalid MIDI sample rate is ignored");
}

static void test_high_quality_filter(void) {
    MIDIPlayer normal;
    MIDIPlayer high_quality;
    assert(midi_load(&normal, test_midi, sizeof(test_midi)));
    assert(midi_load(&high_quality, test_midi, sizeof(test_midi)));
    midi_set_high_quality(&high_quality, true);
    midi_play(&normal, false);
    midi_play(&high_quality, false);

    int16_t normal_buf[512];
    int16_t high_quality_buf[512];
    midi_render(&normal, normal_buf, 512);
    midi_render(&high_quality, high_quality_buf, 512);
    bool differs = false;
    for (size_t i = 0; i < sizeof(normal_buf) / sizeof(normal_buf[0]); ++i) {
        if (normal_buf[i] != high_quality_buf[i]) {
            differs = true;
            break;
        }
    }
    ASSERT(differs, "HQ MIDI output filter changes rendered samples");
    ASSERT(high_quality.high_quality, "HQ MIDI mode remains enabled");
}

static void test_loop(void) {
    MIDIPlayer mp;
    midi_load(&mp, test_midi, sizeof(test_midi));
    midi_play(&mp, true);

    int16_t buf[512];
    /* Render enough to finish the track and restart */
    for (int i = 0; i < 100; i++) {
        midi_render(&mp, buf, 512);
    }
    ASSERT(mp.playing, "still playing in loop mode");
}

int main(void) {
    test_load();
    test_invalid();
    test_render();
    test_stop();
    test_master_volume();
    test_sample_rate();
    test_high_quality_filter();
    test_loop();
    if (failures) {
        printf("%d test(s) FAILED\n", failures);
        return 1;
    }
    printf("All midi_player tests passed\n");
    return 0;
}
