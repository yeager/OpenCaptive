#include "start_menu.h"
#include "opencaptive.h"
#include "i18n.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <SDL3_ttf/SDL_ttf.h>

extern unsigned char *load_png_file(const char *path, int *w, int *h);

static uint32_t *load_card_image(const char *filename, int *w, int *h) {
    char path[1024];
    const char *base = SDL_GetBasePath();
    if (base) {
        snprintf(path, sizeof(path), "%s%s", base, filename);
        unsigned char *rgba = load_png_file(path, w, h);
        if (rgba) return (uint32_t *)rgba;
    }
    snprintf(path, sizeof(path), "./%s", filename);
    unsigned char *rgba = load_png_file(path, w, h);
    if (rgba) return (uint32_t *)rgba;

    const char *home = getenv("HOME");
    if (home) {
        snprintf(path, sizeof(path), "%s/Downloads/%s", home, filename);
        rgba = load_png_file(path, w, h);
        if (rgba) return (uint32_t *)rgba;
    }
    return NULL;
}

static TTF_Font *load_font(float pt) {
    char path[1024];
    const char *base = SDL_GetBasePath();
    const char *names[] = {
        "data/DejaVuSansMono-Bold.ttf",
        "../data/DejaVuSansMono-Bold.ttf",
        NULL
    };
    for (int i = 0; names[i]; i++) {
        if (base) {
            snprintf(path, sizeof(path), "%s%s", base, names[i]);
            TTF_Font *f = TTF_OpenFont(path, pt);
            if (f) return f;
        }
        TTF_Font *f = TTF_OpenFont(names[i], pt);
        if (f) return f;
    }
    const char *home = getenv("HOME");
    if (home) {
        snprintf(path, sizeof(path), "%s/.local/share/opencaptive/DejaVuSansMono-Bold.ttf", home);
        TTF_Font *f = TTF_OpenFont(path, pt);
        if (f) return f;
    }
    return NULL;
}

static void blit_scaled(uint32_t *dst, int dw, int dh,
                        int dx, int dy, int tw, int th,
                        const uint32_t *src, int sw, int sh) {
    for (int y = 0; y < th; y++) {
        int sy = y * sh / th;
        if (sy >= sh) sy = sh - 1;
        for (int x = 0; x < tw; x++) {
            int sx = x * sw / tw;
            if (sx >= sw) sx = sw - 1;
            int px = dx + x, py = dy + y;
            if (px >= 0 && px < dw && py >= 0 && py < dh) {
                uint32_t c = src[sy * sw + sx];
                uint32_t r = c & 0xFF;
                uint32_t g = (c >> 8) & 0xFF;
                uint32_t b = (c >> 16) & 0xFF;
                uint32_t a = (c >> 24) & 0xFF;
                if (a > 0)
                    dst[py * dw + px] = 0xFF000000 | (r << 16) | (g << 8) | b;
            }
        }
    }
}

static void ttf_text(uint32_t *pixels, int pw, int ph,
                     int x, int y, const char *text,
                     TTF_Font *font, uint32_t color) {
    if (!font || !text || !text[0]) return;
    SDL_Color c = {
        (uint8_t)((color >> 16) & 0xFF),
        (uint8_t)((color >> 8) & 0xFF),
        (uint8_t)(color & 0xFF),
        255
    };
    SDL_Surface *surf = TTF_RenderText_Blended(font, text, 0, c);
    if (!surf) return;
    uint32_t *sp = (uint32_t *)surf->pixels;
    for (int sy = 0; sy < surf->h; sy++) {
        for (int sx = 0; sx < surf->w; sx++) {
            int px = x + sx, py2 = y + sy;
            if (px < 0 || px >= pw || py2 < 0 || py2 >= ph) continue;
            uint32_t sc = sp[sy * (surf->pitch / 4) + sx];
            uint32_t sa = (sc >> 24) & 0xFF;
            if (sa == 0) continue;
            uint32_t sr = (sc >> 16) & 0xFF;
            uint32_t sg = (sc >> 8) & 0xFF;
            uint32_t sb = sc & 0xFF;
            if (sa == 255) {
                pixels[py2 * pw + px] = 0xFF000000 | (sr << 16) | (sg << 8) | sb;
            } else {
                uint32_t dc = pixels[py2 * pw + px];
                uint32_t dr = (dc >> 16) & 0xFF;
                uint32_t dg = (dc >> 8) & 0xFF;
                uint32_t db = dc & 0xFF;
                dr = (sr * sa + dr * (255 - sa)) / 255;
                dg = (sg * sa + dg * (255 - sa)) / 255;
                db = (sb * sa + db * (255 - sa)) / 255;
                pixels[py2 * pw + px] = 0xFF000000 | (dr << 16) | (dg << 8) | db;
            }
        }
    }
    SDL_DestroySurface(surf);
}

static void ttf_text_centered(uint32_t *pixels, int pw, int ph,
                               int y, const char *text,
                               TTF_Font *font, uint32_t color) {
    if (!font || !text || !text[0]) return;
    int tw = 0, th = 0;
    TTF_GetStringSize(font, text, 0, &tw, &th);
    ttf_text(pixels, pw, ph, (pw - tw) / 2, y, text, font, color);
}

static const char *const lang_codes[] = {
    "en", "sv", "cs", "da", "de", "es", "fi", "fr", "hu", "it",
    "ja", "ko", "nl", "no", "pl", "pt", "ro", "ru", "zh",
};
static const char *const lang_labels[] = {
    "English", "Svenska", "\xc4\x8c" "e\xc5\xa1tina", "Dansk", "Deutsch",
    "Espa\xc3\xb1ol", "Suomi", "Fran\xc3\xa7" "ais", "Magyar", "Italiano",
    "\xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e", "\xed\x95\x9c\xea\xb5\xad\xec\x96\xb4",
    "Nederlands", "Norsk", "Polski",
    "Portugu\xc3\xaas", "Rom\xc3\xa2n\xc4\x83", "\xd0\xa0\xd1\x83\xd1\x81\xd1\x81\xd0\xba\xd0\xb8\xd0\xb9",
    "\xe4\xb8\xad\xe6\x96\x87",
};
#define LANG_COUNT 19

static void draw_rect(uint32_t *pixels, int pw, int ph,
                      int x, int y, int w, int h, uint32_t color) {
    for (int ry = y; ry < y + h && ry < ph; ry++) {
        if (ry < 0) continue;
        for (int rx = x; rx < x + w && rx < pw; rx++) {
            if (rx < 0) continue;
            pixels[ry * pw + rx] = color;
        }
    }
}

static void draw_border(uint32_t *pixels, int pw, int ph,
                        int x, int y, int w, int h, uint32_t color, int t) {
    draw_rect(pixels, pw, ph, x, y, w, t, color);
    draw_rect(pixels, pw, ph, x, y + h - t, w, t, color);
    draw_rect(pixels, pw, ph, x, y, t, h, color);
    draw_rect(pixels, pw, ph, x + w - t, y, t, h, color);
}

static uint32_t hash_pixel(int x, int y, int seed) {
    uint32_t h = (uint32_t)(x * 374761393 + y * 668265263 + seed * 1274126177);
    h = (h ^ (h >> 13)) * 1103515245;
    return h ^ (h >> 16);
}

static void draw_captive_card(uint32_t *pixels, int pw, int ph,
                              int cx, int cy, int cw, int ch,
                              bool selected, uint32_t tick) {
    uint32_t ceil_col   = 0xFF1A1020;
    uint32_t floor_dark = 0xFF1E1228;
    uint32_t floor_tile = 0xFF281838;
    uint32_t wall_far   = 0xFF2A1A3A;
    uint32_t wall_side  = 0xFF3D2D50;
    uint32_t wall_edge  = 0xFF504068;
    uint32_t door_col   = 0xFF5A4A30;
    uint32_t door_frame = 0xFF6A5A40;
    uint32_t torch_stem = 0xFF5A4A30;
    int mid_x = cw / 2, mid_y = ch / 2;
    int vp_l = cw / 4, vp_r = cw * 3 / 4, vp_t = ch / 4, vp_b = ch * 3 / 4;
    for (int y = 0; y < ch; y++) {
        for (int x = 0; x < cw; x++) {
            int px = cx + x, py = cy + y;
            if (px < 0 || px >= pw || py < 0 || py >= ph) continue;
            uint32_t col;
            bool in_vp = (x >= vp_l && x <= vp_r && y >= vp_t && y <= vp_b);
            if (in_vp) {
                int dl = mid_x - cw/10, dr = mid_x + cw/10, dt = mid_y - ch/8;
                if (x >= dl && x <= dr && y >= dt && y <= vp_b) {
                    col = (x == dl || x == dr || y == dt) ? door_frame : door_col;
                    if (x == dr - 2 && y == mid_y + 2) col = 0xFFAA8844;
                } else {
                    int bx = (x - vp_l) % 8, by = (y - vp_t) % 6;
                    col = (bx == 0 || by == 0) ? wall_edge : wall_far;
                }
            } else if (y < mid_y) {
                if (x < vp_l) {
                    int d = vp_l - x, v = y - (vp_t - (vp_t * d / vp_l));
                    if (v < 0) col = ceil_col;
                    else { int bx = d%10, by = v%8; col = (bx==0||by==0) ? wall_edge : wall_side; }
                } else if (x > vp_r) {
                    int d = x - vp_r, md = cw-1-vp_r;
                    int v = y - (vp_t - (vp_t * d / (md?md:1)));
                    if (v < 0) col = ceil_col;
                    else { int bx = d%10, by = v%8; col = (bx==0||by==0) ? wall_edge : wall_side; }
                } else col = ceil_col;
            } else {
                if (x < vp_l) {
                    int d = vp_l-x, v = (vp_b+((ch-1-vp_b)*d/vp_l))-y;
                    if (v < 0) col = floor_dark;
                    else { int bx = d%10, by = v%8; col = (bx==0||by==0) ? wall_edge : wall_side; }
                } else if (x > vp_r) {
                    int d = x-vp_r, md = cw-1-vp_r;
                    int v = (vp_b+((ch-1-vp_b)*d/(md?md:1)))-y;
                    if (v < 0) col = floor_dark;
                    else { int bx = d%10, by = v%8; col = (bx==0||by==0) ? wall_edge : wall_side; }
                } else {
                    int dp = y-vp_b, th2 = 3+dp/4, tw2 = 8+dp;
                    if (th2<1) th2=1; if (tw2<1) tw2=1;
                    col = ((x-vp_l)%tw2==0 || dp%th2==0) ? floor_dark : floor_tile;
                }
            }
            int tlx=vp_l-8,trx=vp_r+8,tty=vp_t+4;
            if (x>=tlx&&x<=tlx+2&&y>=tty&&y<=tty+8) col=torch_stem;
            if (x>=trx-2&&x<=trx&&y>=tty&&y<=tty+8) col=torch_stem;
            int fl=(int)((tick/3)%5);
            if (x>=tlx-1&&x<=tlx+3&&y>=tty-4-fl&&y<tty) {
                int fy=tty-y; uint32_t g2=(uint32_t)(0xCC-fy*0x18);
                if (fy>3) g2=0x44;
                col=0xFF000000|(0xFF<<16)|(g2<<8);
            }
            int fr=(int)((tick/3+2)%5);
            if (x>=trx-3&&x<=trx+1&&y>=tty-4-fr&&y<tty) {
                int fy=tty-y; uint32_t g2=(uint32_t)(0xCC-fy*0x18);
                if (fy>3) g2=0x44;
                col=0xFF000000|(0xFF<<16)|(g2<<8);
            }
            pixels[py*pw+px] = col;
        }
    }
    if (selected) {
        uint32_t sc = ((tick/8)%2) ? 0xFFFF8800 : 0xFFFFAA44;
        draw_border(pixels, pw, ph, cx, cy, cw, ch, sc, 3);
    } else {
        draw_border(pixels, pw, ph, cx, cy, cw, ch, 0xFF444466, 2);
    }
}

static void draw_liberation_card(uint32_t *pixels, int pw, int ph,
                                 int cx, int cy, int cw, int ch,
                                 bool selected, uint32_t tick) {
    static const struct { int x8,w8,h; uint32_t col; } bldgs[] = {
        {2,14,38,0xFF283048},{18,20,28,0xFF2A3050},{40,12,45,0xFF303858},
        {54,18,22,0xFF282848},{74,22,32,0xFF2C3454},{98,14,18,0xFF262840},
    };
    int horizon = ch*3/5;
    for (int y = 0; y < ch; y++) {
        for (int x = 0; x < cw; x++) {
            int px=cx+x,py=cy+y;
            if (px<0||px>=pw||py<0||py>=ph) continue;
            uint32_t col;
            if (y < horizon) {
                int t=y*256/horizon;
                uint32_t r=(0x06*(256-t)+0x18*t)>>8;
                uint32_t g=(0x06*(256-t)+0x10*t)>>8;
                uint32_t b=(0x20*(256-t)+0x38*t)>>8;
                col=0xFF000000|(r<<16)|(g<<8)|b;
                if (y<horizon/3) { uint32_t sh=hash_pixel(x,y,7); if (sh%180==0) { int tw2=(int)((tick/12+sh)%6); if(tw2<4) col=(tw2<2)?0xFFDDDDEE:0xFF888899; } }
                int x8=x*128/cw;
                for (int bi=0;bi<6;bi++) {
                    int bl=bldgs[bi].x8,br=bl+bldgs[bi].w8,bt=horizon-bldgs[bi].h;
                    if (x8>=bl&&x8<br&&y>=bt) {
                        int lx=x8-bl,ly=y-bt;
                        if (lx==0) col=0xFF404868; else if (lx==bldgs[bi].w8-1) col=0xFF1A1A30;
                        else if (ly==0) col=0xFF505878;
                        else { col=bldgs[bi].col; int wx=(lx-2)%5,wy=(ly-2)%4; if(lx>=2&&lx<bldgs[bi].w8-2&&ly>=2&&wx<3&&wy<2){uint32_t wh=hash_pixel(lx/5,ly/4,bi*13+50);int ph2=(int)((tick/40+wh)%12);if(ph2<5)col=0xFFEECC55;else if(ph2<7)col=0xFF88AADD;else col=0xFF101024;}}
                        break;
                    }
                }
            } else {
                int ry=y-horizon;
                if (ry<2) col=0xFF383848;
                else { col=0xFF141420; if(ry>=2&&ry<=4)col=0xFF303040; if(ry>=8&&ry<=9&&(x%16)<8)col=0xFF555540; }
            }
            pixels[py*pw+px]=col;
        }
    }
    if (selected) {
        uint32_t sc=((tick/8)%2)?0xFF4488FF:0xFF66AAFF;
        draw_border(pixels,pw,ph,cx,cy,cw,ch,sc,3);
    } else draw_border(pixels,pw,ph,cx,cy,cw,ch,0xFF444466,2);
}

void start_menu_init(StartMenu *menu) {
    char saved_path[512];
    memcpy(saved_path, menu->data_path, sizeof(saved_path));
    int saved_cursor = menu->data_path_cursor;
    uint32_t *logo_img = menu->logo_img;
    int logo_w = menu->logo_img_w, logo_h = menu->logo_img_h;
    uint32_t *cap_img = menu->captive_img;
    int cap_w = menu->captive_img_w, cap_h = menu->captive_img_h;
    uint32_t *lib_img = menu->liberation_img;
    int lib_w = menu->liberation_img_w, lib_h = menu->liberation_img_h;
    TTF_Font *ft = menu->font_title, *fb = menu->font_body, *fs = menu->font_small;
    bool ttf_ok = menu->ttf_ready;

    memset(menu, 0, sizeof(*menu));
    memcpy(menu->data_path, saved_path, sizeof(menu->data_path));
    menu->data_path_cursor = saved_cursor ? saved_cursor : (int)strlen(menu->data_path);
    menu->num_items = 4;
    menu->platform = CAPTIVE_PLATFORM_DOS;
    menu->music_enabled = true;
    menu->sfx_enabled = true;
    menu->scale_factor = 3;
    menu->vsync = true;
    menu->integer_scaling = true;
    menu->brightness = 50;
    menu->contrast = 50;
    menu->fps_limit = 60;

    menu->logo_img = logo_img; menu->logo_img_w = logo_w; menu->logo_img_h = logo_h;
    menu->captive_img = cap_img; menu->captive_img_w = cap_w; menu->captive_img_h = cap_h;
    menu->liberation_img = lib_img; menu->liberation_img_w = lib_w; menu->liberation_img_h = lib_h;
    menu->font_title = ft; menu->font_body = fb; menu->font_small = fs;
    menu->ttf_ready = ttf_ok;

    if (!menu->logo_img)
        menu->logo_img = load_card_image("captivelogo.png",
            &menu->logo_img_w, &menu->logo_img_h);
    if (!menu->captive_img)
        menu->captive_img = load_card_image("captive.png",
            &menu->captive_img_w, &menu->captive_img_h);
    if (!menu->liberation_img)
        menu->liberation_img = load_card_image("liberation.png",
            &menu->liberation_img_w, &menu->liberation_img_h);

    if (!menu->ttf_ready) {
        if (!TTF_WasInit()) TTF_Init();
        menu->font_title = load_font(36);
        menu->font_body  = load_font(18);
        menu->font_small = load_font(14);
        menu->ttf_ready = true;
    }

    const char *cur_lang = i18n_get_lang();
    menu->lang_index = 0;
    for (int i = 0; i < LANG_COUNT; i++) {
        if (strcmp(lang_codes[i], cur_lang) == 0) {
            menu->lang_index = i;
            break;
        }
    }
}

void start_menu_free(StartMenu *menu) {
    if (menu->font_title) { TTF_CloseFont(menu->font_title); menu->font_title = NULL; }
    if (menu->font_body)  { TTF_CloseFont(menu->font_body);  menu->font_body = NULL; }
    if (menu->font_small) { TTF_CloseFont(menu->font_small); menu->font_small = NULL; }
    if (menu->logo_img) { free(menu->logo_img); menu->logo_img = NULL; }
    if (menu->captive_img) { free(menu->captive_img); menu->captive_img = NULL; }
    if (menu->liberation_img) { free(menu->liberation_img); menu->liberation_img = NULL; }
    menu->ttf_ready = false;
}

MenuResult start_menu_handle_click(StartMenu *menu, float x, float y) {
    if (!menu || menu->in_settings) return MENU_RESULT_NONE;
    int W = MENU_WIDTH, H = MENU_HEIGHT;
    int card_w = 390, card_h = 340, card_y = 80, gap = 30;
    int total_w = card_w * 2 + gap;
    int cx0 = (W - total_w) / 2, cx1 = cx0 + card_w + gap;
    int bottom_y = card_y + card_h + 50;

    int item = -1;
    if (y >= card_y && y < card_y + card_h) {
        if (x >= cx0 && x < cx0 + card_w) item = 0;
        else if (x >= cx1 && x < cx1 + card_w) item = 1;
    } else if (y >= bottom_y && y < bottom_y + 30) {
        if (x >= cx0 && x < cx0 + card_w) item = 2;
        else if (x >= cx1 && x < cx1 + card_w) item = 3;
    }
    if (item < 0) return MENU_RESULT_NONE;
    menu->selected_item = item;
    switch (item) {
        case 0: return MENU_RESULT_START_CAPTIVE;
        case 1: return MENU_RESULT_START_LIBERATION;
        case 2: menu->in_settings = true; menu->settings_cursor = 0; break;
        case 3: return MENU_RESULT_QUIT;
    }
    return MENU_RESULT_NONE;
}

MenuResult start_menu_handle_event(StartMenu *menu, const SDL_Event *event) {
    if (menu->data_path_editing) {
        if (event->type == SDL_EVENT_TEXT_INPUT) {
            int len = (int)strlen(menu->data_path);
            const char *text = event->text.text;
            int tlen = (int)strlen(text);
            if (len + tlen < 510) {
                memmove(&menu->data_path[menu->data_path_cursor + tlen],
                        &menu->data_path[menu->data_path_cursor],
                        len - menu->data_path_cursor + 1);
                memcpy(&menu->data_path[menu->data_path_cursor], text, tlen);
                menu->data_path_cursor += tlen;
            }
            return MENU_RESULT_NONE;
        }
        if (event->type != SDL_EVENT_KEY_DOWN) return MENU_RESULT_NONE;
        switch (event->key.key) {
            case SDLK_BACKSPACE:
                if (menu->data_path_cursor > 0) {
                    int len = (int)strlen(menu->data_path);
                    memmove(&menu->data_path[menu->data_path_cursor - 1],
                            &menu->data_path[menu->data_path_cursor],
                            len - menu->data_path_cursor + 1);
                    menu->data_path_cursor--;
                }
                break;
            case SDLK_DELETE: {
                int len = (int)strlen(menu->data_path);
                if (menu->data_path_cursor < len)
                    memmove(&menu->data_path[menu->data_path_cursor],
                            &menu->data_path[menu->data_path_cursor + 1],
                            len - menu->data_path_cursor);
                break;
            }
            case SDLK_LEFT: if (menu->data_path_cursor > 0) menu->data_path_cursor--; break;
            case SDLK_RIGHT: if (menu->data_path_cursor < (int)strlen(menu->data_path)) menu->data_path_cursor++; break;
            case SDLK_HOME: menu->data_path_cursor = 0; break;
            case SDLK_END: menu->data_path_cursor = (int)strlen(menu->data_path); break;
            case SDLK_RETURN: case SDLK_KP_ENTER: case SDLK_ESCAPE:
                menu->data_path_editing = false; SDL_StopTextInput(NULL); break;
            default: break;
        }
        return MENU_RESULT_NONE;
    }

    if (event->type != SDL_EVENT_KEY_DOWN) return MENU_RESULT_NONE;

    if (menu->in_settings) {
        #define SETTINGS_COUNT 16
        switch (event->key.key) {
            case SDLK_UP: if (menu->settings_cursor > 0) menu->settings_cursor--; break;
            case SDLK_DOWN: if (menu->settings_cursor < SETTINGS_COUNT - 1) menu->settings_cursor++; break;
            case SDLK_RETURN: case SDLK_KP_ENTER: case SDLK_LEFT: case SDLK_RIGHT:
                switch (menu->settings_cursor) {
                    case 0: menu->enhanced_mode = false; break;
                    case 1: menu->scanlines = !menu->scanlines; break;
                    case 2: menu->crt_curvature = !menu->crt_curvature; break;
                    case 3: menu->bilinear = !menu->bilinear; break;
                    case 4: menu->integer_scaling = !menu->integer_scaling; break;
                    case 5:
                        if (event->key.key == SDLK_RIGHT && menu->scale_factor < 5) menu->scale_factor++;
                        else if (event->key.key == SDLK_LEFT && menu->scale_factor > 1) menu->scale_factor--;
                        break;
                    case 6: menu->fullscreen = !menu->fullscreen; break;
                    case 7: menu->vsync = !menu->vsync; break;
                    case 8: {
                        int fps_vals[] = {0, 30, 60, 120};
                        int cur = 2;
                        for (int i = 0; i < 4; i++) if (fps_vals[i] == menu->fps_limit) cur = i;
                        if (event->key.key == SDLK_RIGHT) cur = (cur + 1) % 4;
                        else if (event->key.key == SDLK_LEFT) cur = (cur + 3) % 4;
                        menu->fps_limit = fps_vals[cur];
                        break;
                    }
                    case 9: if (event->key.key==SDLK_RIGHT&&menu->brightness<100) menu->brightness+=5;
                            else if (event->key.key==SDLK_LEFT&&menu->brightness>0) menu->brightness-=5; break;
                    case 10: if (event->key.key==SDLK_RIGHT&&menu->contrast<100) menu->contrast+=5;
                             else if (event->key.key==SDLK_LEFT&&menu->contrast>0) menu->contrast-=5; break;
                    case 11: menu->music_enabled = !menu->music_enabled; break;
                    case 12: menu->sfx_enabled = !menu->sfx_enabled; break;
                    case 13:
                        if (event->key.key == SDLK_RETURN || event->key.key == SDLK_KP_ENTER) {
                            menu->data_path_editing = true;
                            menu->data_path_cursor = (int)strlen(menu->data_path);
                            SDL_StartTextInput(NULL);
                        }
                        break;
                    case 14:
                        if (event->key.key == SDLK_RIGHT) menu->lang_index = (menu->lang_index + 1) % LANG_COUNT;
                        else if (event->key.key == SDLK_LEFT) menu->lang_index = (menu->lang_index + LANG_COUNT - 1) % LANG_COUNT;
                        i18n_init(lang_codes[menu->lang_index]);
                        break;
                    case 15: menu->in_settings = false; break;
                }
                break;
            case SDLK_ESCAPE: menu->in_settings = false; break;
        }
        int visible = 10;
        if (menu->settings_cursor < menu->settings_scroll) menu->settings_scroll = menu->settings_cursor;
        if (menu->settings_cursor >= menu->settings_scroll + visible) menu->settings_scroll = menu->settings_cursor - visible + 1;
        return MENU_RESULT_NONE;
    }

    switch (event->key.key) {
        case SDLK_UP: if (menu->selected_item >= 2) menu->selected_item -= 2; break;
        case SDLK_DOWN: if (menu->selected_item < 2) menu->selected_item += 2; break;
        case SDLK_LEFT: if (menu->selected_item == 1) menu->selected_item = 0; else if (menu->selected_item == 3) menu->selected_item = 2; break;
        case SDLK_RIGHT: if (menu->selected_item == 0) menu->selected_item = 1; else if (menu->selected_item == 2) menu->selected_item = 3; break;
        case SDLK_RETURN: case SDLK_KP_ENTER:
            switch (menu->selected_item) {
                case 0: return MENU_RESULT_START_CAPTIVE;
                case 1: return MENU_RESULT_START_LIBERATION;
                case 2: menu->in_settings = true; menu->settings_cursor = 0; break;
                case 3: return MENU_RESULT_QUIT;
            }
            break;
        case SDLK_ESCAPE: return MENU_RESULT_QUIT;
    }
    return MENU_RESULT_NONE;
}

static void render_settings(StartMenu *menu, uint32_t *pixels, int width, int height) {
    TTF_Font *title = menu->font_title;
    TTF_Font *body = menu->font_body;
    TTF_Font *small = menu->font_small;

    ttf_text_centered(pixels, width, height, 15, _("OPENCAPTIVE"), title, 0xFFFF8800);
    ttf_text_centered(pixels, width, height, 55, _("SETTINGS"), body, 0xFF888888);

    const char *labels[] = {
        _("RENDERER:"), _("SCANLINES:"), _("CRT CURVE:"), _("BILINEAR:"),
        _("INT SCALE:"), _("SCALE:"), _("FULLSCREEN:"), _("VSYNC:"),
        _("FPS LIMIT:"), _("BRIGHTNESS:"), _("CONTRAST:"), _("MUSIC:"),
        _("SFX:"), _("DATA PATH:"), _("LANGUAGE:"), _("BACK"),
    };
    char values[SETTINGS_COUNT][32];
    snprintf(values[0], 32, "PENDING");
    snprintf(values[1], 32, "%s", menu->scanlines ? _("ON") : _("OFF"));
    snprintf(values[2], 32, "%s", menu->crt_curvature ? _("ON") : _("OFF"));
    snprintf(values[3], 32, "%s", menu->bilinear ? _("ON") : _("OFF"));
    snprintf(values[4], 32, "%s", menu->integer_scaling ? _("ON") : _("OFF"));
    snprintf(values[5], 32, "%dX", menu->scale_factor);
    snprintf(values[6], 32, "%s", menu->fullscreen ? _("ON") : _("OFF"));
    snprintf(values[7], 32, "%s", menu->vsync ? _("ON") : _("OFF"));
    snprintf(values[8], 32, "%s", menu->fps_limit == 0 ? _("UNLIMITED") :
             (menu->fps_limit == 30 ? "30" : (menu->fps_limit == 60 ? "60" : "120")));
    snprintf(values[9], 32, "%d%%", menu->brightness);
    snprintf(values[10], 32, "%d%%", menu->contrast);
    snprintf(values[11], 32, "%s", menu->music_enabled ? _("ON") : _("OFF"));
    snprintf(values[12], 32, "%s", menu->sfx_enabled ? _("ON") : _("OFF"));
    values[13][0] = '\0';
    snprintf(values[14], 32, "%s", lang_labels[menu->lang_index]);
    values[15][0] = '\0';

    int menu_y = 90;
    int item_h = 32;
    int visible = 10;

    for (int vi = 0; vi < visible && vi + menu->settings_scroll < SETTINGS_COUNT; vi++) {
        int i = vi + menu->settings_scroll;
        int y = menu_y + vi * item_h;
        bool sel = (i == menu->settings_cursor);

        if (sel) {
            draw_rect(pixels, width, height, 80, y - 2, width - 160, item_h - 4, 0xFF333366);
            ttf_text(pixels, width, height, 85, y, "\xe2\x96\xb6", body,
                     ((menu->anim_tick / 8) % 2) ? 0xFFFFFF00 : 0xFFFF8800);
        }
        uint32_t color = sel ? 0xFFFFFFFF : 0xFFAAAAAA;
        ttf_text(pixels, width, height, 110, y, labels[i], body, color);
        if (values[i][0]) {
            uint32_t vc = sel ? 0xFFFFFF00 : 0xFF88AA88;
            ttf_text(pixels, width, height, 480, y, values[i], body, vc);
        }
    }

    if (menu->settings_scroll > 0)
        ttf_text_centered(pixels, width, height, menu_y - 20, "...", small, 0xFF555555);
    if (menu->settings_scroll + visible < SETTINGS_COUNT)
        ttf_text_centered(pixels, width, height, menu_y + visible * item_h, "...", small, 0xFF555555);

    if (menu->settings_cursor == 13) {
        int py = menu_y + visible * item_h + 15;
        draw_rect(pixels, width, height, 80, py, width - 160, 24, 0xFF222244);
        const char *dp = menu->data_path;
        ttf_text(pixels, width, height, 90, py + 2, dp,
                 small, menu->data_path_editing ? 0xFF44FF44 : 0xFFAAAACC);
    }

    ttf_text_centered(pixels, width, height, height - 30,
                       menu->data_path_editing
                       ? _("TYPE PATH  ENTER: CONFIRM  ESC: CANCEL")
                       : _("UP-DOWN: SELECT  LEFT-RIGHT: ADJUST  ESC: BACK"),
                       small, 0xFF555555);
    draw_border(pixels, width, height, 10, 10, width - 20, height - 20, 0xFF444488, 2);
}

void start_menu_render(StartMenu *menu, uint32_t *pixels, int width, int height) {
    menu->anim_tick++;
    memset(pixels, 0, (size_t)width * height * sizeof(uint32_t));

    if (menu->in_settings) {
        render_settings(menu, pixels, width, height);
        return;
    }

    TTF_Font *title = menu->font_title;
    TTF_Font *body = menu->font_body;
    TTF_Font *small = menu->font_small;

    int logo_h_used = 0;
    if (menu->logo_img) {
        int max_w = width - 40, max_h = 70;
        int lw = menu->logo_img_w, lh = menu->logo_img_h;
        float scale = (float)max_w / lw;
        if (lh * scale > max_h) scale = (float)max_h / lh;
        int dw = (int)(lw * scale), dh = (int)(lh * scale);
        int dx = (width - dw) / 2, dy = 5;
        blit_scaled(pixels, width, height, dx, dy, dw, dh,
                    menu->logo_img, lw, lh);
        logo_h_used = dy + dh + 5;
    } else {
        ttf_text_centered(pixels, width, height, 10, _("OPENCAPTIVE"), title, 0xFFFF8800);
        logo_h_used = 50;
    }
    ttf_text_centered(pixels, width, height, logo_h_used, _("BY DANIEL NYLANDER"), small, 0xFF666688);

    int card_w = 390, card_h = 340, card_y = logo_h_used + 20, gap = 30;
    int total_w = card_w * 2 + gap;
    int cx0 = (width - total_w) / 2;
    int cx1 = cx0 + card_w + gap;

    if (menu->captive_img) {
        blit_scaled(pixels, width, height, cx0, card_y, card_w, card_h,
                    menu->captive_img, menu->captive_img_w, menu->captive_img_h);
    } else {
        draw_captive_card(pixels, width, height, cx0, card_y, card_w, card_h,
                          menu->selected_item == 0, menu->anim_tick);
    }
    if (menu->selected_item == 0) {
        uint32_t sc = ((menu->anim_tick / 8) % 2) ? 0xFFFF8800 : 0xFFFFAA44;
        draw_border(pixels, width, height, cx0, card_y, card_w, card_h, sc, 3);
    } else {
        draw_border(pixels, width, height, cx0, card_y, card_w, card_h, 0xFF444466, 2);
    }

    if (menu->liberation_img) {
        blit_scaled(pixels, width, height, cx1, card_y, card_w, card_h,
                    menu->liberation_img, menu->liberation_img_w, menu->liberation_img_h);
    } else {
        draw_liberation_card(pixels, width, height, cx1, card_y, card_w, card_h,
                             menu->selected_item == 1, menu->anim_tick);
    }
    if (menu->selected_item == 1) {
        uint32_t sc = ((menu->anim_tick / 8) % 2) ? 0xFF4488FF : 0xFF66AAFF;
        draw_border(pixels, width, height, cx1, card_y, card_w, card_h, sc, 3);
    } else {
        draw_border(pixels, width, height, cx1, card_y, card_w, card_h, 0xFF444466, 2);
    }

    int label_y = card_y + card_h + 8;
    int ltw = 0;
    if (body) TTF_GetStringSize(body, "CAPTIVE", 0, &ltw, NULL);
    ttf_text(pixels, width, height, cx0 + (card_w - ltw) / 2, label_y, "CAPTIVE", body, 0xFFCCCCCC);
    if (body) TTF_GetStringSize(body, "LIBERATION", 0, &ltw, NULL);
    ttf_text(pixels, width, height, cx1 + (card_w - ltw) / 2, label_y, "LIBERATION", body, 0xFFCCCCCC);

    int year_y = label_y + 24;
    int ctw = 0;
    if (small) TTF_GetStringSize(small, "1990", 0, &ctw, NULL);
    ttf_text(pixels, width, height, cx0 + (card_w - ctw) / 2, year_y, "1990", small, 0xFF888888);
    if (small) TTF_GetStringSize(small, "1993", 0, &ctw, NULL);
    ttf_text(pixels, width, height, cx1 + (card_w - ctw) / 2, year_y, "1993", small, 0xFF888888);

    int bottom_y = card_y + card_h + 60;
    const char *btns[] = { _("SETTINGS"), _("QUIT") };
    int bidx[] = { 2, 3 };
    for (int i = 0; i < 2; i++) {
        int bx = cx0 + i * (card_w + gap);
        bool sel = (menu->selected_item == bidx[i]);
        if (sel)
            draw_rect(pixels, width, height, bx, bottom_y - 2, card_w, 28, 0xFF333366);
        uint32_t col = sel ? 0xFFFFFFFF : 0xFFAAAAAA;
        int btw = 0;
        if (body) TTF_GetStringSize(body, btns[i], 0, &btw, NULL);
        ttf_text(pixels, width, height, bx + (card_w - btw) / 2, bottom_y, btns[i], body, col);
    }

    ttf_text_centered(pixels, width, height, height - 25,
                       _("UP-DOWN: SELECT  ENTER: START  ESC: QUIT"),
                       small, 0xFF444444);
    draw_border(pixels, width, height, 10, 10, width - 20, height - 20, 0xFF444488, 2);

    char ver[32];
    snprintf(ver, sizeof(ver), "v%d.%d.%d",
             OPENCAPTIVE_VERSION_MAJOR, OPENCAPTIVE_VERSION_MINOR, OPENCAPTIVE_VERSION_PATCH);
    ttf_text(pixels, width, height, 20, height - 25, ver, small, 0xFF333333);
}
