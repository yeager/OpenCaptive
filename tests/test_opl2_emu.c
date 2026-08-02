#include "opl2_emu.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

static void test_init(void) {
    OPL2 opl;
    opl2_init(&opl);
    for (int ch = 0; ch < OPL2_NUM_CHANNELS; ch++) {
        assert(opl.channels[ch].op[0].env_stage == 0);
        assert(opl.channels[ch].op[1].env_stage == 0);
        assert(opl.channels[ch].key_on == false);
    }
}

static void test_silence_when_off(void) {
    OPL2 opl;
    opl2_init(&opl);
    int16_t buf[256];
    opl2_render(&opl, buf, 256);
    for (int i = 0; i < 256; i++)
        assert(buf[i] == 0);
}

static void test_produces_output(void) {
    OPL2 opl;
    opl2_init(&opl);

    opl2_write(&opl, 0x01, 0x20); // enable waveform select
    opl2_write(&opl, 0x20, 0x21); // op0: sustain, mult=1
    opl2_write(&opl, 0x23, 0x21); // op1: sustain, mult=1
    opl2_write(&opl, 0x40, 0x10); // op0: TL=16
    opl2_write(&opl, 0x43, 0x00); // op1: TL=0
    opl2_write(&opl, 0x60, 0xFF); // op0: fast attack/decay
    opl2_write(&opl, 0x63, 0xFF); // op1: fast attack/decay
    opl2_write(&opl, 0x80, 0x00); // op0: SL=0, RR=0
    opl2_write(&opl, 0x83, 0x00); // op1: SL=0, RR=0
    opl2_write(&opl, 0xC0, 0x01); // additive mode
    opl2_write(&opl, 0xA0, 0x80); // fnum low
    opl2_write(&opl, 0xB0, 0x2C); // key on, block=2, fnum hi

    int16_t buf[1024];
    opl2_render(&opl, buf, 1024);

    int nonzero = 0;
    for (int i = 0; i < 1024; i++)
        if (buf[i] != 0) nonzero++;
    assert(nonzero > 100);
}

static void test_key_off_releases(void) {
    OPL2 opl;
    opl2_init(&opl);

    opl2_write(&opl, 0x20, 0x21);
    opl2_write(&opl, 0x23, 0x21);
    opl2_write(&opl, 0x40, 0x00);
    opl2_write(&opl, 0x43, 0x00);
    opl2_write(&opl, 0x60, 0xFF);
    opl2_write(&opl, 0x63, 0xFF);
    opl2_write(&opl, 0x80, 0x0F); // fast release
    opl2_write(&opl, 0x83, 0x0F);
    opl2_write(&opl, 0xA0, 0x80);
    opl2_write(&opl, 0xB0, 0x2C); // key on

    int16_t buf[512];
    opl2_render(&opl, buf, 512);

    opl2_write(&opl, 0xB0, 0x0C); // key off

    opl2_render(&opl, buf, 512);
    opl2_render(&opl, buf, 512);

    /* After release, output should decay toward silence */
    int16_t late_buf[256];
    opl2_render(&opl, late_buf, 256);
    int32_t late_energy = 0;
    for (int i = 0; i < 256; i++)
        late_energy += abs(late_buf[i]);

    assert(late_energy < 50000);
}

int main(void) {
    test_init();
    test_silence_when_off();
    test_produces_output();
    test_key_off_releases();
    printf("All opl2_emu tests passed.\n");
    return 0;
}
