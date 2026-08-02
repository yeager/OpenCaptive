#include "midi_player.h"
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
    ASSERT(!midi_load(&mp, (const uint8_t*)"NOPE", 4), "reject non-MIDI");
    ASSERT(!midi_load(&mp, test_midi, 5), "reject truncated");
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
    test_loop();
    if (failures) {
        printf("%d test(s) FAILED\n", failures);
        return 1;
    }
    printf("All midi_player tests passed\n");
    return 0;
}
