#include "liberation.h"
#include <assert.h>
#include <stdio.h>

static const char save_path[] = "opencaptive-liberation-test-save.bin";

int main(void) {
    LibState first, second;
    lib_init(&first, 42);
    lib_init(&second, 42);

    assert(first.mode == LIB_MODE_CITY);
    assert(first.current_building == -1);
    assert(first.city.num_buildings > 0);
    assert(first.city.num_buildings == second.city.num_buildings);
    assert(first.target_building >= 0);
    assert(first.target_building < first.city.num_buildings);

    for (int i = 0; i < first.city.num_buildings; i++) {
        const LibBuilding *a = &first.city.buildings[i];
        const LibBuilding *b = &second.city.buildings[i];
        assert(a->city_x == b->city_x && a->city_y == b->city_y);
        assert(a->width > 0 && a->height > 0);
        assert(a->num_floors >= 1 && a->num_floors <= LIB_BUILDING_FLOORS);
        assert(first.city.grid[a->city_y][a->city_x] == a->type);
        assert(a->floors[0].cells[LIB_FLOOR_HEIGHT - 1][LIB_FLOOR_WIDTH / 2]
               == LIB_CELL_DOOR);
    }

    const LibBuilding *target = &first.city.buildings[first.target_building];
    first.player_cx = target->city_x;
    first.player_cy = target->city_y;
    assert(lib_enter_current_building(&first));
    assert(first.mode == LIB_MODE_BUILDING);
    assert(first.current_building == first.target_building);
    assert(first.mission_complete);
    assert(lib_leave_current_building(&first));
    assert(first.mode == LIB_MODE_CITY && first.current_building == -1);

    first.player_cx = 7;
    first.player_cy = 8;
    first.mission_complete = true;
    assert(lib_save_game(&first, save_path));
    LibState loaded = {0};
    assert(lib_load_game(&loaded, save_path));
    assert(loaded.player_cx == 7 && loaded.player_cy == 8);
    assert(loaded.mission_complete && loaded.mode == LIB_MODE_CITY);

    FILE *file = fopen(save_path, "r+b");
    assert(file != NULL);
    assert(fputc(0, file) != EOF);
    assert(fclose(file) == 0);
    loaded.player_cx = 13;
    assert(!lib_load_game(&loaded, save_path));
    assert(loaded.player_cx == 13);
    remove(save_path);

    puts("All Liberation engine tests passed");
    return 0;
}
