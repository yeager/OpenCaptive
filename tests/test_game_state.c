#include "game_state.h"
#include "combat.h"
#include <assert.h>
#include <stdio.h>

static int generator_count(const GameState *gs) {
    int count = 0;
    for (int level = 0; level < gs->num_levels; level++)
        for (int y = 0; y < MAP_HEIGHT; y++)
            for (int x = 0; x < MAP_WIDTH; x++)
                count += gs->levels[level].cells[y][x].type == CELL_GENERATOR;
    return count;
}

int main(void) {
    GameState gs;
    game_state_init(&gs, GAME_CAPTIVE, 1);
    gs.base_id = 3;
    game_state_new_mission(&gs, 17);

    assert(gs.mission_seed == 179);
    assert(gs.num_levels >= 2 && gs.num_levels <= 5);
    assert(gs.generators_total == generator_count(&gs));
    assert(gs.generators_total > 0);

    /* Interacting with a generator is the only action that increments the
     * objective counter.  Position it in front of the party to test the
     * gameplay path, rather than mutating the counter directly. */
    gs.current_level = 0;
    gs.party_x = 1;
    gs.party_y = 1;
    gs.party_dir = DIR_EAST;
    gs.levels[0].cells[1][2].type = CELL_GENERATOR;
    int before = gs.generators_destroyed;
    combat_interact(&gs);
    assert(gs.generators_destroyed == before + 1);
    assert(gs.levels[0].cells[1][2].type == CELL_FLOOR);

    puts("All game state tests passed");
    return 0;
}
