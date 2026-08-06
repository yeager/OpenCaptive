#include "droid_ui.h"
#include <assert.h>

static void test_inactive_ui_cannot_mutate_state(void) {
    DroidUIState ui;
    GameState gs = {0};
    ItemDatabase db;

    item_db_init(&db);
    gs.droids[0].items[0] = 8; /* battery */
    gs.droids[0].energy = 10;
    gs.droids[0].energy_max = 100;
    droid_ui_init(&ui, 0);
    ui.ui_mode = DROID_UI_INVENTORY;
    ui.cursor = 0;
    ui.active = false;

    assert(!droid_ui_handle_key(&ui, &gs, &db, 0x0D));
    assert(gs.droids[0].energy == 10);
    assert(gs.droids[0].items[0] == 8);
}

static void test_battery_rejects_invalid_energy_state(void) {
    DroidUIState ui;
    GameState gs = {0};
    ItemDatabase db;
    item_db_init(&db);
    gs.droids[0].items[0] = 8;
    gs.droids[0].energy = 101;
    gs.droids[0].energy_max = 100;
    droid_ui_init(&ui, 0);
    ui.ui_mode = DROID_UI_INVENTORY;
    ui.cursor = 0;
    assert(!droid_ui_handle_key(&ui, &gs, &db, 0x0D));
    assert(gs.droids[0].energy == 101);
    assert(gs.droids[0].items[0] == 8);
}

static void test_unknown_inventory_item_is_not_equipped(void) {
    DroidUIState ui;
    GameState gs = {0};
    ItemDatabase db;
    item_db_init(&db);
    gs.droids[0].items[0] = 255;
    droid_ui_init(&ui, 0);
    ui.ui_mode = DROID_UI_INVENTORY;
    ui.cursor = 0;
    assert(!droid_ui_handle_key(&ui, &gs, &db, 0x0D));
    assert(gs.droids[0].items[0] == 255);
    assert(gs.droids[0].weapons[0] == 0);
    assert(gs.droids[0].weapons[1] == 0);
}

static void test_non_weapon_item_is_not_equipped_as_weapon(void) {
    DroidUIState ui;
    GameState gs = {0};
    ItemDatabase db;
    item_db_init(&db);
    gs.droids[0].items[0] = 40; /* cartridges */
    droid_ui_init(&ui, 0);
    ui.ui_mode = DROID_UI_INVENTORY;
    ui.cursor = 0;
    assert(!droid_ui_handle_key(&ui, &gs, &db, 0x0D));
    assert(gs.droids[0].items[0] == 40);
    assert(gs.droids[0].weapons[0] == 0);
    assert(gs.droids[0].weapons[1] == 0);
}

static void test_weapon_damage_uses_product_not_packed_order(void) {
    ItemDatabase db = {0};
    db.num_defs = 2;
    db.defs[0].id = 100;
    db.defs[0].category = ITEM_WEAPON_HANDGUN;
    db.defs[0].damage_min = 200;
    db.defs[0].damage_max = 1;
    db.defs[1].id = 101;
    db.defs[1].category = ITEM_WEAPON_HANDGUN;
    db.defs[1].damage_min = 1;
    db.defs[1].damage_max = 2;

    Droid d = {0};
    d.weapons[0] = 100;
    d.weapons[1] = 101;
    droid_recalc_weapon_damage(&d, &db);
    assert(d.weapon_damage == (uint16_t)(200 | (1u << 8)));

    db.defs[0].damage_min = -1;
    db.defs[0].damage_max = 255;
    d.weapons[1] = 0;
    droid_recalc_weapon_damage(&d, &db);
    assert(d.weapon_damage == 0);
}

int main(void) {
    test_inactive_ui_cannot_mutate_state();
    test_battery_rejects_invalid_energy_state();
    test_unknown_inventory_item_is_not_equipped();
    test_non_weapon_item_is_not_equipped_as_weapon();
    test_weapon_damage_uses_product_not_packed_order();
    return 0;
}
