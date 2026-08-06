#include "sound.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void put_be32(uint8_t *p, uint32_t value) {
    p[0] = (uint8_t)(value >> 24);
    p[1] = (uint8_t)(value >> 16);
    p[2] = (uint8_t)(value >> 8);
    p[3] = (uint8_t)value;
}

static void test_shutdown_without_audio_device(void) {
    SoundSystem sound = {0};
    const int8_t sample[] = {1, -2, 3};
    assert(sound_load_raw(&sound, sample, sizeof(sample), 8000) == 0);
    assert(sound.num_samples == 1);

    /* Loading samples is independent of SDL device creation.  Shutdown must
     * release them even when sound_init() was never successful. */
    sound_shutdown(&sound);
    assert(sound.num_samples == 0);
    assert(sound.samples[0].data == NULL);
}

static void test_8svx_respects_form_extent(void) {
    uint8_t form[12 + 8 + 20 + 8 + 2 + 1] = {0};
    memcpy(form, "FORM", 4);
    put_be32(form + 4, 4 + 8 + 20); /* FORM ends before the BODY chunk. */
    memcpy(form + 8, "8SVX", 4);
    memcpy(form + 12, "VHDR", 4);
    put_be32(form + 16, 20);
    form[12 + 8 + 12] = 0x1f;
    form[12 + 8 + 13] = 0x40;
    SoundSystem sound = {0};
    assert(sound_load_8svx(&sound, form, sizeof(form)) == -1);
    sound_shutdown(&sound);
}

int main(void) {
    test_shutdown_without_audio_device();
    test_8svx_respects_form_extent();
    puts("All sound tests passed");
    return 0;
}
