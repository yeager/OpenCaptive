#include "liberation_shop.h"
#include "i18n.h"
#include <string.h>
#include <stdio.h>

static uint16_t shop_prng(uint16_t *state) {
    *state = (uint16_t)(*state * 0x5E5 + 0x29);
    return *state;
}

static const char *shop_item_names[] = {
    "Laser Pistol", "Plasma Rifle", "Ion Cannon", "Shield Generator",
    "Power Cell", "Neural Interface", "Servo Motor", "Armour Plate",
    "Sensor Array", "Navigation Chip", "Communicator", "Medikit",
    "Charge Pack", "Targeting System", "Gravity Boots", "Data Crystal",
};

static const char *bar_item_names[] = {
    "Synthi-Ale", "Neuro-Fizz", "Grav-Tonic", "Plasma Punch",
    "Ion Brew", "Dark Matter", "Quasar Shot", "Nebula Wine",
};

void lib_shop_init(LibShopState *shop, const CityBuilding *building, uint16_t seed) {
    memset(shop, 0, sizeof(*shop));
    shop->prng_seed = seed;
    shop->building_type = (BuildingType)building->type;
    snprintf(shop->shop_name, sizeof(shop->shop_name), "%s", building->name);
    dialogue_tree_init(&shop->dialogue);
}

void lib_shop_generate_inventory(LibShopState *shop) {
    uint16_t state = shop->prng_seed;
    int count = 4 + (shop_prng(&state) & 7);
    if (count > SHOP_MAX_ITEMS) count = SHOP_MAX_ITEMS;

    shop->item_count = 0;
    for (int i = 0; i < count; i++) {
        LibShopItem *item = &shop->items[shop->item_count++];
        unsigned name_idx = shop_prng(&state) % 16;
        snprintf(item->name, sizeof(item->name), "%s", shop_item_names[name_idx]);
        item->price = 50 + (shop_prng(&state) % 450);
        item->item_type = (uint16_t)(name_idx + 1);
        item->quantity = 1 + (shop_prng(&state) & 3);
        item->quality = 50 + (shop_prng(&state) % 51);
    }
}

void lib_shop_generate_bar_menu(LibShopState *shop) {
    uint16_t state = shop->prng_seed;
    int count = 3 + (shop_prng(&state) & 3);
    if (count > SHOP_MAX_ITEMS) count = SHOP_MAX_ITEMS;

    shop->item_count = 0;
    for (int i = 0; i < count; i++) {
        LibShopItem *item = &shop->items[shop->item_count++];
        unsigned name_idx = shop_prng(&state) % 8;
        snprintf(item->name, sizeof(item->name), "%s", bar_item_names[name_idx]);
        item->price = 5 + (shop_prng(&state) % 20);
        item->item_type = (uint16_t)(name_idx + 100);
        item->quantity = 10;
        item->quality = 80 + (shop_prng(&state) % 21);
    }
}

bool lib_shop_buy_item(LibShopState *shop, unsigned item_idx, uint32_t *gold) {
    if (!shop || !gold || item_idx >= shop->item_count) return false;

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
    if (shop->item_count >= SHOP_MAX_ITEMS) return false;

    uint16_t sell_price = price / 2;
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
    DialogueTree *tree = &shop->dialogue;
    dialogue_tree_init(tree);

    char greeting[DIALOGUE_MAX_TEXT];
    snprintf(greeting, sizeof(greeting),
             _("Welcome to %s! What can I do for you?"), shop->shop_name);

    unsigned exit_node = dialogue_tree_add_exit(tree, _("Goodbye!"));
    unsigned trade_text = dialogue_tree_add_text(tree, npc_name,
        _("Take a look at what I have."), exit_node);
    unsigned info_text = dialogue_tree_add_text(tree, npc_name,
        _("I might have heard something. Check back later."), exit_node);

    unsigned choice = dialogue_tree_add_choice(tree, npc_name, greeting);
    dialogue_tree_add_option(tree, choice, _("Buy/Sell"), trade_text);
    dialogue_tree_add_option(tree, choice, _("Any news?"), info_text);
    dialogue_tree_add_option(tree, choice, _("Leave"), exit_node);
}
