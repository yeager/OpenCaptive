#include "start_menu.h"
#include "opencaptive.h"
#include "data_vfs.h"
#include "sha256.h"
#include "liberation_data.h"
#include "i18n.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <SDL3_ttf/SDL_ttf.h>

extern unsigned char *load_png_file(const char *path, int *w, int *h);

static const char *captive_required_hashes[] = {
    "71bcf404103f1ac2920800a8bc166939bb49a1204cf51bebce8aca7dd5faafde",
    "1ec1f90adbcfcb3b99b64a56cf1c669b409b7d3a76bc09cedb056f503bfb1959",
    "47ad15b4a593c37880d0306b6a0f51b7a9f20615cf6a188f23716d5b48315524",
    "43833e4a8df622f84d53698a76c6d18f910c1cca79c6b89cbfacc563f695356c",
    "8b7301fc6c302fd673a81d23e7a99d715aa02d5b404c1e1edea19ceccccc9681",
    "519d3ef4494f0e868479a90c8a47249b840598e382c7ba3272f417ce3daf5936",
    "7edb8ee856a91e835ea86dda00af49fda3dae730d694bd7234b7fa96d711e296",
    "978d18857d5ffcf6fb7b91fb22c02b85079db0171caeac3d290a69b276cf098f",
    "dec7143f063c98459ab2f267ed135204cdee1b521eda9810b219e8c10e05c7e8",
    "ce00ba2bc78f160b934486fe101a90264163356e02e7acbea2a41cf5d125b017",
    "21db7daf64cff3b0cae19c3e7eb2057762df9110055e7253175024ecb146fb6b",
    "dfca77f0e219962242226f11f9697f580f92e8ad24786296a5b2571b20c2b707",
};
#define CAPTIVE_HASH_COUNT (sizeof(captive_required_hashes)/sizeof(captive_required_hashes[0]))

static void stop_data_scanner(StartMenu *menu);

#define SETTINGS_COUNT 24

static uint32_t *load_card_image(const char *filename, int *w, int *h) {
    char path[1024];
    const char *base = SDL_GetBasePath();
    if (base) {
        snprintf(path, sizeof(path), "%s%s", base, filename);
        unsigned char *rgba = load_png_file(path, w, h);
        if (rgba) return (uint32_t *)rgba;

        /* A macOS app bundle keeps launch resources in Contents/Resources,
         * while SDL_GetBasePath() points at Contents/MacOS. */
        snprintf(path, sizeof(path), "%s../Resources/%s", base, filename);
        rgba = load_png_file(path, w, h);
        if (rgba) return (uint32_t *)rgba;

        /* Linux packages and AppImages keep launcher resources in the
         * standard share directory beside the executable's bin directory. */
        snprintf(path, sizeof(path), "%s../share/opencaptive/%s", base, filename);
        rgba = load_png_file(path, w, h);
        if (rgba) return (uint32_t *)rgba;
    }
    snprintf(path, sizeof(path), "./%s", filename);
    unsigned char *rgba = load_png_file(path, w, h);
    if (rgba) return (uint32_t *)rgba;

    /* Native Unix packages keep launcher resources separate from /usr/bin. */
    snprintf(path, sizeof(path), "/usr/share/opencaptive/%s", filename);
    rgba = load_png_file(path, w, h);
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
        "../share/opencaptive/data/DejaVuSansMono-Bold.ttf",
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
    {
        TTF_Font *f = TTF_OpenFont("/usr/share/opencaptive/data/DejaVuSansMono-Bold.ttf", pt);
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
                uint32_t r=(uint32_t)((0x06*(256-t)+0x18*t)>>8);
                uint32_t g=(uint32_t)((0x06*(256-t)+0x10*t)>>8);
                uint32_t b=(uint32_t)((0x20*(256-t)+0x38*t)>>8);
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

static int start_menu_logo_height(const StartMenu *menu, int width) {
    if (!menu || !menu->logo_img || menu->logo_img_w <= 0 ||
        menu->logo_img_h <= 0) return 45;
    int max_w = width - 40;
    int max_h = 60;
    float scale = (float)max_w / menu->logo_img_w;
    if (menu->logo_img_h * scale > max_h)
        scale = (float)max_h / menu->logo_img_h;
    int dh = (int)(menu->logo_img_h * scale);
    return 3 + dh + 3;
}

static void start_menu_reset(StartMenu *menu, bool preserve_resources) {
    char saved_path[512] = {0};
    int saved_cursor = 0;
    uint32_t *logo_img = NULL, *cap_img = NULL, *lib_img = NULL;
    int logo_w = 0, logo_h = 0, cap_w = 0, cap_h = 0, lib_w = 0, lib_h = 0;
    TTF_Font *ft = NULL, *fb = NULL, *fs = NULL;
    bool ttf_ok = false;

    /* A fresh init must not inspect an uninitialised StartMenu.  Re-entry
     * from the running game explicitly opts into preserving loaded resources
     * and the user's data path. */
    if (preserve_resources) {
        memcpy(saved_path, menu->data_path, sizeof(saved_path));
        saved_cursor = menu->data_path_cursor;
        logo_img = menu->logo_img;
        logo_w = menu->logo_img_w; logo_h = menu->logo_img_h;
        cap_img = menu->captive_img;
        cap_w = menu->captive_img_w; cap_h = menu->captive_img_h;
        lib_img = menu->liberation_img;
        lib_w = menu->liberation_img_w; lib_h = menu->liberation_img_h;
        ft = menu->font_title; fb = menu->font_body; fs = menu->font_small;
        ttf_ok = menu->ttf_ready;
    }

    memset(menu, 0, sizeof(*menu));
    memcpy(menu->data_path, saved_path, sizeof(menu->data_path));
    menu->data_path_cursor = saved_cursor ? saved_cursor : (int)strlen(menu->data_path);
    menu->num_items = 6;
    menu->platform = CAPTIVE_PLATFORM_DOS;
    menu->music_enabled = true;
    menu->sfx_enabled = true;
    menu->scale_factor = 3;
    menu->vsync = true;
    menu->integer_scaling = true;
    menu->brightness = 50;
    menu->contrast = 50;
    menu->gamma = 50;
    menu->fps_limit = 60;
    menu->master_volume = 80;
    menu->audio_sample_rate = 1;
    menu->game_speed = 1;
    menu->mouse_sensitivity = 5;
    menu->window_size = 1;

    menu->logo_img = logo_img; menu->logo_img_w = logo_w; menu->logo_img_h = logo_h;
    menu->captive_img = cap_img; menu->captive_img_w = cap_w; menu->captive_img_h = cap_h;
    menu->liberation_img = lib_img; menu->liberation_img_w = lib_w; menu->liberation_img_h = lib_h;
    menu->font_title = ft; menu->font_body = fb; menu->font_small = fs;
    menu->ttf_ready = ttf_ok;

    if (!menu->logo_img)
        menu->logo_img = load_card_image("captivelogo.png",
            &menu->logo_img_w, &menu->logo_img_h);
    if (!menu->captive_img)
        menu->captive_img = load_card_image("assets/menu/captive-cover-v1.png",
            &menu->captive_img_w, &menu->captive_img_h);
    if (!menu->liberation_img)
        menu->liberation_img = load_card_image("assets/menu/liberation-cover-v1.png",
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

void start_menu_init(StartMenu *menu) {
    if (!menu) return;
    start_menu_reset(menu, false);
}

void start_menu_reinit(StartMenu *menu) {
    if (!menu) return;
    start_menu_reset(menu, true);
}

void start_menu_check_data(StartMenu *menu, const char *data_path) {
    if (!menu) return;
    stop_data_scanner(menu);
    menu->in_scanner = false;
    menu->scanner_background = false;
    menu->captive_data_ok = false;
    menu->liberation_data_ok = false;
    menu->captive_source_mask = 0U;
    menu->liberation_source_mask = 0U;
    menu->captive_source_choice = CAPTIVE_PLATFORM_DOS;
    menu->liberation_source_choice = LIBERATION_SOURCE_NONE;
    if (!data_path || !data_path[0]) return;
    DataVFS vfs;
    if (!vfs_init(&vfs, data_path)) return;
    bool dos_valid = true;
    for (size_t i = 0; i < CAPTIVE_HASH_COUNT; i++) {
        size_t sz = 0;
        uint8_t *d = vfs_find_sha256(&vfs, captive_required_hashes[i], &sz);
        if (d) free(d); else { dos_valid = false; break; }
    }
    /* The Amiga verifier is intentionally not part of this playable-source
       mask yet: the Captive runtime currently loads the DOS PL5 atlas and
       has no Amiga graphics adapter. Advertising a verified ADF as a launch
       source made the version popup select Amiga and then silently load DOS
       assets instead. The verifier remains available to --verify-data. */
    menu->captive_source_mask = dos_valid ? (1U << CAPTIVE_PLATFORM_DOS) : 0U;
    menu->captive_data_ok = menu->captive_source_mask != 0U;
    menu->captive_source_choice = CAPTIVE_PLATFORM_DOS;
    menu->liberation_source_mask = liberation_data_available_sources(&vfs);
    menu->liberation_data_ok = menu->liberation_source_mask != 0U;
    vfs_free(&vfs);
}

static MenuResult request_game_start(StartMenu *menu, int game) {
    /* The data path may have been edited since the last menu refresh.  Refresh
       the cheap metadata-backed scan before deciding whether a version choice
       is needed. */
    start_menu_check_data(menu, menu->data_path);
    unsigned source_mask = game == GAME_CAPTIVE ? menu->captive_source_mask
                                                : menu->liberation_source_mask;
    if ((source_mask & (source_mask - 1U)) != 0U) {
        menu->show_version_popup = true;
        menu->version_popup_game = game;
        menu->version_popup_selection = 0;
        return MENU_RESULT_NONE;
    }
    return game == GAME_LIBERATION ? MENU_RESULT_START_LIBERATION
                                   : MENU_RESULT_START_CAPTIVE;
}

static unsigned version_popup_source_mask(const StartMenu *menu) {
    if (!menu) return 0U;
    return menu->version_popup_game == GAME_CAPTIVE
        ? menu->captive_source_mask : menu->liberation_source_mask;
}

static int version_popup_source_candidate(const StartMenu *menu, int index) {
    static const int captive_sources[] = {
        CAPTIVE_PLATFORM_DOS, CAPTIVE_PLATFORM_AMIGA
    };
    static const int liberation_sources[] = {
        LIBERATION_SOURCE_CD32, LIBERATION_SOURCE_AMIGA_ADF
    };
    const int *sources = menu && menu->version_popup_game == GAME_CAPTIVE
        ? captive_sources : liberation_sources;
    return sources[index];
}

static int version_popup_source_at(const StartMenu *menu, int selection) {
    unsigned mask = version_popup_source_mask(menu);
    int found = 0;
    if (selection < 0) return -1;
    for (int index = 0; index < 2; ++index) {
        int source = version_popup_source_candidate(menu, index);
        if ((mask & (1U << source)) == 0U) continue;
        if (found++ == selection) return source;
    }
    return -1;
}

static void version_popup_clamp_selection(StartMenu *menu) {
    if (!menu) return;
    int count = 0;
    unsigned mask = version_popup_source_mask(menu);
    for (int index = 0; index < 2; ++index) {
        int source = version_popup_source_candidate(menu, index);
        if (mask & (1U << source)) count++;
    }
    if (count <= 0) menu->version_popup_selection = 0;
    else if (menu->version_popup_selection >= count) menu->version_popup_selection = count - 1;
    else if (menu->version_popup_selection < 0) menu->version_popup_selection = 0;
}

static MenuResult version_popup_start(StartMenu *menu) {
    if (!menu) return MENU_RESULT_NONE;
    int source = version_popup_source_at(menu, menu->version_popup_selection);
    if (source < 0) {
        menu->show_version_popup = false;
        return MENU_RESULT_NONE;
    }
    menu->show_version_popup = false;
    if (menu->version_popup_game == GAME_CAPTIVE) {
        menu->captive_source_choice = (CaptivePlatform)source;
        menu->platform = menu->captive_source_choice;
        return MENU_RESULT_START_CAPTIVE;
    }
    menu->liberation_source_choice = (LiberationSource)source;
    return MENU_RESULT_START_LIBERATION;
}

void start_menu_check_saves(StartMenu *menu) {
    if (!menu) return;
    menu->captive_save_exists = false;
    menu->captive_save_slot = -1;
    time_t newest_time = 0;
    for (int slot = 0; slot < 10; ++slot) {
        char path[64];
        struct stat st;
        snprintf(path, sizeof(path), "opencaptive_slot%d.sav", slot);
        if (stat(path, &st) == 0 && S_ISREG(st.st_mode)) {
            if (!menu->captive_save_exists || st.st_mtime > newest_time) {
                menu->captive_save_slot = slot;
                newest_time = st.st_mtime;
            }
            menu->captive_save_exists = true;
        }
    }
    FILE *f = fopen("opencaptive.sav", "rb");
    if (f) {
        menu->captive_save_exists = true;
        /* Keep numbered saves preferred when both formats are present. */
        fclose(f);
    }
    f = fopen("liberation.sav", "rb");
    menu->liberation_save_exists = (f != NULL);
    if (f) fclose(f);
}

static void stop_data_scanner(StartMenu *menu) {
    if (!menu) return;
    if (menu->scanner_vfs_ready) {
        vfs_free(&menu->scanner_vfs);
        menu->scanner_vfs_ready = false;
    }
}

void start_menu_start_scan(StartMenu *menu, const char *data_path,
                           bool show_progress) {
    if (!menu) return;
    stop_data_scanner(menu);
    menu->in_scanner = show_progress;
    menu->scanner_background = !show_progress;
    menu->scanner_done = false;
    if (!data_path || !data_path[0]) {
        menu->scanner_done = true;
        return;
    }
    if (!vfs_init(&menu->scanner_vfs, data_path)) {
        menu->scanner_done = true;
        return;
    }
    menu->scanner_vfs_ready = true;
    menu->scanner_captive_index = 0;
    menu->scanner_zip_count = menu->scanner_vfs.num_zips;
    menu->scanner_captive_total = (int)CAPTIVE_HASH_COUNT;
    menu->scanner_captive_found = 0;
    menu->scanner_liberation_total = (int)LIBERATION_RESOURCE_COUNT;
    menu->scanner_liberation_found = 0;
    menu->scanner_total = (int)CAPTIVE_HASH_COUNT +
                          (int)LIBERATION_RESOURCE_COUNT;
    menu->scanner_progress = 0;
    menu->scanner_phase = 0;
    menu->scanner_done = false;
}

void start_menu_update(StartMenu *menu) {
    if (!menu || (!menu->in_scanner && !menu->scanner_background) ||
        menu->scanner_done) return;
    if (!menu->scanner_vfs_ready) {
        menu->scanner_done = true;
        return;
    }
    if (menu->scanner_phase == 0) {
        if (menu->scanner_captive_index < CAPTIVE_HASH_COUNT) {
            size_t sz = 0;
            uint8_t *d = vfs_find_sha256(&menu->scanner_vfs,
                                         captive_required_hashes[menu->scanner_captive_index],
                                         &sz);
            if (d) {
                free(d);
                menu->scanner_captive_found++;
            }
            menu->scanner_captive_index++;
            menu->scanner_progress = (int)menu->scanner_captive_index;
            return;
        }
        menu->scanner_phase = 1;
    }
    if (menu->scanner_phase == 1) {
        /* Availability verification checks the required source hashes but
         * does not decode the optional RNC/ANIM presentation bundle.  The
         * latter belongs to game startup and could otherwise block the menu
         * for an entire frame despite the incremental scanner. */
        menu->liberation_source_mask =
            liberation_data_available_sources(&menu->scanner_vfs);
        if (menu->liberation_source_mask != 0U) {
            menu->scanner_liberation_found = (int)LIBERATION_RESOURCE_COUNT;
        }
        menu->scanner_progress = menu->scanner_total;
        menu->scanner_phase = 2;
        menu->scanner_done = true;
        menu->captive_data_ok = menu->scanner_captive_found ==
                                menu->scanner_captive_total;
        menu->captive_source_mask = menu->captive_data_ok
            ? (1U << CAPTIVE_PLATFORM_DOS) : 0U;
        menu->captive_source_choice = CAPTIVE_PLATFORM_DOS;
        menu->liberation_data_ok = menu->liberation_source_mask != 0U;
        menu->liberation_source_choice = menu->liberation_data_ok
            ? ((menu->liberation_source_mask &
                (1U << LIBERATION_SOURCE_CD32))
                   ? LIBERATION_SOURCE_CD32 : LIBERATION_SOURCE_AMIGA_ADF)
            : LIBERATION_SOURCE_NONE;
        stop_data_scanner(menu);
    }
}

void start_menu_free(StartMenu *menu) {
    if (!menu) return;
    stop_data_scanner(menu);
    if (menu->font_title) { TTF_CloseFont(menu->font_title); menu->font_title = NULL; }
    if (menu->font_body)  { TTF_CloseFont(menu->font_body);  menu->font_body = NULL; }
    if (menu->font_small) { TTF_CloseFont(menu->font_small); menu->font_small = NULL; }
    if (menu->logo_img) { free(menu->logo_img); menu->logo_img = NULL; }
    if (menu->captive_img) { free(menu->captive_img); menu->captive_img = NULL; }
    if (menu->liberation_img) { free(menu->liberation_img); menu->liberation_img = NULL; }
    menu->ttf_ready = false;
}

static int hit_test_main_menu(const StartMenu *menu, float x, float y) {
    int card_w = 390, card_h = 280, gap = 30;
    int total_w = card_w * 2 + gap;
    int cx0 = (MENU_WIDTH - total_w) / 2, cx1 = cx0 + card_w + gap;
    int logo_h_used = start_menu_logo_height(menu, MENU_WIDTH);
    int card_y = logo_h_used + 18;
    int label_y = card_y + card_h + 4;
    int year_y = label_y + 22;
    int continue_y = year_y + 22;
    int bottom_y = continue_y + 26;
    int bottom2_y = bottom_y + 28;

    if (y >= card_y && y < card_y + card_h) {
        if (x >= cx0 && x < cx0 + card_w) return 0;
        if (x >= cx1 && x < cx1 + card_w) return 1;
    } else if (y >= continue_y && y < continue_y + 24) {
        if (x >= cx0 && x < cx0 + card_w && menu->captive_save_exists) return 2;
        if (x >= cx1 && x < cx1 + card_w && menu->liberation_save_exists) return 3;
    } else if (y >= bottom_y && y < bottom_y + 26) {
        if (x >= cx0 && x < cx0 + card_w) return 4;
        if (x >= cx1 && x < cx1 + card_w) return 5;
    } else if (y >= bottom2_y && y < bottom2_y + 26) {
        if (x >= cx0 && x < cx0 + card_w) return 6;
        if (x >= cx1 && x < cx1 + card_w) return 7;
    }
    return -1;
}

MenuResult start_menu_handle_click(StartMenu *menu, float x, float y) {
    if (!menu) return MENU_RESULT_NONE;

    if (menu->show_version_popup) {
        int pw = 700, ph = 300;
        int px = (MENU_WIDTH - pw) / 2, py = (MENU_HEIGHT - ph) / 2;
        if (x >= px + 80 && x < px + pw - 80 && y >= py + 110 && y < py + 205) {
            menu->version_popup_selection = (y >= py + 165) ? 1 : 0;
            return MENU_RESULT_NONE;
        }
        if (x >= px && x < px + pw && y >= py && y < py + ph) {
            if (y >= py + ph - 60) {
                return version_popup_start(menu);
            }
            return MENU_RESULT_NONE;
        }
        menu->show_version_popup = false;
        return MENU_RESULT_NONE;
    }

    if (menu->in_about || menu->in_controls) {
        menu->in_about = false;
        menu->in_controls = false;
        return MENU_RESULT_NONE;
    }
    if (menu->in_scanner) {
        stop_data_scanner(menu);
        menu->in_scanner = false;
        menu->scanner_background = false;
        menu->scanner_done = false;
        return MENU_RESULT_NONE;
    }
    if (menu->in_settings) {
        int menu_y = 90, item_h = 28;
        int col_items = 12;
        int back = SETTINGS_COUNT - 1;
        int half = MENU_WIDTH / 2;
        int back_y = menu_y + col_items * item_h + 10;
        if (y >= back_y - 2 && y < back_y + item_h - 4 &&
            x >= half - 80 && x < half + 80) {
            menu->settings_cursor = back;
            menu->in_settings = false;
            return MENU_RESULT_NONE;
        }
        int col = (x >= half) ? 1 : 0;
        int row = (int)(y - menu_y) / item_h;
        if (row >= 0 && row < col_items) {
            int i = col * col_items + row;
            if (i < back) {
                menu->settings_cursor = i;
                SDL_Event fake = {0};
                fake.type = SDL_EVENT_KEY_DOWN;
                fake.key.key = SDLK_RETURN;
                return start_menu_handle_event(menu, &fake);
            }
        }
        return MENU_RESULT_NONE;
    }

    int item = hit_test_main_menu(menu, x, y);
    if (item < 0) return MENU_RESULT_NONE;
    menu->selected_item = item;
    switch (item) {
        case 0: return request_game_start(menu, GAME_CAPTIVE);
        case 1: return request_game_start(menu, GAME_LIBERATION);
        case 2: return MENU_RESULT_CONTINUE_CAPTIVE;
        case 3: return MENU_RESULT_CONTINUE_LIBERATION;
        case 4: menu->in_settings = true; menu->settings_cursor = 0; break;
        case 5: menu->in_about = true; break;
        case 6: menu->in_controls = true; break;
        case 7: return MENU_RESULT_QUIT;
    }
    return MENU_RESULT_NONE;
}

void start_menu_handle_mouse_motion(StartMenu *menu, float x, float y) {
    if (!menu || menu->in_settings || menu->in_about || menu->in_controls || menu->in_scanner)
        return;
    int item = hit_test_main_menu(menu, x, y);
    if (item >= 0) menu->selected_item = item;
}

MenuResult start_menu_handle_event(StartMenu *menu, const SDL_Event *event) {
    if (!menu || !event) return MENU_RESULT_NONE;
    if (menu->show_setup_popup) {
        if (event->type == SDL_EVENT_KEY_DOWN || event->type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
            menu->show_setup_popup = false;
        }
        return MENU_RESULT_NONE;
    }
    if (menu->show_version_popup) {
        if (event->type == SDL_EVENT_KEY_DOWN) {
            if (event->key.key == SDLK_ESCAPE) {
                menu->show_version_popup = false;
            } else if (event->key.key == SDLK_UP || event->key.key == SDLK_LEFT) {
                if (menu->version_popup_selection > 0) menu->version_popup_selection--;
            } else if (event->key.key == SDLK_DOWN || event->key.key == SDLK_RIGHT) {
                menu->version_popup_selection++;
                version_popup_clamp_selection(menu);
            } else if (event->key.key == SDLK_RETURN || event->key.key == SDLK_KP_ENTER) {
                return version_popup_start(menu);
            }
        }
        return MENU_RESULT_NONE;
    }
    if (menu->data_path_editing) {
        int path_len = (int)strlen(menu->data_path);
        if (menu->data_path_cursor < 0) menu->data_path_cursor = 0;
        if (menu->data_path_cursor > path_len) menu->data_path_cursor = path_len;
        if (event->type == SDL_EVENT_TEXT_INPUT) {
            const char *text = event->text.text;
            size_t tlen = text ? strlen(text) : 0;
            if (tlen <= sizeof(menu->data_path) - 1U - (size_t)path_len) {
                memmove(&menu->data_path[menu->data_path_cursor + tlen],
                        &menu->data_path[menu->data_path_cursor],
                        (size_t)(path_len - menu->data_path_cursor + 1));
                if (tlen > 0)
                    memcpy(&menu->data_path[menu->data_path_cursor], text, tlen);
                menu->data_path_cursor += (int)tlen;
            }
            return MENU_RESULT_NONE;
        }
        if (event->type != SDL_EVENT_KEY_DOWN) return MENU_RESULT_NONE;
        switch (event->key.key) {
            case SDLK_BACKSPACE:
                if (menu->data_path_cursor > 0) {
                    memmove(&menu->data_path[menu->data_path_cursor - 1],
                            &menu->data_path[menu->data_path_cursor],
                            (size_t)(path_len - menu->data_path_cursor + 1));
                    menu->data_path_cursor--;
                }
                break;
            case SDLK_DELETE: {
                if (menu->data_path_cursor < path_len)
                    memmove(&menu->data_path[menu->data_path_cursor],
                            &menu->data_path[menu->data_path_cursor + 1],
                            (size_t)(path_len - menu->data_path_cursor));
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

    if (menu->in_about || menu->in_controls) {
        menu->in_about = false;
        menu->in_controls = false;
        return MENU_RESULT_NONE;
    }

    if (menu->in_scanner) {
        if (event->key.key == SDLK_ESCAPE || event->key.key == SDLK_RETURN) {
            stop_data_scanner(menu);
            menu->in_scanner = false;
            menu->scanner_background = false;
            menu->scanner_done = false;
        }
        return MENU_RESULT_NONE;
    }

    if (menu->in_settings) {
        switch (event->key.key) {
            case SDLK_UP: {
                int c = menu->settings_cursor;
                int col_items = 12;
                int back = SETTINGS_COUNT - 1;
                if (c == back) {
                    menu->settings_cursor = col_items - 1;
                } else if (c < col_items) {
                    if (c > 0) menu->settings_cursor--;
                } else {
                    if (c > col_items) menu->settings_cursor--;
                }
                break;
            }
            case SDLK_DOWN: {
                int c = menu->settings_cursor;
                int col_items = 12;
                int back = SETTINGS_COUNT - 1;
                if (c == back) break;
                if (c < col_items - 1) {
                    menu->settings_cursor++;
                } else if (c == col_items - 1) {
                    menu->settings_cursor = back;
                } else if (c < back - 1) {
                    menu->settings_cursor++;
                } else {
                    menu->settings_cursor = back;
                }
                break;
            }
            case SDLK_TAB: {
                int c = menu->settings_cursor;
                int col_items = 12;
                int back = SETTINGS_COUNT - 1;
                if (c == back) break;
                if (c < col_items) {
                    int target = col_items + c;
                    if (target >= back) target = back - 1;
                    menu->settings_cursor = target;
                } else {
                    menu->settings_cursor = c - col_items;
                }
                break;
            }
            case SDLK_RETURN: case SDLK_KP_ENTER: case SDLK_LEFT: case SDLK_RIGHT: {
                bool right = (event->key.key == SDLK_RIGHT);
                bool left = (event->key.key == SDLK_LEFT);
                switch (menu->settings_cursor) {
                    case 0:
                        if (right) menu->renderer_backend = (menu->renderer_backend + 1) % 3;
                        else if (left) menu->renderer_backend = (menu->renderer_backend + 2) % 3;
                        break;
                    case 1:
                        if (right && menu->window_size < 3) {
                            menu->window_size++;
                            menu->scale_custom = false;
                        } else if (left && menu->window_size > 0) {
                            menu->window_size--;
                            menu->scale_custom = false;
                        }
                        break;
                    case 2:
                        if (right && menu->scale_factor < 5) {
                            menu->scale_factor++;
                            menu->scale_custom = true;
                        } else if (left && menu->scale_factor > 1) {
                            menu->scale_factor--;
                            menu->scale_custom = true;
                        }
                        break;
                    case 3: menu->fullscreen = !menu->fullscreen; break;
                    case 4: menu->integer_scaling = !menu->integer_scaling; break;
                    case 5: menu->vsync = !menu->vsync; break;
                    case 6: {
                        int fps_vals[] = {0, 30, 60, 120};
                        int cur = 2;
                        for (int i = 0; i < 4; i++) if (fps_vals[i] == menu->fps_limit) cur = i;
                        if (right) cur = (cur + 1) % 4;
                        else if (left) cur = (cur + 3) % 4;
                        menu->fps_limit = fps_vals[cur];
                        break;
                    }
                    case 7: menu->bilinear = !menu->bilinear; break;
                    case 8: menu->scanlines = !menu->scanlines; break;
                    case 9: menu->crt_curvature = !menu->crt_curvature; break;
                    case 10: if (right && menu->brightness < 100) menu->brightness += 5;
                             else if (left && menu->brightness > 0) menu->brightness -= 5; break;
                    case 11: if (right && menu->contrast < 100) menu->contrast += 5;
                             else if (left && menu->contrast > 0) menu->contrast -= 5; break;
                    case 12: if (right && menu->gamma < 100) menu->gamma += 5;
                             else if (left && menu->gamma > 0) menu->gamma -= 5; break;
                    case 13: if (right && menu->master_volume < 100) menu->master_volume += 5;
                             else if (left && menu->master_volume > 0) menu->master_volume -= 5; break;
                    case 14: menu->music_enabled = !menu->music_enabled; break;
                    case 15: menu->sfx_enabled = !menu->sfx_enabled; break;
                    case 16: menu->audio_reverb = !menu->audio_reverb; break;
                    case 17:
                        if (right) menu->audio_sample_rate = (menu->audio_sample_rate + 1) % 3;
                        else if (left) menu->audio_sample_rate = (menu->audio_sample_rate + 2) % 3;
                        break;
                    case 18:
                        if (right && menu->game_speed < 2) menu->game_speed++;
                        else if (left && menu->game_speed > 0) menu->game_speed--;
                        break;
                    case 19:
                        if (right && menu->mouse_sensitivity < 10) menu->mouse_sensitivity++;
                        else if (left && menu->mouse_sensitivity > 1) menu->mouse_sensitivity--;
                        break;
                    case 20:
                        if (event->key.key == SDLK_RETURN || event->key.key == SDLK_KP_ENTER) {
                            menu->data_path_editing = true;
                            menu->data_path_cursor = (int)strlen(menu->data_path);
                            SDL_StartTextInput(NULL);
                        }
                        break;
                    case 21:
                        if (right) menu->lang_index = (menu->lang_index + 1) % LANG_COUNT;
                        else if (left) menu->lang_index = (menu->lang_index + LANG_COUNT - 1) % LANG_COUNT;
                        i18n_init(lang_codes[menu->lang_index]);
                        break;
                    case 22: menu->enhanced_mode = !menu->enhanced_mode; break;
                    case 23: menu->in_settings = false; break;
                }
                break;
            }
            case SDLK_ESCAPE: menu->in_settings = false; break;
        }
        menu->settings_scroll = 0;
        return MENU_RESULT_NONE;
    }

    switch (event->key.key) {
        case SDLK_UP: if (menu->selected_item >= 2) menu->selected_item -= 2; break;
        case SDLK_DOWN: if (menu->selected_item < 6) menu->selected_item += 2; break;
        case SDLK_LEFT: if (menu->selected_item % 2 == 1) menu->selected_item--; break;
        case SDLK_RIGHT: if (menu->selected_item % 2 == 0) menu->selected_item++; break;
        case SDLK_RETURN: case SDLK_KP_ENTER:
            switch (menu->selected_item) {
                case 0: return request_game_start(menu, GAME_CAPTIVE);
                case 1: return request_game_start(menu, GAME_LIBERATION);
                case 2: if (menu->captive_save_exists) return MENU_RESULT_CONTINUE_CAPTIVE; break;
                case 3: if (menu->liberation_save_exists) return MENU_RESULT_CONTINUE_LIBERATION; break;
                case 4: menu->in_settings = true; menu->settings_cursor = 0; break;
                case 5: menu->in_about = true; break;
                case 6: menu->in_controls = true; break;
                case 7: return MENU_RESULT_QUIT;
            }
            break;
        case SDLK_F1: menu->in_controls = true; break;
        case SDLK_D:
            start_menu_start_scan(menu, menu->data_path, true);
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

    static const char *renderer_names[] = { "AUTO", "GPU", "SOFTWARE" };
    static const int win_w[] = { 960, 1280, 1600, 1920 };
    static const int win_h[] = { 600, 800, 1000, 1200 };
    static const int sample_rates[] = { 22050, 44100, 48000 };
    static const char *speed_labels[] = { "0.5X", "1X", "2X" };

    const char *labels[] = {
        _("RENDERER:"), _("WINDOW SIZE:"), _("SCALE:"), _("FULLSCREEN:"),
        _("INT SCALE:"), _("VSYNC:"), _("FPS LIMIT:"), _("FILTERING:"),
        _("SCANLINES:"), _("CRT CURVE:"), _("BRIGHTNESS:"), _("CONTRAST:"),
        _("GAMMA:"), _("VOLUME:"), _("MUSIC:"), _("SFX:"),
        _("REVERB:"), _("SAMPLE RATE:"), _("GAME SPEED:"), _("MOUSE SENS:"),
        _("DATA PATH:"), _("LANGUAGE:"), _("ENHANCED:"), _("BACK"),
    };
    char values[SETTINGS_COUNT][32];
    snprintf(values[0], 32, "%s", renderer_names[menu->renderer_backend]);
    snprintf(values[1], 32, "%dx%d", win_w[menu->window_size], win_h[menu->window_size]);
    snprintf(values[2], 32, "%dX", menu->scale_factor);
    snprintf(values[3], 32, "%s", menu->fullscreen ? _("ON") : _("OFF"));
    snprintf(values[4], 32, "%s", menu->integer_scaling ? _("ON") : _("OFF"));
    snprintf(values[5], 32, "%s", menu->vsync ? _("ON") : _("OFF"));
    snprintf(values[6], 32, "%s", menu->fps_limit == 0 ? _("UNLIMITED") :
             (menu->fps_limit == 30 ? "30" : (menu->fps_limit == 60 ? "60" : "120")));
    snprintf(values[7], 32, "%s", menu->bilinear ? _("BILINEAR") : _("NEAREST"));
    snprintf(values[8], 32, "%s", menu->scanlines ? _("ON") : _("OFF"));
    snprintf(values[9], 32, "%s", menu->crt_curvature ? _("ON") : _("OFF"));
    snprintf(values[10], 32, "%d%%", menu->brightness);
    snprintf(values[11], 32, "%d%%", menu->contrast);
    snprintf(values[12], 32, "%d%%", menu->gamma);
    snprintf(values[13], 32, "%d%%", menu->master_volume);
    snprintf(values[14], 32, "%s", menu->music_enabled ? _("ON") : _("OFF"));
    snprintf(values[15], 32, "%s", menu->sfx_enabled ? _("ON") : _("OFF"));
    snprintf(values[16], 32, "%s", menu->audio_reverb ? _("ON") : _("OFF"));
    snprintf(values[17], 32, "%d Hz", sample_rates[menu->audio_sample_rate]);
    snprintf(values[18], 32, "%s", speed_labels[menu->game_speed]);
    snprintf(values[19], 32, "%d", menu->mouse_sensitivity);
    values[20][0] = '\0';
    snprintf(values[21], 32, "%s", lang_labels[menu->lang_index]);
    snprintf(values[22], 32, "%s", menu->enhanced_mode ? _("ON") : _("OFF"));
    values[23][0] = '\0';

    int menu_y = 90;
    int item_h = 28;
    int col_items = 12;
    int col_left_x = 40;
    int col_right_x = width / 2 + 10;
    int val_offset = 200;

    for (int i = 0; i < SETTINGS_COUNT - 1; i++) {
        int col = (i < col_items) ? 0 : 1;
        int row = (i < col_items) ? i : i - col_items;
        int x = col ? col_right_x : col_left_x;
        int y = menu_y + row * item_h;
        bool sel = (i == menu->settings_cursor);

        if (sel) {
            int rx = col ? col_right_x - 25 : col_left_x - 25;
            draw_rect(pixels, width, height, rx, y - 2, width / 2 - 30, item_h - 4, 0xFF333366);
            ttf_text(pixels, width, height, rx + 5, y, "\xe2\x96\xb6", body,
                     ((menu->anim_tick / 8) % 2) ? 0xFFFFFF00 : 0xFFFF8800);
        }
        uint32_t color = sel ? 0xFFFFFFFF : 0xFFAAAAAA;
        ttf_text(pixels, width, height, x, y, labels[i], body, color);
        if (values[i][0]) {
            uint32_t vc = sel ? 0xFFFFFF00 : 0xFF88AA88;
            ttf_text(pixels, width, height, x + val_offset, y, values[i], body, vc);
        }
    }

    {
        int back_i = SETTINGS_COUNT - 1;
        int back_y = menu_y + col_items * item_h + 10;
        bool sel = (back_i == menu->settings_cursor);
        if (sel)
            draw_rect(pixels, width, height, width / 2 - 80, back_y - 2, 160, item_h - 4, 0xFF333366);
        ttf_text_centered(pixels, width, height, back_y, labels[back_i], body,
                          sel ? 0xFFFFFF00 : 0xFFAAAAAA);
    }

    if (menu->settings_cursor == 20) {
        int py = menu_y + col_items * item_h + 45;
        draw_rect(pixels, width, height, 40, py, width - 80, 24, 0xFF222244);
        const char *dp = menu->data_path;
        ttf_text(pixels, width, height, 50, py + 2, dp,
                 small, menu->data_path_editing ? 0xFF44FF44 : 0xFFAAAACC);
    }

    ttf_text_centered(pixels, width, height, height - 30,
                       menu->data_path_editing
                       ? _("TYPE PATH  ENTER: CONFIRM  ESC: CANCEL")
                       : _("UP-DOWN: SELECT  LEFT-RIGHT: ADJUST  ESC: BACK"),
                       small, 0xFF555555);
    draw_border(pixels, width, height, 10, 10, width - 20, height - 20, 0xFF444488, 2);
}

static void render_about(StartMenu *menu, uint32_t *pixels, int width, int height) {
    TTF_Font *title = menu->font_title;
    TTF_Font *body = menu->font_body;
    TTF_Font *small = menu->font_small;
    char ver[48];
    snprintf(ver, sizeof(ver), "v%d.%d.%d",
             OPENCAPTIVE_VERSION_MAJOR, OPENCAPTIVE_VERSION_MINOR, OPENCAPTIVE_VERSION_PATCH);

    ttf_text_centered(pixels, width, height, 20, _("OPENCAPTIVE"), title, 0xFFFF8800);
    ttf_text_centered(pixels, width, height, 60, ver, body, 0xFF888888);

    int y = 100;
    ttf_text(pixels, width, height, 80, y, _("ABOUT"), body, 0xFFCCCCCC); y += 35;
    ttf_text(pixels, width, height, 80, y, _("A reimplementation of:"), small, 0xFFAAAACC); y += 22;
    ttf_text(pixels, width, height, 100, y, "Captive (1990) - Mindscape / Tony Crowther", small, 0xFF88AACC); y += 20;
    ttf_text(pixels, width, height, 100, y, "Liberation: Captive 2 (1993) - Mindscape", small, 0xFF88AACC); y += 30;
    ttf_text(pixels, width, height, 80, y, _("CREDITS"), body, 0xFFCCCCCC); y += 30;
    ttf_text(pixels, width, height, 100, y, "Original game design: Tony Crowther", small, 0xFF88AACC); y += 20;
    ttf_text(pixels, width, height, 100, y, "OpenCaptive: Daniel Nylander", small, 0xFF88AACC); y += 30;
    ttf_text(pixels, width, height, 80, y, _("TECHNOLOGY"), body, 0xFFCCCCCC); y += 30;
    ttf_text(pixels, width, height, 100, y, "Pure C / SDL3 / OPL2 emulation", small, 0xFF88AACC); y += 20;
    ttf_text(pixels, width, height, 100, y, "SHA-256 content-addressed asset loading", small, 0xFF88AACC); y += 20;
    ttf_text(pixels, width, height, 100, y, "CAPPO.EXE disassembly-verified formulas", small, 0xFF88AACC); y += 20;
    ttf_text(pixels, width, height, 100, y, "Amiga CityGen/PlotGen/BuildingGen disassembly", small, 0xFF88AACC); y += 30;
    ttf_text(pixels, width, height, 80, y, "https://github.com/yeager/OpenCaptive", small, 0xFF6688BB);

    ttf_text_centered(pixels, width, height, height - 30, _("PRESS ANY KEY TO RETURN"), small, 0xFF555555);
    draw_border(pixels, width, height, 10, 10, width - 20, height - 20, 0xFF444488, 2);
}

static void render_controls(StartMenu *menu, uint32_t *pixels, int width, int height) {
    TTF_Font *title = menu->font_title;
    TTF_Font *body = menu->font_body;
    TTF_Font *small = menu->font_small;

    ttf_text_centered(pixels, width, height, 20, _("CONTROLS"), title, 0xFFFF8800);

    int y = 70;
    int lx = 80, rx = 420;
    ttf_text(pixels, width, height, lx, y, _("MOVEMENT"), body, 0xFFCCCCCC); y += 30;
    ttf_text(pixels, width, height, lx, y, "W / Up", small, 0xFF88AACC);
    ttf_text(pixels, width, height, rx, y, _("Move forward"), small, 0xFFAAAACC); y += 20;
    ttf_text(pixels, width, height, lx, y, "S / Down", small, 0xFF88AACC);
    ttf_text(pixels, width, height, rx, y, _("Move backward"), small, 0xFFAAAACC); y += 20;
    ttf_text(pixels, width, height, lx, y, "A / Left", small, 0xFF88AACC);
    ttf_text(pixels, width, height, rx, y, _("Turn left"), small, 0xFFAAAACC); y += 20;
    ttf_text(pixels, width, height, lx, y, "D / Right", small, 0xFF88AACC);
    ttf_text(pixels, width, height, rx, y, _("Turn right"), small, 0xFFAAAACC); y += 30;

    ttf_text(pixels, width, height, lx, y, _("COMBAT & ITEMS"), body, 0xFFCCCCCC); y += 30;
    ttf_text(pixels, width, height, lx, y, "Space", small, 0xFF88AACC);
    ttf_text(pixels, width, height, rx, y, _("Fire weapon"), small, 0xFFAAAACC); y += 20;
    ttf_text(pixels, width, height, lx, y, "Enter", small, 0xFF88AACC);
    ttf_text(pixels, width, height, rx, y, _("Use item / Interact"), small, 0xFFAAAACC); y += 20;
    ttf_text(pixels, width, height, lx, y, "Tab", small, 0xFF88AACC);
    ttf_text(pixels, width, height, rx, y, _("Cycle droids"), small, 0xFFAAAACC); y += 20;
    ttf_text(pixels, width, height, lx, y, "I", small, 0xFF88AACC);
    ttf_text(pixels, width, height, rx, y, _("Inventory"), small, 0xFFAAAACC); y += 30;

    ttf_text(pixels, width, height, lx, y, _("SYSTEM"), body, 0xFFCCCCCC); y += 30;
    ttf_text(pixels, width, height, lx, y, "F5 / F9", small, 0xFF88AACC);
    ttf_text(pixels, width, height, rx, y, _("Save / Load game"), small, 0xFFAAAACC); y += 20;
    ttf_text(pixels, width, height, lx, y, "F6", small, 0xFF88AACC);
    ttf_text(pixels, width, height, rx, y, _("Cycle save slots"), small, 0xFFAAAACC); y += 20;
    ttf_text(pixels, width, height, lx, y, "M / Shift+M", small, 0xFF88AACC);
    ttf_text(pixels, width, height, rx, y, _("Minimap / City map"), small, 0xFFAAAACC); y += 20;
    ttf_text(pixels, width, height, lx, y, "H", small, 0xFF88AACC);
    ttf_text(pixels, width, height, rx, y, _("Help screen"), small, 0xFFAAAACC); y += 20;
    ttf_text(pixels, width, height, lx, y, "ESC", small, 0xFF88AACC);
    ttf_text(pixels, width, height, rx, y, _("Pause menu"), small, 0xFFAAAACC);
    y += 20;
    ttf_text(pixels, width, height, lx, y, "F10", small, 0xFF88AACC);
    ttf_text(pixels, width, height, rx, y, _("Runtime options: graphics and cheats"), small, 0xFFAAAACC);

    ttf_text_centered(pixels, width, height, height - 30, _("PRESS ANY KEY TO RETURN"), small, 0xFF555555);
    draw_border(pixels, width, height, 10, 10, width - 20, height - 20, 0xFF444488, 2);
}

static void render_scanner(StartMenu *menu, uint32_t *pixels, int width, int height) {
    TTF_Font *title = menu->font_title;
    TTF_Font *body = menu->font_body;
    TTF_Font *small = menu->font_small;

    ttf_text_centered(pixels, width, height, 20, _("DATA SCANNER"), title, 0xFFFF8800);

    int y = 80;
    char buf[256];
    ttf_text(pixels, width, height, 80, y, _("DATA PATH:"), body, 0xFFCCCCCC); y += 28;
    ttf_text(pixels, width, height, 100, y, menu->data_path, small, 0xFFAAAACC); y += 30;

    if (!menu->scanner_done) {
        const char *phase_label = menu->scanner_phase == 0
            ? _("Scanning Captive hashes...")
            : _("Scanning Liberation data...");
        ttf_text_centered(pixels, width, height, y + 20, phase_label, body, 0xFFFFFF00);

        int bar_x = 80, bar_w = width - 160, bar_h = 20, bar_y = y + 60;
        draw_border(pixels, width, height, bar_x, bar_y, bar_w, bar_h, 0xFF666666, 1);
        if (menu->scanner_total > 0) {
            int fill_w = (menu->scanner_progress * (bar_w - 2)) / menu->scanner_total;
            if (fill_w > 0) {
                for (int py = bar_y + 1; py < bar_y + bar_h - 1; py++)
                    for (int px = bar_x + 1; px < bar_x + 1 + fill_w; px++)
                        if (px >= 0 && px < width && py >= 0 && py < height)
                            pixels[py * width + px] = 0xFF44AAFF;
            }
        }
        char pct[32];
        int percent = menu->scanner_total > 0
            ? (menu->scanner_progress * 100) / menu->scanner_total : 0;
        snprintf(pct, sizeof(pct), "%d%%", percent);
        ttf_text_centered(pixels, width, height, bar_y + bar_h + 6, pct, small, 0xFFCCCCCC);
    } else {
        snprintf(buf, sizeof(buf), _("ZIP archives found: %d"), menu->scanner_zip_count);
        ttf_text(pixels, width, height, 80, y, buf, body, 0xFFCCCCCC); y += 35;

        snprintf(buf, sizeof(buf), _("Captive: %d / %d %s"),
                 menu->scanner_captive_found, menu->scanner_captive_total,
                 menu->scanner_captive_found == menu->scanner_captive_total ? "[OK]" : "[MISSING]");
        uint32_t cap_col = menu->scanner_captive_found == menu->scanner_captive_total
                           ? 0xFF44FF44 : 0xFFFF4444;
        ttf_text(pixels, width, height, 100, y, buf, body, cap_col); y += 30;

        snprintf(buf, sizeof(buf), _("Liberation: %d / %d %s"),
                 menu->scanner_liberation_found, menu->scanner_liberation_total,
                 menu->scanner_liberation_found == menu->scanner_liberation_total ? "[OK]" : "[MISSING]");
        uint32_t lib_col = menu->scanner_liberation_found == menu->scanner_liberation_total
                           ? 0xFF44FF44 : 0xFFFF4444;
        ttf_text(pixels, width, height, 100, y, buf, body, lib_col); y += 40;

        ttf_text(pixels, width, height, 80, y, _("VERIFICATION"), body, 0xFFCCCCCC); y += 28;
        ttf_text(pixels, width, height, 100, y,
                 _("Files identified by SHA-256 content hash, not filename."),
                 small, 0xFF88AACC); y += 20;
    ttf_text(pixels, width, height, 100, y,
             _("Nested ZIPs, ADF disk images, and ISO tracks are scanned."),
             small, 0xFF88AACC); y += 20;
    ttf_text(pixels, width, height, 100, y,
             _("No filenames are trusted; only content determines identity."),
             small, 0xFF88AACC);
    }

    ttf_text_centered(pixels, width, height, height - 30, _("ESC: BACK"), small, 0xFF555555);
    draw_border(pixels, width, height, 10, 10, width - 20, height - 20, 0xFF444488, 2);
}

static void render_setup_popup(StartMenu *menu, uint32_t *pixels, int width, int height) {
    TTF_Font *title = menu->font_title;
    TTF_Font *body = menu->font_body;
    TTF_Font *small = menu->font_small;

    int pw = 700, ph = 340;
    int px = (width - pw) / 2, py = (height - ph) / 2;

    for (int y = py; y < py + ph && y < height; y++)
        for (int x = px; x < px + pw && x < width; x++)
            if (x >= 0 && y >= 0)
                pixels[y * width + x] = 0xFF1A1A2E;
    draw_border(pixels, width, height, px, py, pw, ph, 0xFFFF8800, 2);

    int y = py + 20;
    ttf_text_centered(pixels, width, height, y, _("GAME DATA NOT FOUND"), title, 0xFFFF4444);
    y += 50;
    ttf_text(pixels, width, height, px + 30, y,
             _("OpenCaptive needs original game data files to run."), body, 0xFFCCCCCC);
    y += 30;
    ttf_text(pixels, width, height, px + 30, y,
             _("Place your game data in the following folder:"), body, 0xFFCCCCCC);
    y += 35;
    ttf_text(pixels, width, height, px + 50, y, menu->data_path, small, 0xFF44AAFF);
    y += 30;
    ttf_text(pixels, width, height, px + 30, y,
             _("Supported formats: ZIP archives, ADF, ISO, raw files."), small, 0xFF88AACC);
    y += 22;
    ttf_text(pixels, width, height, px + 30, y,
             _("Files are identified by SHA-256 hash, not filename."), small, 0xFF88AACC);
    y += 35;
    ttf_text(pixels, width, height, px + 30, y,
             _("You can change the data path in Settings (S)."), body, 0xFFFFCC44);
    ttf_text_centered(pixels, width, height, py + ph - 35,
                      _("PRESS ANY KEY TO CONTINUE"), small, 0xFF555555);
}

static void render_version_popup(StartMenu *menu, uint32_t *pixels, int width, int height) {
    TTF_Font *title = menu->font_title;
    TTF_Font *body = menu->font_body;
    TTF_Font *small = menu->font_small;
    int pw = 700, ph = 300;
    int px = (width - pw) / 2, py = (height - ph) / 2;
    for (int y = py; y < py + ph && y < height; ++y)
        for (int x = px; x < px + pw && x < width; ++x)
            if (x >= 0 && y >= 0) pixels[y * width + x] = 0xFF1A1A2E;
    draw_border(pixels, width, height, px, py, pw, ph, 0xFFFF8800, 2);
    ttf_text_centered(pixels, width, height, py + 20,
                      _("SELECT GAME"), title, 0xFFFF8800);
    ttf_text_centered(pixels, width, height, py + 70,
                      _("UP-DOWN: SELECT  ENTER: START  ESC: QUIT"), body, 0xFFCCCCCC);
    /* Platform names are product/version identifiers and are intentionally
       kept unchanged across locales.  Render only verified sources; this
       also keeps the highlighted row aligned with keyboard selection. */
    version_popup_clamp_selection(menu);
    unsigned mask = version_popup_source_mask(menu);
    int row = 0;
    for (int index = 0; index < 2; ++index) {
        int source = version_popup_source_candidate(menu, index);
        if ((mask & (1U << source)) == 0U) continue;
        const char *label;
        if (menu->version_popup_game == GAME_CAPTIVE)
            label = source == CAPTIVE_PLATFORM_DOS ? _("CAPTIVE DOS") : _("CAPTIVE AMIGA");
        else
            label = source == LIBERATION_SOURCE_CD32 ? _("LIBERATION CD32") : _("LIBERATION AMIGA");
        int i = row++;
        uint32_t color = i == menu->version_popup_selection ? 0xFFFFFF44 : 0xFFAAAAAA;
        ttf_text(pixels, width, height, px + 100, py + 125 + i * 42,
                 i == menu->version_popup_selection ? ">" : " ", body, color);
        ttf_text(pixels, width, height, px + 130, py + 125 + i * 42,
                 label, body, color);
    }
    ttf_text_centered(pixels, width, height, py + ph - 30,
                      _("UP-DOWN: SELECT  ENTER: START  ESC: QUIT"), small, 0xFF666666);
}

void start_menu_render(StartMenu *menu, uint32_t *pixels, int width, int height) {
    if (!menu || !pixels || width <= 0 || height <= 0) return;
    uint32_t now = SDL_GetTicks();
    if (!menu->anim_clock_initialized) {
        menu->anim_clock_initialized = true;
        menu->anim_last_ms = now;
    } else {
        uint32_t elapsed = now - menu->anim_last_ms;
        menu->anim_last_ms = now;
        /* Keep a stalled/debugged window from fast-forwarding the menu
         * animation after a long pause.  One tick represents 1/60 second. */
        if (elapsed > 250U) elapsed = 250U;
        menu->anim_accumulator_ms += elapsed;
        uint32_t steps = menu->anim_accumulator_ms / 16U;
        menu->anim_accumulator_ms %= 16U;
        menu->anim_tick += steps;
    }
    memset(pixels, 0, (size_t)width * (size_t)height * sizeof(uint32_t));

    if (menu->show_setup_popup) { render_setup_popup(menu, pixels, width, height); return; }
    if (menu->show_version_popup) { render_version_popup(menu, pixels, width, height); return; }
    if (menu->in_settings) { render_settings(menu, pixels, width, height); return; }
    if (menu->in_about)    { render_about(menu, pixels, width, height);    return; }
    if (menu->in_controls) { render_controls(menu, pixels, width, height); return; }
    if (menu->in_scanner)  { render_scanner(menu, pixels, width, height);  return; }

    TTF_Font *title = menu->font_title;
    TTF_Font *body = menu->font_body;
    TTF_Font *small = menu->font_small;

    int logo_h_used = 0;
    if (menu->logo_img) {
        int max_w = width - 40, max_h = 60;
        int lw = menu->logo_img_w, lh = menu->logo_img_h;
        float scale = (float)max_w / lw;
        if (lh * scale > max_h) scale = (float)max_h / lh;
        int dw = (int)(lw * scale), dh = (int)(lh * scale);
        int dx = (width - dw) / 2, dy = 3;
        blit_scaled(pixels, width, height, dx, dy, dw, dh,
                    menu->logo_img, lw, lh);
        logo_h_used = dy + dh + 3;
    } else {
        ttf_text_centered(pixels, width, height, 8, _("OPENCAPTIVE"), title, 0xFFFF8800);
        logo_h_used = 45;
    }
    ttf_text_centered(pixels, width, height, logo_h_used, _("BY DANIEL NYLANDER"), small, 0xFF666688);

    int card_w = 390, card_h = 280, card_y = logo_h_used + 18, gap = 30;
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

    int label_y = card_y + card_h + 4;
    int ltw = 0;

    // Game titles with data status indicators
    {
        const char *cap_label = "CAPTIVE";
        if (body) TTF_GetStringSize(body, cap_label, 0, &ltw, NULL);
        int lx = cx0 + (card_w - ltw - 24) / 2;
        ttf_text(pixels, width, height, lx, label_y, cap_label, body, 0xFFCCCCCC);
        const char *cap_status = menu->captive_data_ok ? "\xe2\x9c\x93" : "\xe2\x9c\x97";
        uint32_t cap_scol = menu->captive_data_ok ? 0xFF44FF44 : 0xFFFF4444;
        ttf_text(pixels, width, height, lx + ltw + 8, label_y, cap_status, body, cap_scol);
    }
    {
        const char *lib_label = "LIBERATION";
        if (body) TTF_GetStringSize(body, lib_label, 0, &ltw, NULL);
        int lx = cx1 + (card_w - ltw - 24) / 2;
        ttf_text(pixels, width, height, lx, label_y, lib_label, body, 0xFFCCCCCC);
        const char *lib_status = menu->liberation_data_ok ? "\xe2\x9c\x93" : "\xe2\x9c\x97";
        uint32_t lib_scol = menu->liberation_data_ok ? 0xFF44FF44 : 0xFFFF4444;
        ttf_text(pixels, width, height, lx + ltw + 8, label_y, lib_status, body, lib_scol);
    }

    int year_y = label_y + 22;
    int ctw = 0;
    if (small) TTF_GetStringSize(small, "1990", 0, &ctw, NULL);
    ttf_text(pixels, width, height, cx0 + (card_w - ctw) / 2, year_y, "1990", small, 0xFF888888);
    if (small) TTF_GetStringSize(small, "1993", 0, &ctw, NULL);
    ttf_text(pixels, width, height, cx1 + (card_w - ctw) / 2, year_y, "1993", small, 0xFF888888);

    // Continue buttons (row 2-3)
    int continue_y = year_y + 22;
    {
        const char *cont_cap = menu->captive_save_exists ? _("CONTINUE") : "";
        const char *cont_lib = menu->liberation_save_exists ? _("CONTINUE") : "";
        bool sel2 = (menu->selected_item == 2);
        bool sel3 = (menu->selected_item == 3);
        if (cont_cap[0]) {
            if (sel2) draw_rect(pixels, width, height, cx0, continue_y - 2, card_w, 24, 0xFF333366);
            int tw2 = 0; if (body) TTF_GetStringSize(body, cont_cap, 0, &tw2, NULL);
            ttf_text(pixels, width, height, cx0 + (card_w - tw2) / 2, continue_y, cont_cap,
                     small, sel2 ? 0xFFFFFF00 : 0xFF88AA88);
        }
        if (cont_lib[0]) {
            if (sel3) draw_rect(pixels, width, height, cx1, continue_y - 2, card_w, 24, 0xFF333366);
            int tw2 = 0; if (body) TTF_GetStringSize(body, cont_lib, 0, &tw2, NULL);
            ttf_text(pixels, width, height, cx1 + (card_w - tw2) / 2, continue_y, cont_lib,
                     small, sel3 ? 0xFFFFFF00 : 0xFF88AA88);
        }
    }

    // Row: Settings / About
    int bottom_y = continue_y + 26;
    {
        const char *btns1[] = { _("SETTINGS"), _("ABOUT") };
        int bidx1[] = { 4, 5 };
        for (int i = 0; i < 2; i++) {
            int bx = cx0 + i * (card_w + gap);
            bool sel = (menu->selected_item == bidx1[i]);
            if (sel) draw_rect(pixels, width, height, bx, bottom_y - 2, card_w, 26, 0xFF333366);
            uint32_t col = sel ? 0xFFFFFFFF : 0xFFAAAAAA;
            int btw = 0;
            if (body) TTF_GetStringSize(body, btns1[i], 0, &btw, NULL);
            ttf_text(pixels, width, height, bx + (card_w - btw) / 2, bottom_y, btns1[i], body, col);
        }
    }

    // Row: Controls / Quit
    int bottom2_y = bottom_y + 28;
    {
        const char *btns2[] = { _("CONTROLS"), _("QUIT") };
        int bidx2[] = { 6, 7 };
        for (int i = 0; i < 2; i++) {
            int bx = cx0 + i * (card_w + gap);
            bool sel = (menu->selected_item == bidx2[i]);
            if (sel) draw_rect(pixels, width, height, bx, bottom2_y - 2, card_w, 26, 0xFF333366);
            uint32_t col = sel ? 0xFFFFFFFF : 0xFFAAAAAA;
            int btw = 0;
            if (body) TTF_GetStringSize(body, btns2[i], 0, &btw, NULL);
            ttf_text(pixels, width, height, bx + (card_w - btw) / 2, bottom2_y, btns2[i], body, col);
        }
    }

    ttf_text_centered(pixels, width, height, height - 25,
                       _("ARROWS: SELECT  ENTER: START  D: SCAN DATA  F1: CONTROLS  F10: IN-GAME OPTIONS"),
                       small, 0xFF444444);
    draw_border(pixels, width, height, 10, 10, width - 20, height - 20, 0xFF444488, 2);

    char ver[32];
    snprintf(ver, sizeof(ver), "v%d.%d.%d",
             OPENCAPTIVE_VERSION_MAJOR, OPENCAPTIVE_VERSION_MINOR, OPENCAPTIVE_VERSION_PATCH);
    ttf_text(pixels, width, height, 20, height - 25, ver, small, 0xFF333333);
}
