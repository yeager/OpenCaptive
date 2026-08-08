#include "droid_ui.h"
#include "i18n.h"
#include "xp_level.h"
#include <limits.h>
#include <string.h>
#include <stdio.h>

static void put_px(uint32_t *p, int w, int h, int x, int y, uint32_t c) {
    if (x >= 0 && x < w && y >= 0 && y < h) p[y * w + x] = c;
}

static void fill(uint32_t *p, int w, int h, int rx, int ry, int rw, int rh, uint32_t c) {
    for (int y = ry; y < ry + rh && y < h; y++) {
        if (y < 0) continue;
        for (int x = rx; x < rx + rw && x < w; x++) {
            if (x < 0) continue;
            p[y * w + x] = c;
        }
    }
}

static const uint8_t ui_font[][5] = {
    ['A'] = {0x7C,0x12,0x12,0x12,0x7C}, ['B'] = {0x7E,0x4A,0x4A,0x4A,0x34},
    ['C'] = {0x3C,0x42,0x42,0x42,0x24}, ['D'] = {0x7E,0x42,0x42,0x42,0x3C},
    ['E'] = {0x7E,0x4A,0x4A,0x4A,0x42}, ['F'] = {0x7E,0x0A,0x0A,0x0A,0x02},
    ['G'] = {0x3C,0x42,0x52,0x52,0x34}, ['H'] = {0x7E,0x08,0x08,0x08,0x7E},
    ['I'] = {0x42,0x42,0x7E,0x42,0x42}, ['J'] = {0x20,0x40,0x42,0x3E,0x02},
    ['K'] = {0x7E,0x08,0x14,0x22,0x40},
    ['L'] = {0x7E,0x40,0x40,0x40,0x40}, ['M'] = {0x7E,0x04,0x08,0x04,0x7E},
    ['N'] = {0x7E,0x04,0x08,0x10,0x7E}, ['O'] = {0x3C,0x42,0x42,0x42,0x3C},
    ['P'] = {0x7E,0x12,0x12,0x12,0x0C}, ['Q'] = {0x3C,0x42,0x52,0x22,0x5C},
    ['R'] = {0x7E,0x12,0x32,0x52,0x0C},
    ['S'] = {0x24,0x4A,0x4A,0x4A,0x30}, ['T'] = {0x02,0x02,0x7E,0x02,0x02},
    ['U'] = {0x3E,0x40,0x40,0x40,0x3E}, ['V'] = {0x1E,0x20,0x40,0x20,0x1E},
    ['W'] = {0x3E,0x40,0x30,0x40,0x3E}, ['X'] = {0x42,0x24,0x18,0x24,0x42},
    ['Y'] = {0x06,0x08,0x70,0x08,0x06}, ['Z'] = {0x62,0x52,0x4A,0x46,0x42},
    ['0'] = {0x3C,0x46,0x4A,0x52,0x3C}, ['1'] = {0x00,0x44,0x7E,0x40,0x00},
    ['2'] = {0x64,0x52,0x52,0x52,0x4C}, ['3'] = {0x22,0x42,0x4A,0x4A,0x36},
    ['4'] = {0x1E,0x10,0x10,0x7E,0x10}, ['5'] = {0x2E,0x4A,0x4A,0x4A,0x32},
    ['6'] = {0x3C,0x4A,0x4A,0x4A,0x30}, ['7'] = {0x02,0x62,0x12,0x0A,0x06},
    ['8'] = {0x34,0x4A,0x4A,0x4A,0x34}, ['9'] = {0x0C,0x52,0x52,0x52,0x3C},
    [' '] = {0,0,0,0,0}, [':'] = {0x00,0x28,0x00,0x00,0x00},
    ['/'] = {0x40,0x20,0x10,0x08,0x04}, ['-'] = {0x08,0x08,0x08,0x00,0x00},
    ['!'] = {0x00,0x00,0x5F,0x00,0x00}, ['.'] = {0x00,0x00,0x00,0x00,0x40},
    ['%'] = {0x62,0x14,0x08,0x14,0x46}, ['_'] = {0x40,0x40,0x40,0x40,0x40},
};

static void draw_txt(uint32_t *fb, int pw, int ph,
                     int x, int y, const char *text, uint32_t color) {
    for (int i = 0; text[i]; i++) {
        unsigned char ch = (unsigned char)text[i];
        if (ch >= sizeof(ui_font)/sizeof(ui_font[0])) continue;
        const uint8_t *glyph = ui_font[ch];
        for (int col = 0; col < 5; col++) {
            uint8_t bits = glyph[col];
            for (int row = 0; row < 7; row++) {
                if (bits & (1 << row))
                    put_px(fb, pw, ph, x + i * 6 + col, y + row, color);
            }
        }
    }
}

void droid_ui_init(DroidUIState *ui, int droid_idx) {
    if (!ui) return;
    memset(ui, 0, sizeof(*ui));
    ui->droid_idx = droid_idx;
    ui->active = true;
}

static const char *body_part_names[] = {
    "HEAD", "TORSO", "L ARM", "R ARM", "L LEG", "R LEG"
};

void droid_ui_render(const DroidUIState *ui, const GameState *gs,
                     const ItemDatabase *db, uint32_t *pixels, int width, int height) {
    if (!ui || !gs || !db || !pixels || width <= 0 || height <= 0 ||
        ui->droid_idx < 0 || ui->droid_idx >= 4) return;
    // Dark overlay
    size_t pixel_count = (size_t)width * (size_t)height;
    for (size_t i = 0; i < pixel_count; i++)
        pixels[i] = (pixels[i] & 0xFF000000) | ((pixels[i] & 0xFEFEFE) >> 1);

    const Droid *d = &gs->droids[ui->droid_idx];

    // Panel background
    int px = 20, py = 10, pw = 280, ph = 180;
    fill(pixels, width, height, px, py, pw, ph, 0xFF0A0A2A);

    // Border
    for (int x = px; x < px + pw; x++) {
        put_px(pixels, width, height, x, py, 0xFF4444AA);
        put_px(pixels, width, height, x, py + ph - 1, 0xFF4444AA);
    }
    for (int y = py; y < py + ph; y++) {
        put_px(pixels, width, height, px, y, 0xFF4444AA);
        put_px(pixels, width, height, px + pw - 1, y, 0xFF4444AA);
    }

    // Droid name and stats
    char buf[64];
    snprintf(buf, sizeof(buf), "DROID %d: %s", ui->droid_idx + 1, d->name);
    draw_txt(pixels, width, height, px + 8, py + 4, buf, 0xFFFFAA00);

    snprintf(buf, sizeof(buf), "HP:%d/%d  EN:%d/%d  LV:%u  XP:%u",
             d->hp, d->hp_max, d->energy, d->energy_max,
             (unsigned)xp_to_display_level(d->xp), (unsigned)d->xp);
    draw_txt(pixels, width, height, px + 8, py + 14, buf, 0xFFAAAAFF);

    // Equipment section
    draw_txt(pixels, width, height, px + 8, py + 28, _("EQUIPMENT"), 0xFF88FF88);

    // Body parts
    for (int i = 0; i < 6; i++) {
        int iy = py + 38 + i * 10;
        bool sel = (ui->ui_mode == DROID_UI_EQUIP && ui->cursor == i);
        if (sel) fill(pixels, width, height, px + 4, iy - 1, pw/2 - 8, 10, 0xFF222266);

        const Item *item = d->body_parts[i] ? item_db_get(db, d->body_parts[i]) : NULL;
        int pct = d->body_part_hp[i] * 100 / 255;
        snprintf(buf, sizeof(buf), "%s: %s %d%%", body_part_names[i],
                 item ? item->name : _("EMPTY"), pct);
        uint32_t color = sel ? 0xFFFFFF00 : (pct > 50 ? 0xFFCCCCCC :
                          (pct > 20 ? 0xFFAAAA00 : 0xFFFF4444));
        draw_txt(pixels, width, height, px + 8, iy, buf, color);
    }

    // Weapons
    for (int i = 0; i < 2; i++) {
        int iy = py + 38 + (6 + i) * 10;
        bool sel = (ui->ui_mode == DROID_UI_EQUIP && ui->cursor == 6 + i);
        if (sel) fill(pixels, width, height, px + 4, iy - 1, pw/2 - 8, 10, 0xFF222266);

        const Item *item = d->weapons[i] ? item_db_get(db, d->weapons[i]) : NULL;
        snprintf(buf, sizeof(buf), "%s HAND: %s", i == 0 ? "L" : "R",
                 item ? item->name : _("EMPTY"));
        draw_txt(pixels, width, height, px + 8, iy, buf,
                 sel ? 0xFFFFFF00 : 0xFFCCCCCC);
    }
    {
        int iy = py + 38 + 8 * 10;
        bool sel = (ui->ui_mode == DROID_UI_EQUIP && ui->cursor == 8);
        if (sel) fill(pixels, width, height, px + 4, iy - 1, pw/2 - 8, 10, 0xFF222266);
        const Item *item = d->shield ? item_db_get(db, d->shield) : NULL;
        snprintf(buf, sizeof(buf), "SHIELD: %s %d/%d",
                 item ? item->name : _("EMPTY"), d->shield_hp,
                 item ? item->defense : 0);
        draw_txt(pixels, width, height, px + 8, iy, buf,
                 sel ? 0xFFFFFF00 : 0xFFCCCCCC);
    }

    // Inventory section (right side)
    draw_txt(pixels, width, height, px + pw/2 + 8, py + 28, _("INVENTORY"), 0xFF88FF88);

    for (int i = 0; i < 10; i++) {
        int iy = py + 38 + i * 10;
        bool sel = (ui->ui_mode == DROID_UI_INVENTORY && ui->cursor == i);
        if (sel) fill(pixels, width, height, px + pw/2 + 4, iy - 1, pw/2 - 8, 10, 0xFF222266);

        const Item *item = d->items[i] ? item_db_get(db, d->items[i]) : NULL;
        snprintf(buf, sizeof(buf), "%d: %s", i + 1, item ? item->name : "---");
        draw_txt(pixels, width, height, px + pw/2 + 8, iy, buf,
                 sel ? 0xFFFFFF00 : 0xFFAAAAAA);
    }

    // Help text
    draw_txt(pixels, width, height, px + 8, py + ph - 12,
             _("TAB:SWITCH ENTER:EQUIP/USE ESC:CLOSE"), 0xFF666688);
}

void droid_recalc_weapon_damage(Droid *d, const ItemDatabase *db) {
    if (!d || !db) return;
    uint16_t best = 0;
    int best_damage = 0;
    for (int w = 0; w < 2; w++) {
        if (d->weapons[w] == 0) continue;
        const Item *item = item_db_get(db, d->weapons[w]);
        if (!item) continue;
        /* Combat stores both components in one byte.  Do not let a malformed
         * signed database value wrap into a high-damage weapon. */
        if (item->damage_min < 0 || item->damage_min > UINT8_MAX ||
            item->damage_max < 0 || item->damage_max > UINT8_MAX)
            continue;
        uint8_t lo = (uint8_t)item->damage_min;
        uint8_t hi = (uint8_t)item->damage_max;
        uint16_t encoded = (uint16_t)lo | ((uint16_t)hi << 8);
        int damage = (int)lo * (int)hi;
        if (damage > best_damage || (damage == best_damage && encoded > best)) {
            best_damage = damage;
            best = encoded;
        }
    }
    d->weapon_damage = best;
}

bool droid_ui_handle_key(DroidUIState *ui, GameState *gs, const ItemDatabase *db, int key) {
    if (!ui || !gs || !db || !ui->active || ui->droid_idx < 0 || ui->droid_idx >= 4 ||
        ui->cursor < 0 ||
        (ui->ui_mode != DROID_UI_EQUIP && ui->ui_mode != DROID_UI_INVENTORY)) return false;
    int max_cursor = (ui->ui_mode == DROID_UI_EQUIP) ? 8 : 9;
    if (ui->cursor > max_cursor) return false;

    switch (key) {
        case 0x50:
            if (ui->cursor > 0) ui->cursor--;
            return true;
        case 0x51:
            if (ui->cursor < max_cursor) ui->cursor++;
            return true;
        case 0x09:
            ui->ui_mode = (ui->ui_mode == DROID_UI_EQUIP) ?
                          DROID_UI_INVENTORY : DROID_UI_EQUIP;
            ui->cursor = 0;
            return true;
        case 0x0D: {
            Droid *d = &gs->droids[ui->droid_idx];
            if (ui->ui_mode == DROID_UI_INVENTORY) {
                uint8_t item_id = d->items[ui->cursor];
                if (item_id == 0) return true;
                const Item *use_item = item_db_get(db, item_id);
                if (!use_item) return false;
                if (use_item && use_item->category == ITEM_BATTERY) {
                    if (d->energy < 0 || d->energy_max < 0 ||
                        d->energy > d->energy_max) return false;
                    int charged = (int)d->energy + 50;
                    if (charged > d->energy_max) charged = d->energy_max;
                    if (charged > INT16_MAX) charged = INT16_MAX;
                    d->energy = (int16_t)charged;
                    d->items[ui->cursor] = 0;
                    return true;
                }
                if (use_item && use_item->category >= ITEM_ARMOR_HEAD &&
                    use_item->category <= ITEM_ARMOR_HAND) {
                    int slot = use_item->category - ITEM_ARMOR_HEAD;
                    if (slot >= 0 && slot < 6) {
                        uint8_t old = d->body_parts[slot];
                        d->body_parts[slot] = item_id;
                        d->body_part_hp[slot] = 255;
                        d->items[ui->cursor] = old;
                        return true;
                    }
                }
                if (use_item->category == ITEM_SHIELD) {
                    uint8_t old = d->shield;
                    d->shield = item_id;
                    d->shield_hp = use_item->defense > INT16_MAX ? INT16_MAX :
                                   use_item->defense;
                    d->items[ui->cursor] = old;
                    return true;
                }
                if (use_item->category < ITEM_WEAPON_MELEE ||
                    use_item->category > ITEM_WEAPON_SPRAY)
                    return false;
                for (int w = 0; w < 2; w++) {
                    if (d->weapons[w] == 0) {
                        d->weapons[w] = item_id;
                        d->items[ui->cursor] = 0;
                        droid_recalc_weapon_damage(d, db);
                        return true;
                    }
                }
                uint8_t old = d->weapons[0];
                d->weapons[0] = item_id;
                d->items[ui->cursor] = old;
                droid_recalc_weapon_damage(d, db);
            } else if (ui->cursor < 6) {
                uint8_t item_id = d->body_parts[ui->cursor];
                if (item_id == 0) return true;
                int empty_slot = -1;
                for (int i = 0; i < 10; i++) {
                    if (d->items[i] == 0) {
                        empty_slot = i;
                        break;
                    }
                }
                /* Do not remove an equipped body part unless its replacement
                 * has somewhere to go.  The old one was silently discarded
                 * when all inventory slots were occupied. */
                if (empty_slot < 0) return false;
                d->items[empty_slot] = item_id;
                d->body_parts[ui->cursor] = 0;
                return true;
            } else if (ui->cursor < 8) {
                int wi = ui->cursor - 6;
                uint8_t item_id = d->weapons[wi];
                if (item_id == 0) return true;
                for (int i = 0; i < 10; i++) {
                    if (d->items[i] == 0) {
                        d->items[i] = item_id;
                        d->weapons[wi] = 0;
                        droid_recalc_weapon_damage(d, db);
                        return true;
                    }
                }
            } else {
                if (d->shield == 0) return true;
                for (int i = 0; i < 10; i++) {
                    if (d->items[i] == 0) {
                        d->items[i] = d->shield;
                        d->shield = 0;
                        d->shield_hp = 0;
                        return true;
                    }
                }
                return false;
            }
            return true;
        }
        default:
            return false;
    }
}

bool droid_ui_handle_click(DroidUIState *ui, GameState *gs, const ItemDatabase *db,
                           int mx, int my, int width, int height) {
    if (!ui || !gs || !db || !ui->active) return false;
    (void)width; (void)height;
    int px = 20, py = 10, pw = 280;
    int slot_y_start = py + 38;

    if (my >= py + 4 && my < py + 12) {
        int dx = mx - px;
        if (dx >= 0 && dx < pw) {
            int di = dx / (pw / 4);
            if (di >= 0 && di < 4) {
                gs->selected_droid = di;
                ui->droid_idx = di;
                return true;
            }
        }
    }

    if (my < slot_y_start - 1 || my >= slot_y_start + 10 * 10) return false;
    int row = (my - slot_y_start + 1) / 10;
    if (row < 0 || row > 9) return false;

    if (mx >= px + 4 && mx < px + pw / 2) {
        ui->ui_mode = DROID_UI_EQUIP;
        if (row <= 8) {
            ui->cursor = row;
            return droid_ui_handle_key(ui, gs, db, 0x0D);
        }
        return false;
    } else if (mx >= px + pw / 2 + 4 && mx < px + pw - 4) {
        ui->ui_mode = DROID_UI_INVENTORY;
        if (row <= 9) ui->cursor = row;
        return droid_ui_handle_key(ui, gs, db, 0x0D);
    }
    return false;
}
