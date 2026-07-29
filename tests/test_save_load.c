#include "save_load.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static const char save_path[] = "opencaptive-test-save.bin";

static void test_round_trip(void) {
    GameState saved, loaded;
    game_state_init(&saved, GAME_CAPTIVE, 1);
    saved.base_id = 3;
    game_state_new_mission(&saved, 17);
    saved.party_x = 11;
    saved.party_y = 7;
    saved.party_dir = DIR_EAST;
    saved.current_level = 0;
    saved.selected_droid = 2;
    saved.tick = 12345;
    saved.gold = 777;
    saved.droids[2].hp = 37;
    saved.levels[0].cells[7][12].type = CELL_FLOOR;
    saved.generators_destroyed = 1;

    assert(save_game(&saved, save_path));
    memset(&loaded, 0, sizeof(loaded));
    assert(load_game(&loaded, save_path));
    assert(loaded.mission == 17 && loaded.mission_seed == 179);
    assert(loaded.party_x == 11 && loaded.party_y == 7);
    assert(loaded.party_dir == DIR_EAST && loaded.selected_droid == 2);
    assert(loaded.tick == 12345 && loaded.droids[2].hp == 37);
    assert(loaded.gold == 777);
    assert(loaded.levels[0].cells[7][12].type == CELL_FLOOR);
    assert(loaded.generators_destroyed == 1);
}

static void test_corrupt_save_preserves_state(void) {
    GameState before, target;
    game_state_init(&before, GAME_CAPTIVE, 1);
    before.base_id = 3;
    game_state_new_mission(&before, 17);
    assert(save_game(&before, save_path));

    FILE *file = fopen(save_path, "r+b");
    assert(file != NULL);
    assert(fputc(0, file) != EOF);
    assert(fclose(file) == 0);

    game_state_init(&target, GAME_CAPTIVE, 1);
    target.mission = 99;
    target.party_x = 55;
    assert(!load_game(&target, save_path));
    assert(target.mission == 99 && target.party_x == 55);
}

int main(void) {
    test_round_trip();
    test_corrupt_save_preserves_state();
    remove(save_path);
    puts("All save/load tests passed");
    return 0;
}
