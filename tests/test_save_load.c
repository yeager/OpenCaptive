#include "save_load.h"
#include "puzzle.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static const char save_path[] = "opencaptive-test-save.bin";

static void test_round_trip(void) {
    static GameState saved, loaded;
    static CreatureList saved_creatures, loaded_creatures;
    static PuzzleList saved_puzzles, loaded_puzzles;
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
    memset(saved.droids[0].name, 'D', sizeof(saved.droids[0].name));
    saved.levels[0].cells[7][11].type = CELL_FLOOR;
    saved.levels[0].cells[7][12].type = CELL_FLOOR;
    saved.levels[0].cells[7][12].item_id = 57;
    saved.generators_destroyed = 1;
    saved_creatures.num_creatures = 1;
    saved_creatures.creatures[0] = (Creature){
        .type = CREATURE_ALIEN1, .hp = 0, .hp_max = 30,
        .x = 12, .y = 7, .level = 0, .active = false,
        .respawn_timer = 600,
    };
    saved_puzzles.num_puzzles = 1;
    saved_puzzles.puzzles[0] = (Puzzle){
        .type = PUZZLE_LEVER, .x = 9, .y = 7, .level = 0,
        .face = DIR_NORTH, .target_x = -1, .target_y = -1,
        .state = 1, .solution = 1, .solved = true,
    };
    saved.levels[0].cells[7][9].type = CELL_FLOOR;

    assert(save_game(&saved, &saved_creatures, &saved_puzzles, save_path));
    /* Version 5 is a fixed little-endian stream, not a dump of C structs.
     * Check the public header bytes so a future refactor cannot silently
     * reintroduce architecture-dependent saves. */
    FILE *encoded = fopen(save_path, "rb");
    assert(encoded != NULL);
    unsigned char header_prefix[16] = {0};
    assert(fread(header_prefix, 1, sizeof(header_prefix), encoded) == sizeof(header_prefix));
    assert(header_prefix[0] == 'V' && header_prefix[1] == 'S' &&
           header_prefix[2] == 'C' && header_prefix[3] == 'O');
    assert(header_prefix[4] == 5 && header_prefix[5] == 0 &&
           header_prefix[6] == 0 && header_prefix[7] == 0);
    assert(header_prefix[12] == 17 && header_prefix[13] == 0 &&
           header_prefix[14] == 0 && header_prefix[15] == 0);
    assert(fclose(encoded) == 0);
    memset(&loaded, 0, sizeof(loaded));
    static const char data_path[] = "/tmp/opencaptive-data";
    loaded.config.render_mode = CAPTIVE_RENDER_ENHANCED;
    loaded.config.scanlines = true;
    loaded.config.brightness = 73;
    loaded.config.data_path = data_path;
    assert(load_game(&loaded, &loaded_creatures, &loaded_puzzles, save_path));
    assert(loaded.mission == 17 && loaded.mission_seed == 179);
    assert(loaded.party_x == 11 && loaded.party_y == 7);
    assert(loaded.party_dir == DIR_EAST && loaded.selected_droid == 2);
    assert(loaded.tick == 12345 && loaded.droids[2].hp == 37);
    assert(loaded.droids[0].name[sizeof(loaded.droids[0].name) - 1] == '\0');
    assert(loaded.gold == 777);
    assert(loaded.levels[0].cells[7][12].type == CELL_FLOOR);
    assert(loaded.levels[0].cells[7][12].item_id == 57);
    assert(loaded.generators_destroyed == 1);
    assert(loaded_creatures.num_creatures == 1);
    assert(!loaded_creatures.creatures[0].active);
    assert(loaded_puzzles.num_puzzles == 1);
    assert(loaded_puzzles.puzzles[0].solved);
    assert(loaded.levels[0].cells[7][9].ornament[DIR_NORTH] == ORNAMENT_PANEL);
    assert(loaded.config.render_mode == CAPTIVE_RENDER_ENHANCED);
    assert(loaded.config.scanlines);
    assert(loaded.config.brightness == 73);
    assert(loaded.config.data_path == data_path);
}

static void test_solved_lever_and_door_survive_round_trip(void) {
    GameState saved, loaded;
    CreatureList creatures = {0}, loaded_creatures = {0};
    PuzzleList puzzles = {0}, loaded_puzzles = {0};
    game_state_init(&saved, GAME_CAPTIVE, 1);
    game_state_new_mission(&saved, 23);
    saved.current_level = 0;
    saved.party_x = 10;
    saved.party_y = 10;
    saved.levels[0].cells[10][10].type = CELL_FLOOR;
    saved.levels[0].cells[10][11].type = CELL_FLOOR;
    saved.levels[0].cells[10][12].type = CELL_DOOR_LOCKED;

    puzzles.num_puzzles = 1;
    puzzles.puzzles[0] = (Puzzle){
        .type = PUZZLE_LEVER, .x = 11, .y = 10, .level = 0,
        .face = DIR_EAST, .target_x = 12, .target_y = 10,
        .solution = 1, .solved = false,
    };
    saved.levels[0].cells[10][11].ornament[DIR_EAST] = ORNAMENT_PANEL;

    assert(puzzle_interact(&puzzles, &saved, 11, 10, DIR_EAST));
    assert(puzzles.puzzles[0].solved);
    assert(saved.levels[0].cells[10][12].type == CELL_DOOR);
    assert(save_game(&saved, &creatures, &puzzles, save_path));

    assert(load_game(&loaded, &loaded_creatures, &loaded_puzzles, save_path));
    assert(loaded_puzzles.num_puzzles == 1);
    assert(loaded_puzzles.puzzles[0].solved);
    assert(loaded_puzzles.puzzles[0].target_x == 12);
    assert(loaded_puzzles.puzzles[0].target_y == 10);
    assert(loaded.levels[0].cells[10][12].type == CELL_DOOR);
    assert(loaded.levels[0].cells[10][11].ornament[DIR_EAST] == ORNAMENT_PANEL);
}

static void test_corrupt_save_preserves_state(void) {
    static GameState before, target;
    static CreatureList creatures, target_creatures;
    static PuzzleList puzzles, target_puzzles;
    game_state_init(&before, GAME_CAPTIVE, 1);
    before.base_id = 3;
    game_state_new_mission(&before, 17);
    assert(save_game(&before, &creatures, &puzzles, save_path));

    FILE *file = fopen(save_path, "r+b");
    assert(file != NULL);
    assert(fputc(0, file) != EOF);
    assert(fclose(file) == 0);

    game_state_init(&target, GAME_CAPTIVE, 1);
    target.mission = 99;
    target.party_x = 55;
    assert(!load_game(&target, &target_creatures, &target_puzzles, save_path));
    assert(target.mission == 99 && target.party_x == 55);
}

static void test_load_recomputes_derived_weapon_damage(void) {
    GameState saved, loaded;
    CreatureList creatures = {0}, loaded_creatures = {0};
    PuzzleList puzzles = {0}, loaded_puzzles = {0};
    game_state_init(&saved, GAME_CAPTIVE, 1);
    game_state_new_mission(&saved, 1);
    saved.droids[0].weapons[0] = 13; /* KNUCKLE-DUSTER */
    assert(save_game(&saved, &creatures, &puzzles, save_path));

    FILE *file = fopen(save_path, "r+b");
    assert(file != NULL);
    /* v5 header is 18 little-endian u32 fields; weapon_damage is the final
     * two bytes of the first 64-byte droid record. */
    assert(fseek(file, 72 + 62, SEEK_SET) == 0);
    assert(fputc(0xFF, file) != EOF && fputc(0xFF, file) != EOF);
    assert(fclose(file) == 0);

    assert(load_game(&loaded, &loaded_creatures, &loaded_puzzles, save_path));
    assert(loaded.droids[0].weapons[0] == 13);
    assert(loaded.droids[0].weapon_damage != UINT16_MAX);
}

static void test_trailing_save_bytes_rejected(void) {
    GameState saved;
    CreatureList creatures = {0};
    PuzzleList puzzles = {0};
    game_state_init(&saved, GAME_CAPTIVE, 1);
    game_state_new_mission(&saved, 1);
    assert(save_game(&saved, &creatures, &puzzles, save_path));

    FILE *file = fopen(save_path, "ab");
    assert(file != NULL);
    assert(fwrite("X", 1, 1, file) == 1);
    assert(fclose(file) == 0);

    GameState loaded;
    CreatureList loaded_creatures = {0};
    PuzzleList loaded_puzzles = {0};
    assert(!load_game(&loaded, &loaded_creatures, &loaded_puzzles, save_path));
}

static void test_unknown_item_id_rejected(void) {
    GameState saved;
    CreatureList creatures = {0};
    PuzzleList puzzles = {0};
    game_state_init(&saved, GAME_CAPTIVE, 1);
    game_state_new_mission(&saved, 1);
    saved.droids[0].items[0] = 255;
    assert(!save_game(&saved, &creatures, &puzzles, save_path));

    saved.droids[0].items[0] = 0;
    saved.levels[0].cells[0][0].item_id = 255;
    assert(!save_game(&saved, &creatures, &puzzles, save_path));
}

static void test_droid_slots_use_matching_item_categories(void) {
    GameState gs;
    CreatureList creatures = {0};
    PuzzleList puzzles = {0};
    game_state_init(&gs, GAME_CAPTIVE, 1);
    game_state_new_mission(&gs, 1);

    for (int i = 0; i < 6; i++)
        assert(gs.droids[0].body_parts[i] == (uint8_t)(i + 1));
    assert(save_game(&gs, &creatures, &puzzles, save_path));

    gs.droids[0].body_parts[0] = 18; /* handgun cannot occupy armor slot */
    assert(!save_game(&gs, &creatures, &puzzles, save_path));
    gs.droids[0].body_parts[0] = 1;
    gs.droids[0].weapons[0] = 2; /* chest armor cannot occupy weapon slot */
    assert(!save_game(&gs, &creatures, &puzzles, save_path));
}

static void test_save_rejects_state_load_would_reject(void) {
    GameState gs;
    CreatureList creatures = {0};
    PuzzleList puzzles = {0};
    game_state_init(&gs, GAME_CAPTIVE, 1);
    game_state_new_mission(&gs, 1);
    gs.party_x = MAP_WIDTH;
    assert(!save_game(&gs, &creatures, &puzzles, save_path));
    gs.party_x = 0;
    gs.generators_destroyed = 2;
    gs.generators_total = 1;
    assert(!save_game(&gs, &creatures, &puzzles, save_path));
    gs.generators_destroyed = 0;
    creatures.num_creatures = 1;
    creatures.creatures[0] = (Creature){
        .type = CREATURE_ALIEN1, .x = 1, .y = 1, .level = 0,
        .hp = -1, .hp_max = 10
    };
    assert(!save_game(&gs, &creatures, &puzzles, save_path));
}

static void test_save_rejects_creature_overlap(void) {
    GameState gs;
    CreatureList creatures = {0};
    PuzzleList puzzles = {0};
    game_state_init(&gs, GAME_CAPTIVE, 1);
    game_state_new_mission(&gs, 1);
    creatures.num_creatures = 2;
    creatures.creatures[0] = (Creature){
        .type = CREATURE_ALIEN1, .hp = 10, .hp_max = 10,
        .x = gs.party_x, .y = gs.party_y, .level = gs.current_level,
        .active = true
    };
    assert(!save_game(&gs, &creatures, &puzzles, save_path));

    creatures.creatures[0].x = 2;
    creatures.creatures[0].y = 1;
    creatures.creatures[1] = creatures.creatures[0];
    assert(!save_game(&gs, &creatures, &puzzles, save_path));
}

static void test_save_rejects_invalid_identifiers(void) {
    GameState gs;
    CreatureList creatures = {0};
    PuzzleList puzzles = {0};
    game_state_init(&gs, GAME_CAPTIVE, 1);
    game_state_new_mission(&gs, 1);

    gs.mission = 0;
    assert(!save_game(&gs, &creatures, &puzzles, save_path));
    gs.mission = 1;
    gs.base_id = -1;
    assert(!save_game(&gs, &creatures, &puzzles, save_path));
}

static void test_save_rejects_invalid_puzzle(void) {
    GameState gs;
    CreatureList creatures = {0};
    PuzzleList puzzles = {0};
    game_state_init(&gs, GAME_CAPTIVE, 1);
    game_state_new_mission(&gs, 1);
    puzzles.num_puzzles = 1;
    puzzles.puzzles[0] = (Puzzle){
        .type = PUZZLE_BUTTON, .x = MAP_WIDTH, .y = 1,
        .level = 0, .face = DIR_NORTH, .target_x = -1, .target_y = -1
    };
    assert(!save_game(&gs, &creatures, &puzzles, save_path));

    puzzles.puzzles[0].x = 1;
    puzzles.puzzles[0].target_y = MAP_HEIGHT;
    assert(!save_game(&gs, &creatures, &puzzles, save_path));

    puzzles.puzzles[0].target_y = 1;
    puzzles.puzzles[0].target_x = -1;
    assert(!save_game(&gs, &creatures, &puzzles, save_path));

    puzzles.puzzles[0].target_x = 1;
    puzzles.puzzles[0].target_y = -1;
    assert(!save_game(&gs, &creatures, &puzzles, save_path));
}

static void test_save_rejects_invalid_creature_damage(void) {
    GameState gs;
    CreatureList creatures = {0};
    PuzzleList puzzles = {0};
    game_state_init(&gs, GAME_CAPTIVE, 1);
    game_state_new_mission(&gs, 1);
    creatures.num_creatures = 1;
    creatures.creatures[0].type = CREATURE_ALIEN1;
    creatures.creatures[0].hp = 10;
    creatures.creatures[0].hp_max = 10;
    creatures.creatures[0].x = 1;
    creatures.creatures[0].y = 1;
    creatures.creatures[0].level = 0;
    creatures.creatures[0].damage_min = -1;
    creatures.creatures[0].damage_max = 10;
    assert(!save_game(&gs, &creatures, &puzzles, save_path));
}

static void test_save_rejects_negative_creature_defense(void) {
    GameState gs;
    CreatureList creatures = {0};
    PuzzleList puzzles = {0};
    game_state_init(&gs, GAME_CAPTIVE, 1);
    game_state_new_mission(&gs, 1);
    creatures.num_creatures = 1;
    creatures.creatures[0] = (Creature){
        .type = CREATURE_ALIEN1, .hp = 10, .hp_max = 10,
        .x = 1, .y = 1, .level = 0, .defense = -1
    };
    assert(!save_game(&gs, &creatures, &puzzles, save_path));
}

static void test_save_rejects_invalid_creature_runtime_fields(void) {
    GameState gs;
    CreatureList creatures = {0};
    PuzzleList puzzles = {0};
    game_state_init(&gs, GAME_CAPTIVE, 1);
    game_state_new_mission(&gs, 1);
    creatures.num_creatures = 1;
    creatures.creatures[0] = (Creature){
        .type = CREATURE_ALIEN1, .hp = 10, .hp_max = 10,
        .x = 1, .y = 1, .level = 0, .active = true,
        .speed = 61
    };
    assert(!save_game(&gs, &creatures, &puzzles, save_path));
    creatures.creatures[0].speed = -1;
    assert(!save_game(&gs, &creatures, &puzzles, save_path));
    creatures.creatures[0].speed = 0;
    creatures.creatures[0].range = 8;
    assert(!save_game(&gs, &creatures, &puzzles, save_path));
    creatures.creatures[0].range = -1;
    assert(!save_game(&gs, &creatures, &puzzles, save_path));
    creatures.creatures[0].range = 0;
    creatures.creatures[0].cooldown = -1;
    assert(!save_game(&gs, &creatures, &puzzles, save_path));
    creatures.creatures[0].cooldown = 0;
    creatures.creatures[0].active = false;
    creatures.creatures[0].hp = 10;
    assert(!save_game(&gs, &creatures, &puzzles, save_path));
    creatures.creatures[0].hp = 0;
    creatures.creatures[0].respawn_timer = 0;
    assert(!save_game(&gs, &creatures, &puzzles, save_path));
    creatures.creatures[0].respawn_timer = -1;
    assert(!save_game(&gs, &creatures, &puzzles, save_path));
}

static void test_save_rejects_blocked_runtime_positions(void) {
    GameState gs;
    CreatureList creatures = {0};
    PuzzleList puzzles = {0};
    game_state_init(&gs, GAME_CAPTIVE, 1);
    game_state_new_mission(&gs, 1);

    gs.levels[gs.current_level].cells[gs.party_y][gs.party_x].type = CELL_WALL;
    assert(!save_game(&gs, &creatures, &puzzles, save_path));

    game_state_new_mission(&gs, 1);
    creatures.num_creatures = 1;
    creatures.creatures[0] = (Creature){
        .type = CREATURE_ALIEN1, .hp = 10, .hp_max = 10,
        .x = gs.party_x, .y = gs.party_y, .level = gs.current_level,
        .active = true,
    };
    gs.levels[gs.current_level].cells[gs.party_y][gs.party_x].type = CELL_DOOR;
    assert(!save_game(&gs, &creatures, &puzzles, save_path));

    game_state_new_mission(&gs, 1);
    creatures.num_creatures = 0;
    assert(save_game(&gs, &creatures, &puzzles, save_path));
    FILE *file = fopen(save_path, "r+b");
    assert(file != NULL);
    long cell_offset = 72L + 4L * 64L +
        ((long)gs.current_level * MAP_HEIGHT * MAP_WIDTH +
         (long)gs.party_y * MAP_WIDTH + gs.party_x) * 2L;
    assert(fseek(file, cell_offset, SEEK_SET) == 0);
    assert(fputc(CELL_WALL, file) != EOF);
    assert(fclose(file) == 0);
    GameState loaded;
    CreatureList loaded_creatures = {0};
    PuzzleList loaded_puzzles = {0};
    assert(!load_game(&loaded, &loaded_creatures, &loaded_puzzles, save_path));
}

static void test_load_rejects_noncanonical_creature_flag(void) {
    GameState gs;
    CreatureList creatures = {0};
    PuzzleList puzzles = {0};
    game_state_init(&gs, GAME_CAPTIVE, 1);
    game_state_new_mission(&gs, 1);
    creatures.num_creatures = 1;
    creatures.creatures[0] = (Creature){
        .type = CREATURE_ALIEN1, .hp = 10, .hp_max = 10,
        .x = 1, .y = 1, .level = 0, .active = true
    };
    gs.levels[0].cells[1][1].type = CELL_FLOOR;
    assert(save_game(&gs, &creatures, &puzzles, save_path));

    /* Header (72) + four 64-byte droids + all serialized dungeon levels. */
    FILE *file = fopen(save_path, "r+b");
    assert(file != NULL);
    long creature_offset = 72L + 4L * 64L +
                            (long)gs.num_levels * MAP_WIDTH * MAP_HEIGHT * 2L;
    /* Creature v5: type(4), hp fields(8), damage/defense(6), speed/range(2),
     * coordinates/level(12), cooldown(1), active(1). */
    assert(fseek(file, creature_offset + 29, SEEK_SET) == 0);
    assert(fputc(2, file) != EOF);
    assert(fclose(file) == 0);

    GameState loaded;
    CreatureList loaded_creatures = {0};
    PuzzleList loaded_puzzles = {0};
    assert(!load_game(&loaded, &loaded_creatures, &loaded_puzzles, save_path));
}

static void test_load_rejects_noncanonical_puzzle_flag(void) {
    GameState gs;
    CreatureList creatures = {0};
    PuzzleList puzzles = {0};
    game_state_init(&gs, GAME_CAPTIVE, 1);
    game_state_new_mission(&gs, 1);
    puzzles.num_puzzles = 1;
    puzzles.puzzles[0] = (Puzzle){
        .type = PUZZLE_LEVER, .x = 1, .y = 1, .level = 0,
        .face = DIR_NORTH, .target_x = -1, .target_y = -1,
        .solved = false
    };
    assert(save_game(&gs, &creatures, &puzzles, save_path));

    FILE *file = fopen(save_path, "r+b");
    assert(file != NULL);
    long puzzle_offset = 72L + 4L * 64L +
        (long)gs.num_levels * MAP_WIDTH * MAP_HEIGHT * 2L;
    /* Puzzle v5: solved is the final byte after type, four coordinates,
     * state, solution and two target coordinates. */
    assert(fseek(file, puzzle_offset + 30, SEEK_SET) == 0);
    assert(fputc(2, file) != EOF);
    assert(fclose(file) == 0);

    GameState loaded;
    CreatureList loaded_creatures = {0};
    PuzzleList loaded_puzzles = {0};
    assert(!load_game(&loaded, &loaded_creatures, &loaded_puzzles, save_path));
}

static void test_puzzle_interact_rejects_invalid_state(void) {
    PuzzleList puzzles = {0};
    GameState gs;
    game_state_init(&gs, GAME_CAPTIVE, 1);
    game_state_new_mission(&gs, 1);
    assert(!puzzle_interact(NULL, &gs, 0, 0, 0));
    assert(!puzzle_interact(&puzzles, NULL, 0, 0, 0));
    puzzle_check_step(NULL, &gs, 0, 0);
    assert(!puzzle_get_clipboard_hint(NULL, &gs, 0, 0, 0, NULL, 0));

    puzzles.num_puzzles = 1;
    puzzles.puzzles[0] = (Puzzle){
        .type = PUZZLE_BUTTON, .x = 1, .y = 1, .level = gs.current_level,
        .face = DIR_NORTH, .target_x = MAP_WIDTH, .target_y = 1
    };
    assert(puzzle_interact(&puzzles, &gs, 1, 1, DIR_NORTH));

    puzzles.puzzles[0].type = PUZZLE_FLOOR_TRAP;
    gs.selected_droid = 99;
    assert(!puzzle_interact(&puzzles, &gs, 1, 1, DIR_NORTH));

    puzzles.puzzles[0] = (Puzzle){
        .type = PUZZLE_POWER_SOCKET, .x = 1, .y = 1, .level = gs.current_level,
        .face = DIR_NORTH, .state = 3
    };
    assert(!puzzle_interact(&puzzles, &gs, 1, 1, DIR_NORTH));
    assert(puzzles.puzzles[0].state == 3);
}

static void test_power_socket_recharges_420_energy(void) {
    PuzzleList puzzles = {0};
    GameState gs;
    game_state_init(&gs, GAME_CAPTIVE, 1);
    game_state_new_mission(&gs, 1);
    gs.selected_droid = 0;
    gs.droids[0].energy = 100;
    gs.droids[0].energy_max = 1000;
    puzzles.num_puzzles = 1;
    puzzles.puzzles[0] = (Puzzle){
        .type = PUZZLE_POWER_SOCKET, .x = 1, .y = 1, .level = gs.current_level,
        .face = DIR_NORTH, .state = 1
    };

    assert(puzzle_interact(&puzzles, &gs, 1, 1, DIR_NORTH));
    assert(gs.droids[0].energy == 520);
    assert(puzzles.puzzles[0].state == 0);

    gs.droids[0].energy = 900;
    puzzles.puzzles[0].state = 1;
    assert(puzzle_interact(&puzzles, &gs, 1, 1, DIR_NORTH));
    assert(gs.droids[0].energy == 1000);
}

static void test_binary_lever_hint_uses_bit_zero(void) {
    PuzzleList puzzles = {0};
    GameState gs;
    char hint[64];
    game_state_init(&gs, GAME_CAPTIVE, 1);
    game_state_new_mission(&gs, 1);
    puzzles.num_puzzles = 1;
    puzzles.puzzles[0] = (Puzzle){
        .type = PUZZLE_LEVER, .x = 1, .y = 1, .level = gs.current_level,
        .face = DIR_NORTH, .solution = 2
    };

    assert(puzzle_get_clipboard_hint(&puzzles, &gs, 1, 1, DIR_NORTH,
                                     hint, (int)sizeof(hint)));
    assert(strcmp(hint, "Solution: OFF") == 0);
}

int main(void) {
    test_round_trip();
    test_solved_lever_and_door_survive_round_trip();
    test_corrupt_save_preserves_state();
    test_load_recomputes_derived_weapon_damage();
    test_trailing_save_bytes_rejected();
    test_unknown_item_id_rejected();
    test_droid_slots_use_matching_item_categories();
    test_save_rejects_state_load_would_reject();
    test_save_rejects_creature_overlap();
    test_save_rejects_invalid_identifiers();
    test_save_rejects_invalid_puzzle();
    test_save_rejects_invalid_creature_damage();
    test_save_rejects_negative_creature_defense();
    test_save_rejects_invalid_creature_runtime_fields();
    test_save_rejects_blocked_runtime_positions();
    test_load_rejects_noncanonical_creature_flag();
    test_load_rejects_noncanonical_puzzle_flag();
    test_puzzle_interact_rejects_invalid_state();
    test_power_socket_recharges_420_energy();
    test_binary_lever_hint_uses_bit_zero();
    remove(save_path);
    puts("All save/load tests passed");
    return 0;
}
