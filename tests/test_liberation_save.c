#include "liberation_save.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <io.h>
#define unlink _unlink
#else
#include <unistd.h>
#endif

#ifdef _WIN32
static const char *TEST_PATH = "test_liberation_save.bin";
#else
static const char *TEST_PATH = "/tmp/test_liberation_save.bin";
#endif

static void test_write_read_roundtrip(void) {
    LibSaveData out;
    memset(&out, 0, sizeof(out));
    out.seed_hi = 0x1234;
    out.seed_lo = 0xABCD;
    out.difficulty = 2;
    out.mission = 5;
    out.gold = 99999;
    out.tick = 1000000;
    out.city_x = 32;
    out.city_y = -10;
    out.facing = 3;
    out.num_droids = 2;

    snprintf(out.droids[0].name, 16, "ALPHA-7");
    out.droids[0].hp = 100;
    out.droids[0].hp_max = 120;
    out.droids[0].energy = 50;
    out.droids[0].energy_max = 80;
    out.droids[0].level = 4;
    out.droids[0].skills[0] = 10;
    out.droids[0].skills[1] = 20;
    out.droids[0].equipment[0] = 1001;
    out.droids[0].equipment[7] = 2002;

    snprintf(out.droids[1].name, 16, "BETA-3");
    out.droids[1].hp = 80;
    out.droids[1].hp_max = 80;
    out.droids[1].energy = 30;
    out.droids[1].energy_max = 60;
    out.droids[1].level = 2;

    lib_save_set_mission_complete(&out, 0);
    lib_save_set_mission_complete(&out, 4);
    lib_save_set_mission_complete(&out, 255);
    out.generators_destroyed = 3;
    out.generators_total = 10;

    assert(lib_save_write(&out, TEST_PATH));

    LibSaveData in;
    assert(lib_save_read(&in, TEST_PATH));

    assert(in.version == LIB_SAVE_VERSION);
    assert(in.seed_hi == 0x1234);
    assert(in.seed_lo == 0xABCD);
    assert(in.difficulty == 2);
    assert(in.mission == 5);
    assert(in.gold == 99999);
    assert(in.tick == 1000000);
    assert(in.city_x == 32);
    assert(in.city_y == -10);
    assert(in.facing == 3);
    assert(in.num_droids == 2);

    assert(strcmp(in.droids[0].name, "ALPHA-7") == 0);
    assert(in.droids[0].hp == 100);
    assert(in.droids[0].hp_max == 120);
    assert(in.droids[0].energy == 50);
    assert(in.droids[0].energy_max == 80);
    assert(in.droids[0].level == 4);
    assert(in.droids[0].skills[0] == 10);
    assert(in.droids[0].skills[1] == 20);
    assert(in.droids[0].equipment[0] == 1001);
    assert(in.droids[0].equipment[7] == 2002);

    assert(strcmp(in.droids[1].name, "BETA-3") == 0);
    assert(in.droids[1].hp == 80);
    assert(in.droids[1].level == 2);

    assert(lib_save_is_mission_complete(&in, 0));
    assert(!lib_save_is_mission_complete(&in, 1));
    assert(lib_save_is_mission_complete(&in, 4));
    assert(lib_save_is_mission_complete(&in, 255));
    assert(in.generators_destroyed == 3);
    assert(in.generators_total == 10);

    unlink(TEST_PATH);
}

static void test_bad_magic(void) {
    FILE *f = fopen(TEST_PATH, "wb");
    fwrite("JUNK", 1, 4, f);
    fclose(f);

    LibSaveData data;
    assert(!lib_save_read(&data, TEST_PATH));
    unlink(TEST_PATH);
}

static void test_null_args(void) {
    assert(!lib_save_write(NULL, TEST_PATH));
    assert(!lib_save_write(NULL, NULL));
    assert(!lib_save_read(NULL, TEST_PATH));
    assert(!lib_save_read(NULL, NULL));
}

static void test_from_state(void) {
    CityNavState nav;
    memset(&nav, 0, sizeof(nav));
    nav.cell_x = 15;
    nav.cell_y = 20;
    nav.facing = 2;

    LibSaveDroid droids[1];
    memset(droids, 0, sizeof(droids));
    snprintf(droids[0].name, 16, "TEST-1");
    droids[0].hp = 50;

    LibSaveData data;
    lib_save_from_state(&data, 0x11, 0x22, 1, 3, 5000, 42, &nav, droids, 1);

    assert(data.seed_hi == 0x11);
    assert(data.seed_lo == 0x22);
    assert(data.city_x == 15);
    assert(data.city_y == 20);
    assert(data.facing == 2);
    assert(data.num_droids == 1);
    assert(strcmp(data.droids[0].name, "TEST-1") == 0);
}

static void test_mission_bitmap(void) {
    LibSaveData data;
    memset(&data, 0, sizeof(data));

    assert(!lib_save_is_mission_complete(&data, 0));
    lib_save_set_mission_complete(&data, 0);
    assert(lib_save_is_mission_complete(&data, 0));

    lib_save_set_mission_complete(&data, 7);
    lib_save_set_mission_complete(&data, 8);
    assert(lib_save_is_mission_complete(&data, 7));
    assert(lib_save_is_mission_complete(&data, 8));
    assert(!lib_save_is_mission_complete(&data, 9));

    assert(!lib_save_is_mission_complete(&data, 256));
    lib_save_set_mission_complete(&data, 256);
    assert(!lib_save_is_mission_complete(&data, 256));
}

int main(void) {
    test_write_read_roundtrip();
    test_bad_magic();
    test_null_args();
    test_from_state();
    test_mission_bitmap();
    printf("All liberation save tests passed\n");
    return 0;
}
