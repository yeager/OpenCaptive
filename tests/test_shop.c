#include "shop.h"
#include <assert.h>
#include <limits.h>

static void test_shop_clamps_corrupt_definition_count(void) {
    ItemDatabase db = {0};
    ShopState shop;

    db.num_defs = INT_MAX;
    shop_init(&shop, &db, 0, 1234);

    assert(shop.active);
    assert(shop.num_items >= 0);
    assert(shop.num_items <= SHOP_MAX_ITEMS);
}

static void test_shop_rejects_negative_definition_count(void) {
    ItemDatabase db = {0};
    ShopState shop;

    db.num_defs = -1;
    shop_init(&shop, &db, 0, 1234);

    assert(shop.active);
    assert(shop.num_items == 0);
}

static void test_shop_accepts_extreme_level_without_signed_overflow(void) {
    ItemDatabase db = {0};
    ShopState shop;

    db.num_defs = 0;
    shop_init(&shop, &db, INT_MIN, UINT32_MAX);
    assert(shop.active);
    shop_init(&shop, &db, INT_MAX, 0);
    assert(shop.active);
}

static void test_shop_clamps_negative_level(void) {
    ItemDatabase db;
    ShopState negative, zero;
    item_db_init(&db);

    shop_init(&negative, &db, -1, 1234);
    shop_init(&zero, &db, 0, 1234);

    assert(negative.active && zero.active);
    assert(negative.num_items == zero.num_items);
    for (int i = 0; i < negative.num_items; i++)
        assert(negative.item_ids[i] == zero.item_ids[i]);
}

static void test_shop_repair_rejects_invalid_droid_stats(void) {
    ShopState shop = {.gold = 1000};
    GameState gs = {0};
    Droid *d = &gs.droids[0];

    d->hp = -1;
    d->hp_max = 100;
    d->energy = 100;
    d->energy_max = 100;
    assert(!shop_repair(&shop, &gs, 0));
    assert(shop.gold == 1000);

    d->hp = 101;
    d->hp_max = 100;
    assert(!shop_repair(&shop, &gs, 0));
    assert(shop.gold == 1000);
}

static void test_shop_repair_handles_large_valid_stats(void) {
    ShopState shop = {.gold = INT_MAX};
    GameState gs = {0};
    Droid *d = &gs.droids[0];

    d->hp = 0;
    d->hp_max = INT16_MAX;
    d->energy = 0;
    d->energy_max = INT16_MAX;
    assert(shop_repair(&shop, &gs, 0));
    assert(shop.gold == INT_MAX - 4 * INT16_MAX);
    assert(d->hp == INT16_MAX && d->energy == INT16_MAX);
}

static void test_shop_repair_preserves_equipment(void) {
    ShopState shop = {.gold = 1000};
    GameState gs = {0};
    Droid *d = &gs.droids[0];
    d->hp = 90;
    d->hp_max = 100;
    d->energy = 80;
    d->energy_max = 100;
    d->body_parts[0] = 17;
    d->body_parts[1] = 23;
    d->body_part_hp[0] = 10;
    d->body_part_hp[1] = 20;
    assert(shop_repair(&shop, &gs, 0));
    assert(d->body_parts[0] == 17 && d->body_parts[1] == 23);
    assert(d->body_part_hp[0] == 255 && d->body_part_hp[1] == 255);
}

static void test_inactive_shop_cannot_sell(void) {
    ItemDatabase db;
    ShopState shop = {0};
    GameState gs = {0};
    item_db_init(&db);
    gs.selected_droid = 0;
    shop.item_ids[0] = 8;
    shop.num_items = 1;
    shop.selected = 0;
    shop.gold = 1000;
    assert(!shop_buy(&shop, &db, &gs));
    assert(shop.gold == 1000);
    assert(gs.droids[0].items[0] == 0);
}

static void test_shop_rejects_corrupt_item_count(void) {
    ItemDatabase db;
    ShopState shop = {0};
    GameState gs = {0};
    item_db_init(&db);
    shop.active = true;
    shop.num_items = SHOP_MAX_ITEMS + 1;
    shop.selected = SHOP_MAX_ITEMS;
    gs.selected_droid = 0;
    shop.gold = 1000;
    assert(!shop_buy(&shop, &db, &gs));
}

int main(void) {
    test_shop_clamps_corrupt_definition_count();
    test_shop_rejects_negative_definition_count();
    test_shop_accepts_extreme_level_without_signed_overflow();
    test_shop_clamps_negative_level();
    test_shop_repair_rejects_invalid_droid_stats();
    test_shop_repair_handles_large_valid_stats();
    test_shop_repair_preserves_equipment();
    test_inactive_shop_cannot_sell();
    test_shop_rejects_corrupt_item_count();
    return 0;
}
