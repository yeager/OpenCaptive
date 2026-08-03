#include "shop.h"
#include "captive_data.h"
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
    memset(shop, 0, sizeof(*shop));
    shop->active = true;
    shop_seed = seed + level * 997;

    // Stock varies by level — higher levels have better items
    int max_tier = 1 + level / 2;
    if (max_tier > 7) max_tier = 7;

    for (int i = 0; i < db->num_defs && shop->num_items < SHOP_MAX_ITEMS; i++) {
        if (db->defs[i].tier <= max_tier && captive_prng(&shop_seed) % 3 == 0) {
            shop->item_ids[shop->num_items++] = db->defs[i].id;
        }
    }
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

void shop_render(const ShopState *shop, const ItemDatabase *db,
                 uint32_t *pixels, int width, int height) {
    // Dark overlay
    for (int i = 0; i < width * height; i++)
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
    for (int i = 0; i < shop->num_items && i < 12; i++) {
        const Item *item = item_db_get(db, shop->item_ids[i]);
        if (!item) continue;

        int iy = list_y + i * 10;
        bool selected = (i == shop->selected);

        if (selected)
            fill_rect_s(pixels, width, height, px + 4, iy, pw - 8, 10, 0xFF222266);

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
    if (shop->selected < 0 || shop->selected >= shop->num_items) return false;

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
    if (droid_idx < 0 || droid_idx >= 4) return false;
    Droid *d = &gs->droids[droid_idx];
    if (d->hp >= d->hp_max && d->energy >= d->energy_max) return false;

    int damage = (d->hp_max - d->hp) + (d->energy_max - d->energy);
    int cost = damage * 2;
    if (cost < 10) cost = 10;
    if (shop->gold < cost) return false;

    shop->gold -= cost;
    d->hp = d->hp_max;
    d->energy = d->energy_max;
    for (int i = 0; i < 6; i++) d->body_parts[i] = 1;
    return true;
}
