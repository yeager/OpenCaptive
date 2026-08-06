#include "liberation_save.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <io.h>
#define unlink _unlink
#define fileno _fileno
#define ftruncate _chsize
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
    out.city_y = 10;
    out.facing = 3;
    out.num_droids = 2;

    snprintf(out.droids[0].name, 16, "ALPHA-7");
    out.droids[0].hp = 100;
    out.droids[0].hp_max = 120;
    out.droids[0].energy = 50;
    out.droids[0].energy_max = 80;
    out.droids[0].level = 4;
    out.droids[0].xp = 0x12345678;
    out.droids[0].skills[0] = 10;
    out.droids[0].skills[1] = 20;
    out.droids[0].skills[8] = 80;
    out.droids[0].skills[9] = 90;
    out.droids[0].equipment[0] = 1001;
    out.droids[0].equipment[7] = 2002;
    out.droids[0].inventory[0] = 8;
    out.droids[0].inventory[9] = 57;
    out.droids[0].body_part_hp[0] = 12;
    out.droids[0].body_part_hp[5] = 231;

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
    out.reputation = -42;
    out.shared_inventory_count = 2;
    snprintf(out.shared_inventory[0].name, sizeof(out.shared_inventory[0].name),
             "KEYCARD");
    out.shared_inventory[0].item_type = 49;
    snprintf(out.shared_inventory[1].name, sizeof(out.shared_inventory[1].name),
             "POWER CELL");
    out.shared_inventory[1].item_type = 57;

    assert(lib_save_write(&out, TEST_PATH));

    LibSaveData in;
    memset(&in, 0xA5, sizeof(in));
    assert(lib_save_read(&in, TEST_PATH));

    assert(in.version == LIB_SAVE_VERSION);
    assert(in.seed_hi == 0x1234);
    assert(in.seed_lo == 0xABCD);
    assert(in.difficulty == 2);
    assert(in.mission == 5);
    assert(in.gold == 99999);
    assert(in.tick == 1000000);
    assert(in.city_x == 32);
    assert(in.city_y == 10);
    assert(in.facing == 3);
    assert(in.num_droids == 2);

    assert(strcmp(in.droids[0].name, "ALPHA-7") == 0);
    assert(in.droids[0].hp == 100);
    assert(in.droids[0].hp_max == 120);
    assert(in.droids[0].energy == 50);
    assert(in.droids[0].energy_max == 80);
    assert(in.droids[0].level == 4);
    assert(in.droids[0].xp == 0x12345678);
    assert(in.droids[0].skills[0] == 10);
    assert(in.droids[0].skills[1] == 20);
    assert(in.droids[0].skills[8] == 80);
    assert(in.droids[0].skills[9] == 90);
    assert(in.droids[0].equipment[0] == 1001);
    assert(in.droids[0].equipment[7] == 2002);
    assert(in.droids[0].inventory[0] == 8);
    assert(in.droids[0].inventory[9] == 57);
    assert(in.droids[0].body_part_hp[0] == 12);
    assert(in.droids[0].body_part_hp[5] == 231);

    /* A full-width on-disk name must still be a terminated C string. */
    memset(out.droids[0].name, 'X', sizeof(out.droids[0].name));
    assert(lib_save_write(&out, TEST_PATH));
    memset(&in, 0xA5, sizeof(in));
    assert(lib_save_read(&in, TEST_PATH));
    assert(in.droids[0].name[sizeof(in.droids[0].name) - 1] == '\0');

    assert(strcmp(in.droids[1].name, "BETA-3") == 0);
    assert(in.droids[1].hp == 80);
    assert(in.droids[1].level == 2);

    assert(lib_save_is_mission_complete(&in, 0));
    assert(!lib_save_is_mission_complete(&in, 1));
    assert(lib_save_is_mission_complete(&in, 4));
    assert(lib_save_is_mission_complete(&in, 255));
    assert(in.generators_destroyed == 3);
    assert(in.generators_total == 10);
    assert(in.reputation == -42);
    assert(in.shared_inventory_count == 2);
    assert(strcmp(in.shared_inventory[0].name, "KEYCARD") == 0);
    assert(in.shared_inventory[0].item_type == 49);
    assert(strcmp(in.shared_inventory[1].name, "POWER CELL") == 0);
    assert(in.shared_inventory[1].item_type == 57);

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

static void test_truncated_save(void) {
    FILE *f = fopen(TEST_PATH, "wb");
    assert(f != NULL);
    fwrite(LIB_SAVE_MAGIC, 1, 4, f);
    uint8_t version[2] = { (uint8_t)(LIB_SAVE_VERSION >> 8),
                           (uint8_t)LIB_SAVE_VERSION };
    fwrite(version, 1, sizeof(version), f);
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

static void test_write_rejects_invalid_state(void) {
    LibSaveData data;
    memset(&data, 0, sizeof(data));

    data.num_droids = LIB_SAVE_MAX_DROIDS + 1;
    assert(!lib_save_write(&data, TEST_PATH));
    data.num_droids = 0;
    data.facing = 4;
    assert(!lib_save_write(&data, TEST_PATH));
    data.facing = 0;
    data.generators_destroyed = 2;
    data.generators_total = 1;
    assert(!lib_save_write(&data, TEST_PATH));
    data.generators_destroyed = 0;
    data.city_x = -1;
    assert(!lib_save_write(&data, TEST_PATH));
    data.city_x = 0;
    data.city_y = CITYGRID_HEIGHT;
    assert(!lib_save_write(&data, TEST_PATH));
    data.city_y = 0;
    data.mission = 0;
    assert(!lib_save_write(&data, TEST_PATH));
}

static void test_write_rejects_invalid_droid_stats(void) {
    LibSaveData data;
    memset(&data, 0, sizeof(data));
    data.facing = 0;
    data.mission = 1;
    data.num_droids = 1;
    data.droids[0].hp = 101;
    data.droids[0].hp_max = 100;
    assert(!lib_save_write(&data, TEST_PATH));
}

static void test_read_rejects_truncated_droid_payload(void) {
    LibSaveData data;
    memset(&data, 0, sizeof(data));
    data.facing = 0;
    data.mission = 1;
    data.num_droids = 1;
    data.droids[0].hp = 10;
    data.droids[0].hp_max = 10;
    assert(lib_save_write(&data, TEST_PATH));

    FILE *f = fopen(TEST_PATH, "r+b");
    assert(f != NULL);
    assert(fseek(f, -1, SEEK_END) == 0);
    assert(ftruncate(fileno(f), ftell(f)) == 0);
    assert(fclose(f) == 0);

    LibSaveData loaded;
    memset(&loaded, 0xA5, sizeof(loaded));
    assert(!lib_save_read(&loaded, TEST_PATH));
    unlink(TEST_PATH);
}

static void test_read_rejects_trailing_bytes(void) {
    LibSaveData data;
    memset(&data, 0, sizeof(data));
    data.facing = 0;
    data.city_x = 0;
    data.city_y = 0;
    data.mission = 1;
    assert(lib_save_write(&data, TEST_PATH));

    FILE *f = fopen(TEST_PATH, "ab");
    assert(f != NULL);
    assert(fwrite("X", 1, 1, f) == 1);
    assert(fclose(f) == 0);

    LibSaveData loaded;
    memset(&loaded, 0xA5, sizeof(loaded));
    assert(!lib_save_read(&loaded, TEST_PATH));
    unlink(TEST_PATH);
}

static void test_read_previous_version_without_reputation(void) {
    LibSaveData data;
    memset(&data, 0, sizeof(data));
    data.city_x = 0;
    data.city_y = 0;
    data.mission = 1;
    data.reputation = 37;
    assert(lib_save_write(&data, TEST_PATH));

    FILE *f = fopen(TEST_PATH, "r+b");
    assert(f != NULL);
    assert(fseek(f, 4, SEEK_SET) == 0);
    uint8_t version[2] = {0, LIB_SAVE_SHARED_INVENTORY_VERSION};
    assert(fwrite(version, 1, sizeof(version), f) == sizeof(version));
    assert(fseek(f, -2, SEEK_END) == 0);
    long end_without_reputation = ftell(f);
    assert(end_without_reputation >= 0);
    assert(ftruncate(fileno(f), end_without_reputation) == 0);
    assert(fclose(f) == 0);

    LibSaveData loaded;
    memset(&loaded, 0xA5, sizeof(loaded));
    assert(lib_save_read(&loaded, TEST_PATH));
    assert(loaded.version == LIB_SAVE_SHARED_INVENTORY_VERSION);
    assert(loaded.reputation == 0);
    unlink(TEST_PATH);
}

static void test_read_reputation_version(void) {
    /* Version 5 added reputation and has the same on-disk shape as v7 when
       no droid records are present. */
    LibSaveData data;
    memset(&data, 0, sizeof(data));
    data.city_x = 0;
    data.city_y = 0;
    data.mission = 1;
    data.reputation = -42;
    assert(lib_save_write(&data, TEST_PATH));

    FILE *f = fopen(TEST_PATH, "r+b");
    assert(f != NULL);
    assert(fseek(f, 4, SEEK_SET) == 0);
    uint8_t version[2] = {0, LIB_SAVE_REPUTATION_VERSION};
    assert(fwrite(version, 1, sizeof(version), f) == sizeof(version));
    assert(fclose(f) == 0);

    LibSaveData loaded;
    memset(&loaded, 0xA5, sizeof(loaded));
    assert(lib_save_read(&loaded, TEST_PATH));
    assert(loaded.version == LIB_SAVE_REPUTATION_VERSION);
    assert(loaded.reputation == -42);
    unlink(TEST_PATH);
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

static void test_from_state_without_droids_is_empty(void) {
    LibSaveData data;
    CityNavState nav;
    city_nav_init(&nav, 4, 5, CITY_DIR_NORTH);
    lib_save_from_state(&data, 0, 0, 0, 1, 0, 0, &nav, NULL, 2);
    assert(data.num_droids == 0);
}

static void test_legacy_version_record_size_is_supported(void) {
    /* The reader's version dispatch is field-based, so a pre-v6 record must
       remain readable even though LibSaveDroid gained body_part_hp bytes. */
    LibSaveData data;
    memset(&data, 0, sizeof(data));
    data.city_x = 0;
    data.city_y = 0;
    data.mission = 1;
    data.num_droids = 1;
    data.droids[0].hp = 10;
    data.droids[0].hp_max = 10;
    assert(lib_save_write(&data, TEST_PATH));

    FILE *f = fopen(TEST_PATH, "r+b");
    assert(f != NULL);
    assert(fseek(f, 4, SEEK_SET) == 0);
    uint8_t version[2] = {0, LIB_SAVE_PREVIOUS_VERSION};
    assert(fwrite(version, 1, sizeof(version), f) == sizeof(version));
    /* Remove only v7's two additional skill bytes; v6 retains body-part
       condition bytes and reputation. */
    assert(fseek(f, -2, SEEK_END) == 0);
    long end = ftell(f);
    assert(end >= 0 && ftruncate(fileno(f), end) == 0);
    assert(fclose(f) == 0);

    LibSaveData loaded;
    assert(lib_save_read(&loaded, TEST_PATH));
    assert(loaded.version == LIB_SAVE_PREVIOUS_VERSION);
    unlink(TEST_PATH);
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
    test_truncated_save();
    test_null_args();
    test_write_rejects_invalid_state();
    test_write_rejects_invalid_droid_stats();
    test_read_rejects_truncated_droid_payload();
    test_read_rejects_trailing_bytes();
    test_read_previous_version_without_reputation();
    test_read_reputation_version();
    test_from_state();
    test_from_state_without_droids_is_empty();
    test_legacy_version_record_size_is_supported();
    test_mission_bitmap();
    printf("All liberation save tests passed\n");
    return 0;
}
