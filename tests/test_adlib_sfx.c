#include "adlib_sfx.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static void test_init(void) {
    AdlibSfxPlayer player;
    adlib_sfx_init(&player, 22050);
    assert(!adlib_sfx_is_playing(&player));
}

static void test_null_is_playing(void) {
    assert(!adlib_sfx_is_playing(NULL));
}

static void test_play_all_sequences(void) {
    /* Verify all 49 SFX sequences can be started and rendered without crashing */
    for (int i = 0; i < CAPTIVE_SFX_COUNT; i++) {
        AdlibSfxPlayer player;
        adlib_sfx_init(&player, 22050);
        assert(adlib_sfx_play(&player, i));
        assert(adlib_sfx_is_playing(&player));

        int16_t buf[512];
        /* Render enough ticks for the SFX to complete (max ~10 seconds) */
        for (int t = 0; t < 700; t++) {
            adlib_sfx_render(&player, buf, 512);
            if (!adlib_sfx_is_playing(&player)) break;
        }
    }
}

static void test_produces_audio(void) {
    /* SFX 5 has a clear patch load + key-on sequence */
    AdlibSfxPlayer player;
    adlib_sfx_init(&player, 22050);
    adlib_sfx_play(&player, 5);

    int16_t buf[4096];
    adlib_sfx_render(&player, buf, 4096);

    int nonzero = 0;
    for (int i = 0; i < 4096; i++)
        if (buf[i] != 0) nonzero++;

    assert(nonzero > 10);
}

static void test_stop_all(void) {
    AdlibSfxPlayer player;
    adlib_sfx_init(&player, 22050);
    adlib_sfx_play(&player, 1);
    adlib_sfx_play(&player, 2);
    assert(adlib_sfx_is_playing(&player));

    adlib_sfx_stop_all(&player);
    assert(!adlib_sfx_is_playing(&player));
}

static void test_invalid_id(void) {
    AdlibSfxPlayer player;
    adlib_sfx_init(&player, 22050);
    assert(!adlib_sfx_play(&player, -1));
    assert(!adlib_sfx_play(&player, CAPTIVE_SFX_COUNT));
    assert(!adlib_sfx_play(&player, 999));
}

int main(void) {
    test_init();
    test_null_is_playing();
    test_play_all_sequences();
    test_produces_audio();
    test_stop_all();
    test_invalid_id();
    printf("All adlib_sfx tests passed.\n");
    return 0;
}
