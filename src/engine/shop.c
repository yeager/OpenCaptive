#include "shop.h"
#include "captive_data.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>

/* Original shop strings from CAPPO.EXE:
 *   "WELCOME STRANGER TO MY"
 *   "'HUMBLE SHOP"
 *   "HOW MAY I BE OF ASSISTANCE"
 *   "CALL AGAIN LATER"
 *   "THIS WILL COST YOU"
 *   "FOR NEXT LEVEL IN"
 *   "YOU DO NOT HAVE ENOUGH!"
 *   "ACCEPT"
 *   "I DON'T STOCK THIS OBJECT"
 *   "MODEL OF"
 *   "PLEASE REMOVE"
 *   "GRADE OF THE"
 *   "REPAIR"
 */

static uint32_t shop_seed;

void shop_init(ShopState *shop, const ItemDatabase *db, int level, uint32_t seed) {
    if (!shop) return;
    memset(shop, 0, sizeof(*shop));
    if (!db) return;
    if (level < 0) level = 0;
    shop->active = true;
    /* Keep malformed caller levels from invoking signed-overflow UB. */
    shop_seed = (uint32_t)((uint64_t)seed +
                           (uint64_t)(int64_t)level * UINT64_C(997));

    // Stock varies by level — higher levels have better items
    int max_tier = 1 + level / 2;
    if (max_tier > 7) max_tier = 7;

    int def_count = db->num_defs;
    if (def_count < 0) def_count = 0;
    if (def_count > MAX_ITEM_DEFS) def_count = MAX_ITEM_DEFS;
    /* The Captive shop renderer and keyboard navigation expose only the
     * visible list. Do not generate hidden stock that the player can never
     * select until a scrolling UI exists. */
    for (int i = 0; i < def_count && shop->num_items < SHOP_VISIBLE_ITEMS; i++) {
        if (db->defs[i].tier <= max_tier && captive_prng(&shop_seed) % 3 == 0) {
            shop->item_ids[shop->num_items++] = db->defs[i].id;
        }
    }
}

int shop_next_selection(const ShopState *shop) {
    if (!shop || shop->num_items <= 0) return 0;
    int visible_items = shop->num_items;
    if (visible_items > SHOP_VISIBLE_ITEMS) visible_items = SHOP_VISIBLE_ITEMS;
    int selected = shop->selected;
    if (selected < 0) selected = 0;
    if (selected >= visible_items) selected = visible_items - 1;
    if (selected + 1 < visible_items) selected++;
    return selected;
}

static void put_pixel_s(uint32_t *pixels, int w, int h, int x, int y, uint32_t c) {
    if (x >= 0 && x < w && y >= 0 && y < h)
        pixels[y * w + x] = c;
}

static void fill_rect_s(uint32_t *pixels, int w, int h,
                        int rx, int ry, int rw, int rh, uint32_t c) {
    for (int y = ry; y < ry + rh && y < h; y++) {
        if (y < 0) continue;
        for (int x = rx; x < rx + rw && x < w; x++) {
            if (x < 0) continue;
            pixels[y * w + x] = c;
        }
    }
}

// Tiny 3x5 font for shop (just digits and uppercase)
static const uint8_t tiny_font[][5] = {
    ['0'-'0'] = {0x7,0x5,0x5,0x5,0x7},
    ['1'-'0'] = {0x2,0x6,0x2,0x2,0x7},
    ['2'-'0'] = {0x7,0x1,0x7,0x4,0x7},
    ['3'-'0'] = {0x7,0x1,0x7,0x1,0x7},
    ['4'-'0'] = {0x5,0x5,0x7,0x1,0x1},
    ['5'-'0'] = {0x7,0x4,0x7,0x1,0x7},
    ['6'-'0'] = {0x7,0x4,0x7,0x5,0x7},
    ['7'-'0'] = {0x7,0x1,0x1,0x1,0x1},
    ['8'-'0'] = {0x7,0x5,0x7,0x5,0x7},
    ['9'-'0'] = {0x7,0x5,0x7,0x1,0x7},
};

static void draw_small_num(uint32_t *pixels, int w, int h,
                           int x, int y, int value, uint32_t color) {
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", value);
    for (int i = 0; buf[i]; i++) {
        int d = buf[i] - '0';
        if (d < 0 || d > 9) continue;
        for (int gy = 0; gy < 5; gy++)
            for (int gx = 0; gx < 3; gx++)
                if (tiny_font[d][gy] & (0x4 >> gx))
                    put_pixel_s(pixels, w, h, x + i*4 + gx, y + gy, color);
    }
}

/* The shop originally rendered only the numeric price, leaving two or more
 * equally priced entries indistinguishable.  Keep the compact native-style
 * font, but add the uppercase glyphs needed by the item names in the
 * Captive database. */
static uint8_t shop_letter_glyph(char ch, int row) {
    static const uint8_t glyphs[26][5] = {
        {0x7,0x5,0x7,0x5,0x5}, {0x6,0x5,0x6,0x5,0x6},
        {0x7,0x4,0x4,0x4,0x7}, {0x6,0x5,0x5,0x5,0x6},
        {0x7,0x4,0x6,0x4,0x7}, {0x7,0x4,0x6,0x4,0x4},
        {0x7,0x4,0x5,0x5,0x7}, {0x5,0x5,0x7,0x5,0x5},
        {0x7,0x2,0x2,0x2,0x7}, {0x1,0x1,0x1,0x5,0x7},
        {0x5,0x5,0x6,0x5,0x5}, {0x4,0x4,0x4,0x4,0x7},
        {0x5,0x7,0x7,0x5,0x5}, {0x5,0x7,0x7,0x7,0x5},
        {0x7,0x5,0x5,0x5,0x7}, {0x6,0x5,0x6,0x4,0x4},
        {0x7,0x5,0x7,0x3,0x1}, {0x6,0x5,0x6,0x5,0x5},
        {0x7,0x4,0x7,0x1,0x7}, {0x7,0x2,0x2,0x2,0x2},
        {0x5,0x5,0x5,0x5,0x7}, {0x5,0x5,0x5,0x5,0x2},
        {0x5,0x5,0x7,0x7,0x5}, {0x5,0x5,0x2,0x5,0x5},
        {0x5,0x5,0x2,0x2,0x2}, {0x7,0x1,0x2,0x4,0x7},
    };
    if (ch >= 'A' && ch <= 'Z' && row >= 0 && row < 5)
        return glyphs[ch - 'A'][row];
    if (ch == '-') return row == 2 ? 0x7 : 0;
    if (ch == ' ') return 0;
    return 0;
}

static void draw_small_text(uint32_t *pixels, int w, int h,
                            int x, int y, const char *text, uint32_t color) {
    if (!pixels || !text) return;
    for (int i = 0; text[i] && i < 32; i++) {
        for (int row = 0; row < 5; row++) {
            uint8_t bits = shop_letter_glyph(text[i], row);
            for (int col = 0; col < 3; col++)
                if (bits & (0x4 >> col))
                    put_pixel_s(pixels, w, h, x + i * 4 + col, y + row, color);
        }
    }
}

void shop_render(const ShopState *shop, const ItemDatabase *db,
                 uint32_t *pixels, int width, int height) {
    if (!shop || !db || !pixels || width <= 0 || height <= 0) return;
    // Dark overlay
    size_t pixel_count = (size_t)width * (size_t)height;
    for (size_t i = 0; i < pixel_count; i++)
        pixels[i] = (pixels[i] & 0xFF000000) | ((pixels[i] & 0xFEFEFE) >> 1);

    // Shop panel
    int px = 30, py = 20, pw = 260, ph = 160;
    fill_rect_s(pixels, width, height, px, py, pw, ph, 0xFF111133);

    // Border
    for (int x = px; x < px + pw; x++) {
        put_pixel_s(pixels, width, height, x, py, 0xFF5555AA);
        put_pixel_s(pixels, width, height, x, py + ph - 1, 0xFF5555AA);
    }
    for (int y = py; y < py + ph; y++) {
        put_pixel_s(pixels, width, height, px, y, 0xFF5555AA);
        put_pixel_s(pixels, width, height, px + pw - 1, y, 0xFF5555AA);
    }

    // Title "SHOP"
    fill_rect_s(pixels, width, height, px + pw/2 - 12, py + 4, 24, 8, 0xFFFFAA00);

    // Item list
    int list_y = py + 16;
    int item_count = shop->num_items;
    if (item_count < 0) item_count = 0;
    if (item_count > SHOP_MAX_ITEMS) item_count = SHOP_MAX_ITEMS;
    for (int i = 0; i < item_count && i < SHOP_VISIBLE_ITEMS; i++) {
        const Item *item = item_db_get(db, shop->item_ids[i]);
        if (!item) continue;

        int iy = list_y + i * 10;
        bool selected = (i == shop->selected);

        if (selected)
            fill_rect_s(pixels, width, height, px + 4, iy, pw - 8, 10, 0xFF222266);

        draw_small_text(pixels, width, height, px + 10, iy + 2,
                        item->name, selected ? 0xFFFFFF00 : 0xFFCCCCCC);

        // Price
        uint32_t price_col = selected ? 0xFFFFFF00 : 0xFFAAAA00;
        draw_small_num(pixels, width, height, px + pw - 30, iy + 2, item->price, price_col);
    }

    // Gold display
    draw_small_num(pixels, width, height, px + 8, py + ph - 12, shop->gold, 0xFFFFFF00);

    // Repair hint
    fill_rect_s(pixels, width, height, px + pw/2 - 20, py + ph - 12, 40, 8, 0xFF334433);
}

bool shop_buy(ShopState *shop, const ItemDatabase *db, GameState *gs) {
    if (!shop || !shop->active || !db || !gs || gs->selected_droid < 0 || gs->selected_droid >= 4)
        return false;
    if (shop->num_items < 0 || shop->num_items > SHOP_MAX_ITEMS ||
        shop->selected < 0 || shop->selected >= shop->num_items) return false;

    const Item *item = item_db_get(db, shop->item_ids[shop->selected]);
    if (!item || shop->gold < item->price) return false;

    shop->gold -= item->price;

    // Add to selected droid's inventory
    Droid *d = &gs->droids[gs->selected_droid];
    for (int i = 0; i < 10; i++) {
        if (d->items[i] == 0) {
            d->items[i] = item->id;
            return true;
        }
    }
    // Inventory full
    shop->gold += item->price;
    return false;
}

bool shop_repair(ShopState *shop, GameState *gs, int droid_idx) {
    if (!shop || !gs || droid_idx < 0 || droid_idx >= 4) return false;
    Droid *d = &gs->droids[droid_idx];
    if (d->hp_max < 0 || d->hp < 0 || d->hp > d->hp_max ||
        d->energy_max < 0 || d->energy < 0 || d->energy > d->energy_max)
        return false;
    bool needs_repair = (d->hp < d->hp_max || d->energy < d->energy_max);
    for (int i = 0; i < 6 && !needs_repair; i++)
        if (d->body_part_hp[i] < 255) needs_repair = true;
    if (!needs_repair) return false;

    int64_t damage = (int64_t)(d->hp_max - d->hp) +
                     (int64_t)(d->energy_max - d->energy);
    int64_t cost = damage * 2;
    if (cost < 10) cost = 10;
    if (cost > shop->gold) return false;

    shop->gold -= (int)cost;
    d->hp = d->hp_max;
    d->energy = d->energy_max;
    for (int i = 0; i < 6; i++)
        d->body_part_hp[i] = 255;
    return true;
}
