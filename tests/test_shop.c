#include "shop.h"
#include <assert.h>
#include <limits.h>
#include <string.h>

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
    static GameState gs;
    memset(&gs, 0, sizeof(gs));
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
    static GameState gs;
    memset(&gs, 0, sizeof(gs));
    Droid *d = &gs.droids[0];

    d->hp = 0;
    d->hp_max = INT16_MAX;
    d->energy = 0;
    d->energy_max = INT16_MAX;
    for (int i = 0; i < 6; i++) { d->body_parts[i] = 1; d->body_part_hp[i] = 0; }
    int64_t expected_cost = (int64_t)INT16_MAX * 2 + (int64_t)INT16_MAX * 2 + (int64_t)255 * 6 * 2;
    assert(shop_repair(&shop, &gs, 0));
    assert(shop.gold == INT_MAX - (int)expected_cost);
    assert(d->hp == INT16_MAX && d->energy == INT16_MAX);
}

static void test_shop_repair_preserves_equipment(void) {
    ShopState shop = {.gold = 10000};
    static GameState gs;
    memset(&gs, 0, sizeof(gs));
    Droid *d = &gs.droids[0];
    d->hp = 90;
    d->hp_max = 100;
    d->energy = 80;
    d->energy_max = 100;
    for (int i = 0; i < 6; i++) d->body_part_hp[i] = 255;
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
    static GameState gs;
    memset(&gs, 0, sizeof(gs));
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
    static GameState gs;
    memset(&gs, 0, sizeof(gs));
    item_db_init(&db);
    shop.active = true;
    shop.num_items = SHOP_MAX_ITEMS + 1;
    shop.selected = SHOP_MAX_ITEMS;
    gs.selected_droid = 0;
    shop.gold = 1000;
    assert(!shop_buy(&shop, &db, &gs));
}

static void test_shop_navigation_stops_at_visible_item_limit(void) {
    ShopState shop = {.num_items = SHOP_MAX_ITEMS, .selected = SHOP_VISIBLE_ITEMS - 2};
    assert(shop_next_selection(&shop) == SHOP_VISIBLE_ITEMS - 1);
    assert(shop_next_selection(&shop) == SHOP_VISIBLE_ITEMS - 1);
    shop.num_items = 0;
    assert(shop_next_selection(&shop) == 0);
}

static void test_shop_stock_matches_visible_list(void) {
    ItemDatabase db;
    ShopState shop;
    item_db_init(&db);
    shop_init(&shop, &db, 99, 0x12345678U);
    assert(shop.num_items >= 0);
    assert(shop.num_items <= SHOP_VISIBLE_ITEMS);
}

static void test_shop_render_includes_item_name(void) {
    ItemDatabase db = {0};
    ShopState shop = {.active = true, .num_items = 1, .selected = 0,
                      .gold = 100};
    uint32_t first[320 * 200] = {0};
    uint32_t second[320 * 200] = {0};

    db.num_defs = 2;
    db.defs[0].id = 1;
    db.defs[0].name[0] = 'A';
    db.defs[0].name[1] = 'A';
    db.defs[0].name[2] = 'A';
    db.defs[0].price = 100;
    db.defs[1] = db.defs[0];
    db.defs[1].id = 2;
    db.defs[1].name[0] = 'B';
    /* Stand-in for the original SHOP1 backdrop.  This is a test fixture, not
     * shipped artwork: shop_render only needs some verified background to
     * composite the item list onto. */
    static uint32_t backdrop[320 * 200];
    for (int i = 0; i < 320 * 200; i++) backdrop[i] = 0xFF203040;

    shop.item_ids[0] = 1;
    shop_render(&shop, &db, first, 320, 200, backdrop);
    shop.item_ids[0] = 2;
    shop_render(&shop, &db, second, 320, 200, backdrop);

    bool label_differs = false;
    for (int y = 36; y < 43 && !label_differs; y++)
        for (int x = 40; x < 56; x++)
            if (first[y * 320 + x] != second[y * 320 + x]) {
                label_differs = true;
                break;
            }
    assert(label_differs);
}

/* Without the original backdrop the shop draws nothing at all.  It used to
 * synthesize a panel, border, and title bar in place of the real artwork. */
static void test_shop_render_without_backdrop_draws_nothing(void) {
    ItemDatabase db = {0};
    ShopState shop = {.active = true, .num_items = 1, .selected = 0,
                      .gold = 100};
    static uint32_t canvas[320 * 200];
    for (int i = 0; i < 320 * 200; i++) canvas[i] = 0xDEADBEEF;

    db.num_defs = 1;
    db.defs[0].id = 1;
    db.defs[0].name[0] = 'A';
    db.defs[0].price = 100;
    shop.item_ids[0] = 1;

    shop_render(&shop, &db, canvas, 320, 200, NULL);

    for (int i = 0; i < 320 * 200; i++)
        assert(canvas[i] == 0xDEADBEEF);
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
    test_shop_navigation_stops_at_visible_item_limit();
    test_shop_stock_matches_visible_list();
    test_shop_render_includes_item_name();
    test_shop_render_without_backdrop_draws_nothing();
    return 0;
}
