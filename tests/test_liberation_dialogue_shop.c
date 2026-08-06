#include "liberation_dialogue.h"
#include "liberation_shop.h"
#include "liberation_building_interact.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_dialogue_text_flow(void) {
    DialogueTree tree;
    dialogue_tree_init(&tree);

    unsigned exit = dialogue_tree_add_exit(&tree, "Farewell.");
    unsigned n1 = dialogue_tree_add_text(&tree, "Max", "Hello traveller!", exit);
    (void)n1;

    DialogueState state;
    dialogue_state_init(&state, &tree);
    dialogue_state_start(&state);
    assert(dialogue_state_is_active(&state));

    /* Node 0 is the exit node (added first) */
    /* Node 1 is the text node */
    state.current_node = 1;

    const DialogueNode *cur = dialogue_state_current(&state);
    assert(cur != NULL);
    assert(cur->type == DIALOGUE_NODE_TEXT);
    assert(strcmp(cur->speaker, "Max") == 0);

    dialogue_state_advance(&state);
    assert(!dialogue_state_is_active(&state));
}

static void test_dialogue_start_skips_leading_exit(void) {
    DialogueTree tree;
    dialogue_tree_init(&tree);
    unsigned exit = dialogue_tree_add_exit(&tree, "Bye.");
    dialogue_tree_add_text(&tree, "NPC", "Welcome.", exit);

    DialogueState state;
    dialogue_state_init(&state, &tree);
    dialogue_state_start(&state);
    assert(dialogue_state_is_active(&state));
    assert(dialogue_state_current(&state) != NULL);
    assert(dialogue_state_current(&state)->type == DIALOGUE_NODE_TEXT);
    assert(strcmp(dialogue_state_current(&state)->text, "Welcome.") == 0);
}

static void test_dialogue_rejects_corrupt_node_count(void) {
    DialogueTree tree = {0};
    DialogueState state;
    tree.node_count = DIALOGUE_MAX_NODES + 1;
    dialogue_state_init(&state, &tree);
    dialogue_state_start(&state);
    assert(!dialogue_state_is_active(&state));
    assert(dialogue_state_current(&state) == NULL);
    assert(!dialogue_tree_add_option(&tree, 0, "bad", 300));

    dialogue_state_init(&state, &tree);
    state.active = true;
    dialogue_state_start(&state);
    assert(!dialogue_state_is_active(&state));
}

static void test_dialogue_choices(void) {
    DialogueTree tree;
    dialogue_tree_init(&tree);

    unsigned exit = dialogue_tree_add_exit(&tree, "Bye.");
    unsigned yes_text = dialogue_tree_add_text(&tree, "NPC", "Great!", exit);
    unsigned no_text = dialogue_tree_add_text(&tree, "NPC", "Maybe next time.", exit);

    unsigned choice = dialogue_tree_add_choice(&tree, "NPC", "Want to trade?");
    assert(dialogue_tree_add_option(&tree, choice, "Yes", yes_text));
    assert(dialogue_tree_add_option(&tree, choice, "No", no_text));

    DialogueState state;
    dialogue_state_init(&state, &tree);
    dialogue_state_start(&state);
    state.current_node = choice;

    const DialogueNode *cur = dialogue_state_current(&state);
    assert(cur->type == DIALOGUE_NODE_CHOICE);
    assert(cur->choice_count == 2);

    assert(!dialogue_state_advance(&state));

    assert(dialogue_state_choose(&state, 0));
    cur = dialogue_state_current(&state);
    assert(cur->type == DIALOGUE_NODE_TEXT);
    assert(strcmp(cur->text, "Great!") == 0);
}

static void test_dialogue_invalid_targets_end_state(void) {
    DialogueTree tree;
    dialogue_tree_init(&tree);
    unsigned text = dialogue_tree_add_text(&tree, "NPC", "Hello", 999);
    DialogueState state;
    dialogue_state_init(&state, &tree);
    dialogue_state_start(&state);
    state.current_node = text;
    assert(!dialogue_state_advance(&state));
    assert(!dialogue_state_is_active(&state));

    dialogue_tree_init(&tree);
    unsigned choice = dialogue_tree_add_choice(&tree, "NPC", "Choose");
    tree.nodes[choice].choice_count = 1;
    tree.nodes[choice].choices[0].target_node = 999;
    dialogue_state_init(&state, &tree);
    dialogue_state_start(&state);
    state.current_node = choice;
    assert(!dialogue_state_choose(&state, 0));
    assert(!dialogue_state_is_active(&state));

    dialogue_tree_init(&tree);
    dialogue_tree_add_text(&tree, "NPC", "Hello", 0);
    dialogue_state_init(&state, &tree);
    dialogue_state_start(&state);
    state.current_node = DIALOGUE_MAX_NODES;
    state.active = true;
    assert(!dialogue_state_advance(&state));
    assert(!dialogue_state_is_active(&state));
}

static void test_dialogue_rejects_unrepresentable_target(void) {
    DialogueTree tree;
    dialogue_tree_init(&tree);
    assert(dialogue_tree_add_text(&tree, "NPC", "bad", UINT_MAX) ==
           DIALOGUE_INVALID_NODE);
    assert(tree.node_count == 0);
}

static void test_dialogue_rejects_corrupt_choice_count(void) {
    DialogueTree tree;
    DialogueState state;
    dialogue_tree_init(&tree);
    unsigned node = dialogue_tree_add_choice(&tree, "NPC", "Choose");
    unsigned exit_node = dialogue_tree_add_exit(&tree, "Bye");
    assert(dialogue_tree_add_option(&tree, node, "Leave", exit_node));
    tree.nodes[node].choice_count = DIALOGUE_MAX_CHOICES + 1;

    dialogue_state_init(&state, &tree);
    dialogue_state_start(&state);
    assert(!dialogue_state_choose(&state, 0));
}

static uint16_t expected_shop_item_id(const char *name) {
    static const struct { const char *name; uint16_t id; } entries[] = {
        {"Laser Pistol", 18}, {"Plasma Rifle", 21}, {"Ion Cannon", 30},
        {"Shield Generator", 11}, {"Power Cell", 8},
        {"Neural Interface", 55}, {"Servo Motor", 12}, {"Armour Plate", 2},
        {"Sensor Array", 11}, {"Navigation Chip", 55},
        {"Communicator", 49}, {"Medikit", 8}, {"Charge Pack", 43},
        {"Targeting System", 11}, {"Gravity Boots", 5}, {"Data Crystal", 56},
    };
    for (size_t i = 0; i < sizeof(entries) / sizeof(entries[0]); i++)
        if (strcmp(name, entries[i].name) == 0) return entries[i].id;
    return 0;
}

static void test_police_fine_refusal_rejects_wrapped_choice(void) {
    BuildingInteraction bi;
    building_interact_init(&bi);
    bi.active = true;
    bi.type = INTERACT_POLICE;
    bi.bar_fight = true;
    dialogue_tree_init(&bi.shop.dialogue);
    unsigned exit_node = dialogue_tree_add_exit(&bi.shop.dialogue, "Leave");
    unsigned choice_node = dialogue_tree_add_choice(&bi.shop.dialogue, "Officer", "Fine?");
    assert(dialogue_tree_add_option(&bi.shop.dialogue, choice_node, "Leave", exit_node));
    dialogue_state_init(&bi.dialogue, &bi.shop.dialogue);
    bi.dialogue.active = true;
    bi.dialogue.current_node = choice_node;

    building_interact_choose(&bi, UINT_MAX);
    assert(!bi.fine_refused);
}

static void test_shop_inventory(void) {
    CityBuilding building;
    memset(&building, 0, sizeof(building));
    building.type = BUILDING_SHOP;
    strcpy(building.name, "Goldstein Emporium");

    LibShopState shop;
    lib_shop_init(&shop, &building, 42);
    lib_shop_generate_inventory(&shop);

    assert(shop.item_count >= 4);
    assert(shop.item_count <= 11);
    for (unsigned i = 0; i < shop.item_count; i++) {
        assert(shop.items[i].price > 0);
        assert(shop.items[i].quantity > 0);
        assert(strlen(shop.items[i].name) > 0);
        assert(shop.items[i].item_type == expected_shop_item_id(shop.items[i].name));
    }
}

static void test_lib_shop_buy(void) {
    CityBuilding building;
    memset(&building, 0, sizeof(building));
    building.type = BUILDING_SHOP;
    strcpy(building.name, "Test Shop");

    LibShopState shop;
    lib_shop_init(&shop, &building, 100);
    lib_shop_generate_inventory(&shop);

    uint32_t gold = 10000;
    uint16_t price = shop.items[0].price;
    uint8_t qty = shop.items[0].quantity;

    assert(lib_shop_buy_item(&shop, 0, &gold));
    assert(gold == 10000 - price);
    assert(shop.items[0].quantity == qty - 1);

    uint32_t no_gold = 0;
    assert(!lib_shop_buy_item(&shop, 0, &no_gold));

    assert(!lib_shop_buy_item(&shop, 999, &gold));

    shop.item_count = LIB_SHOP_MAX_ITEMS + 1;
    assert(!lib_shop_buy_item(&shop, LIB_SHOP_MAX_ITEMS, &gold));
}

static void test_lib_shop_sell(void) {
    CityBuilding building;
    memset(&building, 0, sizeof(building));
    building.type = BUILDING_SHOP;
    strcpy(building.name, "Pawnshop");

    LibShopState shop;
    lib_shop_init(&shop, &building, 200);
    unsigned before = shop.item_count;

    uint32_t gold = 100;
    assert(lib_shop_sell_item(&shop, "Old Laser", 80, 1, &gold));
    assert(gold == 140);
    assert(shop.item_count == before + 1);
    assert(strcmp(shop.items[shop.item_count - 1].name, "Old Laser") == 0);

    unsigned count_before_overflow = shop.item_count;
    uint32_t near_max = UINT32_MAX - 10U;
    assert(!lib_shop_sell_item(&shop, "Overflow", UINT16_MAX, 2, &near_max));
    assert(near_max == UINT32_MAX - 10U);
    assert(shop.item_count == count_before_overflow);
}

static void test_bar_menu(void) {
    CityBuilding building;
    memset(&building, 0, sizeof(building));
    building.type = BUILDING_BAR;
    strcpy(building.name, "Boozerama");

    LibShopState shop;
    lib_shop_init(&shop, &building, 77);
    lib_shop_generate_bar_menu(&shop);

    assert(shop.item_count >= 3);
    assert(shop.item_count <= 6);
    for (unsigned i = 0; i < shop.item_count; i++) {
        assert(shop.items[i].price >= 5);
        assert(shop.items[i].price <= 24);
    }
}

static void test_shop_dialogue(void) {
    CityBuilding building;
    memset(&building, 0, sizeof(building));
    building.type = BUILDING_SHOP;
    strcpy(building.name, "Hardware");

    LibShopState shop;
    lib_shop_init(&shop, &building, 300);
    lib_shop_generate_dialogue(&shop, "Joe");

    assert(shop.dialogue.node_count > 0);

    DialogueState state;
    dialogue_state_init(&state, &shop.dialogue);
    dialogue_state_start(&state);

    /* Start at the choice node (last added, which is node_count-1) */
    state.current_node = shop.dialogue.node_count - 1;
    const DialogueNode *cur = dialogue_state_current(&state);
    assert(cur->type == DIALOGUE_NODE_CHOICE);
    assert(cur->choice_count == 3);

    /* Choice 2 = "Leave" → exit node → returns false, dialogue ends */
    dialogue_state_choose(&state, 2);
    assert(!dialogue_state_is_active(&state));
}

static void test_deterministic_inventory(void) {
    CityBuilding b;
    memset(&b, 0, sizeof(b));
    b.type = BUILDING_SHOP;
    strcpy(b.name, "Test");

    LibShopState s1, s2;
    lib_shop_init(&s1, &b, 42);
    lib_shop_init(&s2, &b, 42);
    lib_shop_generate_inventory(&s1);
    lib_shop_generate_inventory(&s2);

    assert(s1.item_count == s2.item_count);
    for (unsigned i = 0; i < s1.item_count; i++) {
        assert(strcmp(s1.items[i].name, s2.items[i].name) == 0);
        assert(s1.items[i].price == s2.items[i].price);
    }
}

static void test_building_interact_shop(void) {
    CityGrid bg;
    memset(&bg, 0, sizeof(bg));
    bg.total_buildings = 1;
    bg.buildings[0].type = BUILDING_SHOP;
    strcpy(bg.buildings[0].name, "Astro Parts");

    CityGridState grid;
    memset(&grid, 0, sizeof(grid));
    grid.building_ids[10 * CITYGRID_WIDTH + 5] = 1;

    BuildingInteraction bi;
    building_interact_init(&bi);

    int gold = 5000;
    assert(building_interact_enter(&bi, &grid, &bg, 5, 10, &gold));
    assert(bi.active);
    assert(bi.type == INTERACT_SHOP);
    assert(bi.shop.item_count > 0);

    bi.purchased_count = 20;
    bi.bar_fight = true;
    bi.fine_paid = true;
    bi.industrial_hazard = true;
    building_interact_leave(&bi);
    assert(building_interact_enter(&bi, &grid, &bg, 5, 10, &gold));
    assert(bi.purchased_count == 0);
    assert(!bi.bar_fight && !bi.fine_paid && !bi.industrial_hazard);

    const char *text = building_interact_text(&bi);
    assert(text != NULL);
    assert(building_interact_choice_count(&bi) > 0);

    building_interact_leave(&bi);
    assert(!bi.active);
}

static void test_building_interact_bar(void) {
    CityGrid bg;
    memset(&bg, 0, sizeof(bg));
    bg.total_buildings = 1;
    bg.buildings[0].type = BUILDING_BAR;
    strcpy(bg.buildings[0].name, "Cosmic Lounge");

    CityGridState grid;
    memset(&grid, 0, sizeof(grid));
    grid.building_ids[5 * CITYGRID_WIDTH + 3] = 1;

    BuildingInteraction bi;
    building_interact_init(&bi);
    int gold = 100;
    assert(building_interact_enter(&bi, &grid, &bg, 3, 5, &gold));
    assert(bi.type == INTERACT_BAR);
    assert(bi.shop.item_count >= 3);
}

static void test_building_interact_generic(void) {
    CityGrid bg;
    memset(&bg, 0, sizeof(bg));
    bg.total_buildings = 1;
    bg.buildings[0].type = BUILDING_POLICE;
    strcpy(bg.buildings[0].name, "Station Alpha");

    CityGridState grid;
    memset(&grid, 0, sizeof(grid));
    grid.building_ids[2 * CITYGRID_WIDTH + 2] = 1;

    BuildingInteraction bi;
    building_interact_init(&bi);
    int gold = 0;
    assert(building_interact_enter(&bi, &grid, &bg, 2, 2, &gold));
    assert(bi.type == INTERACT_POLICE);
    assert(bi.active);

    const char *text = building_interact_text(&bi);
    assert(text != NULL);

    unsigned count = building_interact_choice_count(&bi);
    building_interact_choose(&bi, count - 1);
    assert(!bi.active);
}

static void test_police_fine_refusal_is_recorded(void) {
    CityGrid bg;
    memset(&bg, 0, sizeof(bg));
    bg.total_buildings = 1;
    bg.buildings[0].type = BUILDING_POLICE;
    strcpy(bg.buildings[0].name, "Station");
    CityGridState grid;
    memset(&grid, 0, sizeof(grid));
    grid.building_ids[0] = 1;

    BuildingInteraction bi;
    building_interact_init(&bi);
    int gold = 0;
    assert(building_interact_enter(&bi, &grid, &bg, 0, 0, &gold));
    bi.bar_fight = true;
    unsigned count = building_interact_choice_count(&bi);
    assert(count == 2);
    building_interact_choose(&bi, count - 1); /* Leave/refuse */
    assert(bi.fine_refused);
}

static void test_police_fine_payment_is_recorded(void) {
    CityGrid bg;
    memset(&bg, 0, sizeof(bg));
    bg.total_buildings = 1;
    bg.buildings[0].type = BUILDING_POLICE;
    strcpy(bg.buildings[0].name, "Station");
    CityGridState grid;
    memset(&grid, 0, sizeof(grid));
    grid.building_ids[0] = 1;

    BuildingInteraction bi;
    building_interact_init(&bi);
    int gold = 125;
    assert(building_interact_enter(&bi, &grid, &bg, 0, 0, &gold));
    building_interact_set_bar_fight(&bi, true); /* preceding bar visit */
    assert(building_interact_choice_count(&bi) == 3);
    building_interact_choose(&bi, 1); /* Pay fine */
    assert(gold == 25);
    assert(bi.fine_paid);
    assert(!bi.bar_fight);
    building_interact_advance(&bi);
    assert(!bi.active);
}

static void test_special_building_requires_investigate(void) {
    CityGrid bg;
    memset(&bg, 0, sizeof(bg));
    bg.total_buildings = 1;
    bg.buildings[0].type = BUILDING_SPECIAL;
    strcpy(bg.buildings[0].name, "Mission Site");

    CityGridState grid;
    memset(&grid, 0, sizeof(grid));
    grid.building_ids[2 * CITYGRID_WIDTH + 2] = 1;

    BuildingInteraction bi;
    building_interact_init(&bi);
    int gold = 0;
    assert(building_interact_enter(&bi, &grid, &bg, 2, 2, &gold));
    assert(building_interact_choice_count(&bi) == 2);
    building_interact_choose(&bi, DIALOGUE_MAX_CHOICES);
    assert(!bi.special_investigated);
    building_interact_choose(&bi, 1); /* Leave */
    assert(!bi.active);
    assert(!bi.mission_complete);

    assert(building_interact_enter(&bi, &grid, &bg, 2, 2, &gold));
    building_interact_choose(&bi, 0); /* Investigate */
    assert(bi.active);
    building_interact_advance(&bi);
    assert(!bi.active);
    assert(bi.mission_complete);
}

static void test_building_interact_buy(void) {
    CityGrid bg;
    memset(&bg, 0, sizeof(bg));
    bg.total_buildings = 1;
    bg.buildings[0].type = BUILDING_SHOP;
    strcpy(bg.buildings[0].name, "Gear Shop");

    CityGridState grid;
    memset(&grid, 0, sizeof(grid));
    grid.building_ids[1 * CITYGRID_WIDTH + 1] = 1;

    BuildingInteraction bi;
    building_interact_init(&bi);
    int gold = 10000;
    building_interact_enter(&bi, &grid, &bg, 1, 1, &gold);
    assert(!bi.shop_menu_active);
    assert(building_interact_choice_count(&bi) == 3);
    building_interact_choose(&bi, 0); /* Buy */
    assert(bi.shop_menu_active);

    uint16_t price = bi.shop.items[0].price;
    assert(building_interact_buy(&bi, 0));
    assert(gold == 10000 - price);

    bi.purchased_count = 20;
    int gold_before_full = gold;
    uint8_t quantity_before_full = bi.shop.items[0].quantity;
    assert(!building_interact_buy(&bi, 0));
    assert(gold == gold_before_full);
    assert(bi.shop.items[0].quantity == quantity_before_full);

    bi.purchased_count = 0;
    bi.shop.item_count = LIB_SHOP_MAX_ITEMS + 1;
    assert(!building_interact_buy(&bi, LIB_SHOP_MAX_ITEMS));
}

static void test_bar_fight_uses_seeded_quarter_chance(void) {
    int occurred = 0;
    int did_not_occur = 0;
    for (uint16_t seed = 1; seed <= 64; seed++) {
        CityGrid bg;
        memset(&bg, 0, sizeof(bg));
        bg.total_buildings = 1;
        bg.buildings[0].type = BUILDING_BAR;
        strcpy(bg.buildings[0].name, "Test Bar");
        CityGridState grid;
        memset(&grid, 0, sizeof(grid));
        grid.building_ids[0] = 1;

        BuildingInteraction bi;
        building_interact_init(&bi);
        int gold = 10000;
        assert(building_interact_enter(&bi, &grid, &bg, 0, 0, &gold));
        bi.shop.prng_seed = seed;
        assert(building_interact_buy(&bi, 0));
        if (bi.bar_fight) occurred++;
        else did_not_occur++;
    }
    assert(occurred > 0 && did_not_occur > 0);
}

static void test_reputation_shop_rules(void) {
    CityGrid bg;
    memset(&bg, 0, sizeof(bg));
    bg.total_buildings = 1;
    bg.buildings[0].type = BUILDING_SHOP;
    strcpy(bg.buildings[0].name, "Reputation Shop");
    CityGridState grid;
    memset(&grid, 0, sizeof(grid));
    grid.building_ids[1] = 1;
    int gold = 10000;
    BuildingInteraction bi;
    building_interact_init(&bi);
    assert(building_interact_enter(&bi, &grid, &bg, 1, 0, &gold));
    uint16_t normal = bi.shop.items[0].price;
    building_interact_set_reputation(&bi, 50);
    assert(bi.shop.items[0].price == (uint16_t)((normal * 3U) / 4U));
    building_interact_set_reputation(&bi, -50);
    assert(!building_interact_buy(&bi, 0));

    /* Entering another shop must start a fresh pricing pass. */
    CityGridState second_grid;
    CityGrid second_buildings;
    memset(&second_grid, 0, sizeof(second_grid));
    memset(&second_buildings, 0, sizeof(second_buildings));
    second_buildings.total_buildings = 1;
    second_buildings.buildings[0].type = BUILDING_SHOP;
    strcpy(second_buildings.buildings[0].name, "Second Shop");
    second_grid.building_ids[0] = 1;
    assert(building_interact_enter(&bi, &second_grid, &second_buildings,
                                   0, 0, &gold));
    uint16_t second_normal = bi.shop.items[0].price;
    building_interact_set_reputation(&bi, 50);
    assert(bi.shop.items[0].price == (uint16_t)((second_normal * 3U) / 4U));
}

static void test_building_interact_rejects_corrupt_choice_count(void) {
    BuildingInteraction bi;
    building_interact_init(&bi);
    bi.active = true;
    bi.dialogue.active = true;
    bi.dialogue.tree = &bi.shop.dialogue;
    bi.shop.dialogue.node_count = 1;
    bi.shop.dialogue.nodes[0].type = DIALOGUE_NODE_CHOICE;
    bi.shop.dialogue.nodes[0].choice_count = DIALOGUE_MAX_CHOICES + 1;
    bi.dialogue.current_node = 0;
    assert(building_interact_choice_count(&bi) == 0);
    assert(building_interact_choice_label(&bi, 0) == NULL);
    building_interact_choose(&bi, DIALOGUE_MAX_CHOICES);
}

static void test_building_interact_empty_catalog(void) {
    CityGrid bg;
    memset(&bg, 0, sizeof(bg));
    CityGridState grid;
    memset(&grid, 0, sizeof(grid));
    grid.building_ids[0] = 1;

    BuildingInteraction bi;
    building_interact_init(&bi);
    assert(!building_interact_enter(&bi, &grid, &bg, 0, 0, NULL));
    assert(!bi.active);
}

static void test_building_interact_rejects_empty_building_sentinel(void) {
    CityGrid bg;
    CityGridState grid;
    BuildingInteraction bi;
    memset(&bg, 0, sizeof(bg));
    memset(&grid, 0, sizeof(grid));
    building_interact_init(&bi);
    bg.total_buildings = 1;
    bg.buildings[0].type = BUILDING_SHOP;
    grid.building_ids[0] = 0xFF;
    int gold = 100;
    assert(!building_interact_enter(&bi, &grid, &bg, 0, 0, &gold));
    assert(!bi.active);
}

static void test_building_interact_rejects_invalid_catalog_size(void) {
    CityGrid bg;
    CityGridState grid;
    BuildingInteraction bi;
    memset(&bg, 0, sizeof(bg));
    memset(&grid, 0, sizeof(grid));
    building_interact_init(&bi);
    grid.building_ids[0] = 1;
    bg.total_buildings = -1;
    assert(!building_interact_enter(&bi, &grid, &bg, 0, 0, NULL));
    bg.total_buildings = CITYGEN_MAX_BUILDINGS + 1;
    assert(!building_interact_enter(&bi, &grid, &bg, 0, 0, NULL));

    bg.buildings[0].type = 99;
    bg.total_buildings = 1;
    int gold = 100;
    bi.active = true;
    assert(!building_interact_enter(&bi, &grid, &bg, 0, 0, &gold));
    assert(!bi.active);
}

int main(void) {
    test_dialogue_text_flow();
    test_dialogue_start_skips_leading_exit();
    test_dialogue_rejects_corrupt_node_count();
    test_dialogue_choices();
    test_dialogue_invalid_targets_end_state();
    test_dialogue_rejects_unrepresentable_target();
    test_dialogue_rejects_corrupt_choice_count();
    test_police_fine_refusal_rejects_wrapped_choice();
    test_shop_inventory();
    test_lib_shop_buy();
    test_lib_shop_sell();
    test_bar_menu();
    test_shop_dialogue();
    test_deterministic_inventory();
    test_building_interact_shop();
    test_building_interact_bar();
    test_building_interact_generic();
    test_police_fine_refusal_is_recorded();
    test_police_fine_payment_is_recorded();
    test_special_building_requires_investigate();
    test_building_interact_buy();
    test_bar_fight_uses_seeded_quarter_chance();
    test_reputation_shop_rules();
    test_building_interact_rejects_corrupt_choice_count();
    test_building_interact_empty_catalog();
    test_building_interact_rejects_empty_building_sentinel();
    test_building_interact_rejects_invalid_catalog_size();
    printf("All dialogue/shop tests passed\n");
    return 0;
}
