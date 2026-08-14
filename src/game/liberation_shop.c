#include "liberation_shop.h"
#include "i18n.h"
#include "liberation_descriptions.h"
#include "liberation_city_text.h"
#include <string.h>
#include <stdio.h>

static uint16_t shop_prng(uint16_t *state) {
    *state = (uint16_t)(*state * 0x5E5 + 0x29);
    return *state;
}

/* Real runtime item IDs the shop stocks.  Each entry is a genuine Captive item
 * database id (the shared droid inventory is Captive items); the display name
 * is looked up from that database, not invented.  A previous version carried a
 * parallel table of made-up marketing names ("Laser Pistol", "Ion Cannon"…)
 * for these same items — that fabrication is gone; item 18 shows as PISTOL, 21
 * as RIFLE, and so on, which is what the item actually is. */
static const uint8_t shop_item_ids[] = {
    18, 21, 30, 11, 8, 55, 12, 2,
    11, 55, 49, 8, 43, 11, 5, 56,
};

/* The original Liberation bar is only a location with no drink menu in the game
 * data, so there are no authentic drink names to use.  The bar-fight mechanic
 * still needs purchasable items, so they carry the factual label "Drink" rather
 * than invented brand names (Synthi-Ale, Neuro-Fizz…). */

void lib_shop_init(LibShopState *shop, const CityBuilding *building, uint16_t seed) {
    if (!shop || !building) return;
    memset(shop, 0, sizeof(*shop));
    shop->prng_seed = seed;
    shop->building_type = (BuildingType)building->type;
    snprintf(shop->shop_name, sizeof(shop->shop_name), "%s", building->name);
    dialogue_tree_init(&shop->dialogue);
}

void lib_shop_generate_inventory(LibShopState *shop, const ItemDatabase *item_db) {
    if (!shop) return;
    uint16_t state = shop->prng_seed;
    int count = 4 + (shop_prng(&state) & 7);
    if (count > LIB_SHOP_MAX_ITEMS) count = LIB_SHOP_MAX_ITEMS;

    shop->item_count = 0;
    for (int i = 0; i < count; i++) {
        LibShopItem *item = &shop->items[shop->item_count++];
        unsigned name_idx = shop_prng(&state) % 16;
        item->item_type = shop_item_ids[name_idx];
        /* Authentic name from the item database for this real id; if no
         * database is supplied, fall back to a factual numeric label rather
         * than an invented product name. */
        const Item *def = item_db ? item_db_get(item_db, item->item_type) : NULL;
        if (def)
            snprintf(item->name, sizeof(item->name), "%s", def->name);
        else
            snprintf(item->name, sizeof(item->name), "ITEM %u", item->item_type);
        item->price = 50 + (shop_prng(&state) % 450);
        item->quantity = 1 + (shop_prng(&state) & 3);
        item->quality = 50 + (shop_prng(&state) % 51);
    }
}

void lib_shop_generate_bar_menu(LibShopState *shop) {
    if (!shop) return;
    uint16_t state = shop->prng_seed;
    int count = 3 + (shop_prng(&state) & 3);
    if (count > LIB_SHOP_MAX_ITEMS) count = LIB_SHOP_MAX_ITEMS;

    shop->item_count = 0;
    for (int i = 0; i < count; i++) {
        LibShopItem *item = &shop->items[shop->item_count++];
        /* Consume the roll that used to pick an invented brand name so the
         * item_type marker and prices are unchanged, then use a factual label. */
        unsigned name_idx = shop_prng(&state) % 8;
        snprintf(item->name, sizeof(item->name), "%s", _("Drink"));
        item->price = 5 + (shop_prng(&state) % 20);
        item->item_type = (uint16_t)(name_idx + 100);
        item->quantity = 10;
        item->quality = 80 + (shop_prng(&state) % 21);
    }
}

bool lib_shop_buy_item(LibShopState *shop, unsigned item_idx, uint32_t *gold) {
    if (!shop || !gold || shop->item_count > LIB_SHOP_MAX_ITEMS ||
        item_idx >= shop->item_count || item_idx >= LIB_SHOP_MAX_ITEMS) return false;

    LibShopItem *item = &shop->items[item_idx];
    if (item->quantity == 0) return false;
    if (*gold < item->price) return false;

    *gold -= item->price;
    item->quantity--;
    return true;
}

bool lib_shop_sell_item(LibShopState *shop, const char *name, uint16_t price,
                    uint16_t type, uint32_t *gold) {
    if (!shop || !gold || !name) return false;
    if (shop->item_count >= LIB_SHOP_MAX_ITEMS) return false;

    uint16_t sell_price = price / 2;
    if ((uint32_t)sell_price > UINT32_MAX - *gold) return false;
    *gold += sell_price;

    LibShopItem *item = &shop->items[shop->item_count++];
    snprintf(item->name, sizeof(item->name), "%s", name);
    item->price = sell_price;
    item->item_type = type;
    item->quantity = 1;
    item->quality = 50;
    return true;
}

void lib_shop_generate_dialogue(LibShopState *shop, const char *npc_name) {
    if (!shop || !npc_name) return;
    DialogueTree *tree = &shop->dialogue;
    dialogue_tree_init(tree);

    char greeting[DIALOGUE_MAX_TEXT];
    /* Prefer the authentic Liberation room description for this building
     * category; fall back to a generic greeting only when the real
     * descriptions are unavailable (e.g. no game data loaded in a unit test). */
    if (!liberation_description_for_building(shop->building_type,
                                             shop->prng_seed,
                                             greeting, sizeof(greeting)))
        snprintf(greeting, sizeof(greeting),
                 _("Welcome to %s! What can I do for you?"), shop->shop_name);

    unsigned exit_node = dialogue_tree_add_exit(tree, _("Goodbye!"));
    unsigned trade_text = dialogue_tree_add_text(tree, npc_name,
        _("Take a look at what I have."), exit_node);
    /* When the informant has something to say it is one of the game's
     * authentic clue quotes; the invented line is only the no-data fallback. */
    char news_text[DIALOGUE_MAX_TEXT];
    if (!liberation_city_text_clue(shop->prng_seed, news_text, sizeof(news_text)))
        snprintf(news_text, sizeof(news_text), "%s",
                 _("I might have heard something. Check back later."));
    unsigned info_text = dialogue_tree_add_text(tree, npc_name,
        news_text, exit_node);

    unsigned choice = dialogue_tree_add_choice(tree, npc_name, greeting);
    dialogue_tree_add_option(tree, choice, _("Buy"), trade_text);
    dialogue_tree_add_option(tree, choice, _("Any news?"), info_text);
    dialogue_tree_add_option(tree, choice, _("Leave"), exit_node);
}
