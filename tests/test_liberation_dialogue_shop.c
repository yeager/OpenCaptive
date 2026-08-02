#include "liberation_dialogue.h"
#include "liberation_shop.h"
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

int main(void) {
    test_dialogue_text_flow();
    test_dialogue_choices();
    test_shop_inventory();
    test_lib_shop_buy();
    test_lib_shop_sell();
    test_bar_menu();
    test_shop_dialogue();
    test_deterministic_inventory();
    printf("All dialogue/shop tests passed\n");
    return 0;
}
