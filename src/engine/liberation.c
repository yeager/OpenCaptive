#include "liberation.h"
#include "opencaptive.h"
#include <stdio.h>
#include <string.h>

#define LIB_SAVE_MAGIC 0x4C425356U /* LBSV */
#define LIB_SAVE_VERSION 1U

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t state_size;
} LibSaveHeader;

static uint32_t lib_prng;
static uint32_t lib_rand(void) {
    lib_prng = lib_prng * 1103515245 + 12345;
    return (lib_prng >> 16) & 0x7FFF;
}

void lib_init(LibState *ls, uint32_t seed) {
    memset(ls, 0, sizeof(*ls));
    ls->city.city_seed = seed;
    ls->mode = LIB_MODE_CITY;
    ls->current_building = -1;
    ls->player_dir = DIR_NORTH;
    lib_generate_city(ls);

    // Place player at city center
    ls->player_cx = LIB_CITY_WIDTH / 2;
    ls->player_cy = LIB_CITY_HEIGHT / 2;

    // Pick a random target building
    if (ls->city.num_buildings > 0) {
        lib_prng = seed + 999;
        ls->target_building = lib_rand() % ls->city.num_buildings;
    }
}

void lib_generate_city(LibState *ls) {
    lib_prng = ls->city.city_seed;
    LibCity *c = &ls->city;

    // Fill grid with streets
    for (int y = 0; y < LIB_CITY_HEIGHT; y++)
        for (int x = 0; x < LIB_CITY_WIDTH; x++)
            c->grid[y][x] = LIB_BUILDING_NONE;

    // Place buildings in a grid pattern with streets
    static const LibBuildingType types[] = {
        LIB_BUILDING_RESIDENTIAL, LIB_BUILDING_COMMERCIAL,
        LIB_BUILDING_INDUSTRIAL, LIB_BUILDING_GOVERNMENT,
        LIB_BUILDING_HOSPITAL, LIB_BUILDING_POLICE,
        LIB_BUILDING_SHOP, LIB_BUILDING_PRISON,
    };
    int num_types = sizeof(types) / sizeof(types[0]);

    c->num_buildings = 0;

    // Place buildings in a 4x4 block pattern with 2-wide streets
    for (int by = 0; by < 6; by++) {
        for (int bx = 0; bx < 6; bx++) {
            if (c->num_buildings >= LIB_MAX_BUILDINGS) break;

            int cx = 2 + bx * 5;
            int cy = 2 + by * 5;
            int bw = 3 + lib_rand() % 2;
            int bh = 3 + lib_rand() % 2;

            if (cx + bw >= LIB_CITY_WIDTH || cy + bh >= LIB_CITY_HEIGHT) continue;

            LibBuildingType type = types[lib_rand() % num_types];
            LibBuilding *b = &c->buildings[c->num_buildings];
            b->city_x = cx;
            b->city_y = cy;
            b->width = bw;
            b->height = bh;
            b->type = type;
            b->seed = lib_rand();
            b->num_floors = 1 + lib_rand() % LIB_BUILDING_FLOORS;

            for (int y = cy; y < cy + bh; y++)
                for (int x = cx; x < cx + bw; x++)
                    c->grid[y][x] = type;

            lib_generate_building(b, type, b->seed);
            c->num_buildings++;
        }
    }
}

void lib_generate_building(LibBuilding *b, LibBuildingType type, uint32_t seed) {
    lib_prng = seed;

    for (int f = 0; f < b->num_floors; f++) {
        LibFloor *fl = &b->floors[f];
        memset(fl, 0, sizeof(*fl));

        // Walls around perimeter
        for (int y = 0; y < LIB_FLOOR_HEIGHT; y++) {
            for (int x = 0; x < LIB_FLOOR_WIDTH; x++) {
                if (y == 0 || y == LIB_FLOOR_HEIGHT - 1 ||
                    x == 0 || x == LIB_FLOOR_WIDTH - 1) {
                    fl->cells[y][x] = LIB_CELL_WALL;
                } else {
                    fl->cells[y][x] = LIB_CELL_FLOOR;
                }
            }
        }

        // Internal walls creating rooms
        int num_walls = 2 + lib_rand() % 4;
        for (int w = 0; w < num_walls; w++) {
            bool horiz = lib_rand() % 2;
            if (horiz) {
                int wy = 3 + lib_rand() % (LIB_FLOOR_HEIGHT - 6);
                for (int x = 1; x < LIB_FLOOR_WIDTH - 1; x++)
                    fl->cells[wy][x] = LIB_CELL_WALL;
                // Door
                int dx = 2 + lib_rand() % (LIB_FLOOR_WIDTH - 4);
                fl->cells[wy][dx] = LIB_CELL_DOOR;
            } else {
                int wx = 3 + lib_rand() % (LIB_FLOOR_WIDTH - 6);
                for (int y = 1; y < LIB_FLOOR_HEIGHT - 1; y++)
                    fl->cells[y][wx] = LIB_CELL_WALL;
                int dy = 2 + lib_rand() % (LIB_FLOOR_HEIGHT - 4);
                fl->cells[dy][wx] = LIB_CELL_DOOR;
            }
        }

        // Entrance door on ground floor
        if (f == 0) {
            fl->cells[LIB_FLOOR_HEIGHT - 1][LIB_FLOOR_WIDTH / 2] = LIB_CELL_DOOR;
        }

        // Elevator/stairs between floors
        if (b->num_floors > 1) {
            int ex = LIB_FLOOR_WIDTH - 3;
            int ey = 2;
            fl->cells[ey][ex] = LIB_CELL_ELEVATOR;
        }

        // Terminal in some rooms
        if (lib_rand() % 3 == 0) {
            int tx = 2 + lib_rand() % (LIB_FLOOR_WIDTH - 4);
            int ty = 2 + lib_rand() % (LIB_FLOOR_HEIGHT - 4);
            if (fl->cells[ty][tx] == LIB_CELL_FLOOR)
                fl->cells[ty][tx] = LIB_CELL_TERMINAL;
        }

        // NPC placement
        if (type == LIB_BUILDING_COMMERCIAL || type == LIB_BUILDING_SHOP) {
            int np = 1 + lib_rand() % 3;
            for (int n = 0; n < np; n++) {
                int nx = 2 + lib_rand() % (LIB_FLOOR_WIDTH - 4);
                int ny = 2 + lib_rand() % (LIB_FLOOR_HEIGHT - 4);
                if (fl->cells[ny][nx] == LIB_CELL_FLOOR)
                    fl->cells[ny][nx] = LIB_CELL_NPC;
            }
        }
    }
}

bool lib_enter_current_building(LibState *ls) {
    if (!ls || ls->mode != LIB_MODE_CITY ||
        ls->player_cx < 0 || ls->player_cx >= LIB_CITY_WIDTH ||
        ls->player_cy < 0 || ls->player_cy >= LIB_CITY_HEIGHT ||
        ls->city.grid[ls->player_cy][ls->player_cx] == LIB_BUILDING_NONE)
        return false;

    for (int i = 0; i < ls->city.num_buildings; i++) {
        const LibBuilding *building = &ls->city.buildings[i];
        if (ls->player_cx < building->city_x ||
            ls->player_cx >= building->city_x + building->width ||
            ls->player_cy < building->city_y ||
            ls->player_cy >= building->city_y + building->height)
            continue;
        ls->current_building = i;
        ls->mode = LIB_MODE_BUILDING;
        ls->player_bx = LIB_FLOOR_WIDTH / 2;
        ls->player_by = LIB_FLOOR_HEIGHT - 2;
        ls->player_floor = 0;
        if (i == ls->target_building) ls->mission_complete = true;
        return true;
    }
    return false;
}

bool lib_leave_current_building(LibState *ls) {
    if (!ls || ls->mode != LIB_MODE_BUILDING || ls->current_building < 0)
        return false;
    ls->mode = LIB_MODE_CITY;
    ls->current_building = -1;
    return true;
}

bool lib_change_floor(LibState *ls, int direction) {
    if (!ls || ls->mode != LIB_MODE_BUILDING || (direction != -1 && direction != 1) ||
        ls->current_building < 0 || ls->current_building >= ls->city.num_buildings)
        return false;
    const LibBuilding *building = &ls->city.buildings[ls->current_building];
    if (ls->player_floor < 0 || ls->player_floor >= building->num_floors ||
        ls->player_bx < 0 || ls->player_bx >= LIB_FLOOR_WIDTH ||
        ls->player_by < 0 || ls->player_by >= LIB_FLOOR_HEIGHT ||
        building->floors[ls->player_floor].cells[ls->player_by][ls->player_bx]
            != LIB_CELL_ELEVATOR)
        return false;
    int next_floor = ls->player_floor + direction;
    if (next_floor < 0 || next_floor >= building->num_floors) return false;
    ls->player_floor = next_floor;
    return true;
}

static bool lib_state_valid(const LibState *ls) {
    if (!ls || ls->city.num_buildings < 1 ||
        ls->city.num_buildings > LIB_MAX_BUILDINGS ||
        ls->mode < LIB_MODE_CITY || ls->mode > LIB_MODE_BUILDING ||
        ls->player_dir < DIR_NORTH || ls->player_dir > DIR_WEST ||
        ls->target_building < 0 || ls->target_building >= ls->city.num_buildings ||
        ls->player_cx < 0 || ls->player_cx >= LIB_CITY_WIDTH ||
        ls->player_cy < 0 || ls->player_cy >= LIB_CITY_HEIGHT)
        return false;

    for (int y = 0; y < LIB_CITY_HEIGHT; y++) {
        for (int x = 0; x < LIB_CITY_WIDTH; x++) {
            if (ls->city.grid[y][x] < LIB_BUILDING_NONE ||
                ls->city.grid[y][x] > LIB_BUILDING_SHOP) return false;
        }
    }
    for (int i = 0; i < ls->city.num_buildings; i++) {
        const LibBuilding *building = &ls->city.buildings[i];
        if (building->type < LIB_BUILDING_RESIDENTIAL ||
            building->type > LIB_BUILDING_SHOP || building->width < 1 ||
            building->height < 1 || building->city_x < 0 || building->city_y < 0 ||
            building->city_x + building->width > LIB_CITY_WIDTH ||
            building->city_y + building->height > LIB_CITY_HEIGHT ||
            building->num_floors < 1 || building->num_floors > LIB_BUILDING_FLOORS)
            return false;
        for (int y = building->city_y; y < building->city_y + building->height; y++)
            for (int x = building->city_x; x < building->city_x + building->width; x++)
                if (ls->city.grid[y][x] != building->type) return false;
        for (int floor = 0; floor < building->num_floors; floor++)
            for (int y = 0; y < LIB_FLOOR_HEIGHT; y++)
                for (int x = 0; x < LIB_FLOOR_WIDTH; x++)
                    if (building->floors[floor].cells[y][x] < LIB_CELL_VOID ||
                        building->floors[floor].cells[y][x] > LIB_CELL_NPC) return false;
    }
    if (ls->mode == LIB_MODE_CITY) return ls->current_building == -1;
    return ls->current_building >= 0 && ls->current_building < ls->city.num_buildings &&
        ls->player_floor >= 0 &&
        ls->player_floor < ls->city.buildings[ls->current_building].num_floors &&
        ls->player_bx >= 0 && ls->player_bx < LIB_FLOOR_WIDTH &&
        ls->player_by >= 0 && ls->player_by < LIB_FLOOR_HEIGHT;
}

bool lib_save_game(const LibState *ls, const char *path) {
    if (!path || !lib_state_valid(ls)) return false;
    FILE *file = fopen(path, "wb");
    if (!file) return false;
    LibSaveHeader header = {
        .magic = LIB_SAVE_MAGIC,
        .version = LIB_SAVE_VERSION,
        .state_size = sizeof(*ls),
    };
    bool ok = fwrite(&header, sizeof(header), 1, file) == 1 &&
        fwrite(ls, sizeof(*ls), 1, file) == 1;
    if (fclose(file) != 0) ok = false;
    return ok;
}

bool lib_load_game(LibState *ls, const char *path) {
    if (!ls || !path) return false;
    FILE *file = fopen(path, "rb");
    if (!file) return false;
    LibSaveHeader header;
    LibState restored;
    bool ok = fread(&header, sizeof(header), 1, file) == 1 &&
        header.magic == LIB_SAVE_MAGIC && header.version == LIB_SAVE_VERSION &&
        header.state_size == sizeof(restored) &&
        fread(&restored, sizeof(restored), 1, file) == 1 && lib_state_valid(&restored);
    if (fclose(file) != 0) ok = false;
    if (!ok) return false;
    *ls = restored;
    return true;
}

static const uint32_t building_colors[] = {
    [LIB_BUILDING_NONE]        = 0xFF222222,
    [LIB_BUILDING_RESIDENTIAL] = 0xFF445566,
    [LIB_BUILDING_COMMERCIAL]  = 0xFF556644,
    [LIB_BUILDING_INDUSTRIAL]  = 0xFF665544,
    [LIB_BUILDING_GOVERNMENT]  = 0xFF444466,
    [LIB_BUILDING_PRISON]      = 0xFF664444,
    [LIB_BUILDING_HOSPITAL]    = 0xFF446644,
    [LIB_BUILDING_POLICE]      = 0xFF444488,
    [LIB_BUILDING_SHOP]        = 0xFF666644,
};

static void lib_rect(uint32_t *pixels, int width, int height,
                     int x, int y, int w, int h, uint32_t color) {
    int x0 = x < 0 ? 0 : x;
    int y0 = y < 0 ? 0 : y;
    int x1 = x + w > width ? width : x + w;
    int y1 = y + h > height ? height : y + h;
    for (int py = y0; py < y1; ++py)
        for (int px = x0; px < x1; ++px)
            pixels[py * width + px] = color;
}

static void lib_frame(uint32_t *pixels, int width, int height,
                      int x, int y, int w, int h, uint32_t edge, uint32_t fill) {
    lib_rect(pixels, width, height, x, y, w, h, fill);
    lib_rect(pixels, width, height, x, y, w, 2, edge);
    lib_rect(pixels, width, height, x, y + h - 2, w, 2, edge);
    lib_rect(pixels, width, height, x, y, 2, h, edge);
    lib_rect(pixels, width, height, x + w - 2, y, 2, h, edge);
}

void lib_render_city(const LibState *ls, uint32_t *pixels, int width, int height) {
    memset(pixels, 0, (size_t)width * height * sizeof(uint32_t));

    /* This is deliberately a layout renderer, not a substitute for the
     * original CD assets.  It mirrors the native screen regions so UI and
     * gameplay never inherit Captive's 320x200 geometry. */
    lib_rect(pixels, width, height, 0, 0, width, height, 0xFF09081B);
    lib_rect(pixels, width, height, 0, 35, width, 2, 0xFF30245D);
    for (int module = 0; module < 4; ++module) {
        int x = 8 + module * 78;
        lib_frame(pixels, width, height, x, 5, 70, 25, 0xFF7860A0, 0xFF201B3D);
        lib_rect(pixels, width, height, x + 5, 10, 16, 5, 0xFF4D987D);
        lib_rect(pixels, width, height, x + 25, 10, 36, 5, 0xFF3C3567);
        lib_rect(pixels, width, height, x + 5, 19, 56, 5, 0xFF2D2850);
    }

    for (int side = 0; side < 2; ++side) {
        int x = side == 0 ? 6 : width - 78;
        lib_frame(pixels, width, height, x, 48, 72, 122, 0xFF69588E, 0xFF17142D);
        lib_rect(pixels, width, height, x + 12, 58, 48, 42, 0xFF2A254A);
        lib_rect(pixels, width, height, x + 24, 64, 24, 25, 0xFF577A82);
        lib_rect(pixels, width, height, x + 17, 102, 38, 7, 0xFF4B3B71);
        lib_rect(pixels, width, height, x + 12, 117, 48, 5, 0xFF38416A);
        lib_rect(pixels, width, height, x + 12, 130, 48, 5, 0xFF38416A);
        lib_rect(pixels, width, height, x + 12, 143, 48, 5, 0xFF38416A);
    }
    lib_frame(pixels, width, height, LIBERATION_VIEWPORT_X - 3,
              LIBERATION_VIEWPORT_Y - 3, LIBERATION_VIEWPORT_WIDTH + 6,
              LIBERATION_VIEWPORT_HEIGHT + 6, 0xFF8167A8, 0xFF100E24);

    int scale = 3;
    int ox = LIBERATION_VIEWPORT_X + (LIBERATION_VIEWPORT_WIDTH - LIB_CITY_WIDTH * scale) / 2;
    int oy = LIBERATION_VIEWPORT_Y + (LIBERATION_VIEWPORT_HEIGHT - LIB_CITY_HEIGHT * scale) / 2;

    // Draw city grid
    for (int cy = 0; cy < LIB_CITY_HEIGHT; cy++) {
        for (int cx = 0; cx < LIB_CITY_WIDTH; cx++) {
            LibBuildingType bt = ls->city.grid[cy][cx];
            uint32_t color = (bt == LIB_BUILDING_NONE) ? 0xFF333333 : building_colors[bt];

            for (int sy = 0; sy < scale; sy++)
                for (int sx = 0; sx < scale; sx++) {
                    int px = ox + cx * scale + sx;
                    int py = oy + cy * scale + sy;
                    if (px >= 0 && px < width && py >= 0 && py < height)
                        pixels[py * width + px] = color;
                }
        }
    }

    // Player position (blinking)
    uint32_t player_col = 0xFFFFFFFF;
    for (int sy = 0; sy < scale; sy++)
        for (int sx = 0; sx < scale; sx++) {
            int px = ox + ls->player_cx * scale + sx;
            int py = oy + ls->player_cy * scale + sy;
            if (px >= 0 && px < width && py >= 0 && py < height)
                pixels[py * width + px] = player_col;
        }

    // Target building marker
    if (ls->target_building >= 0 && ls->target_building < ls->city.num_buildings) {
        const LibBuilding *tb = &ls->city.buildings[ls->target_building];
        int tcx = tb->city_x + tb->width / 2;
        int tcy = tb->city_y + tb->height / 2;
        for (int sy = 0; sy < scale; sy++)
            for (int sx = 0; sx < scale; sx++) {
                int px = ox + tcx * scale + sx;
                int py = oy + tcy * scale + sy;
                if (px >= 0 && px < width && py >= 0 && py < height)
                        pixels[py * width + px] = ls->mission_complete
                            ? 0xFF44FF44 : 0xFFFF4444;
            }
    }
}

void lib_render_building(const LibState *ls, uint32_t *pixels, int width, int height) {
    if (ls->current_building < 0) return;

    const LibBuilding *b = &ls->city.buildings[ls->current_building];
    if (ls->player_floor >= b->num_floors) return;
    const LibFloor *fl = &b->floors[ls->player_floor];

    int vp_w = LIBERATION_VIEWPORT_WIDTH;
    int vp_h = LIBERATION_VIEWPORT_HEIGHT;

    // Top-down minimap in viewport area
    int scale = 6;
    int ox = LIBERATION_VIEWPORT_X + (vp_w - LIB_FLOOR_WIDTH * scale) / 2;
    int oy = LIBERATION_VIEWPORT_Y + (vp_h - LIB_FLOOR_HEIGHT * scale) / 2;

    for (int fy = 0; fy < LIB_FLOOR_HEIGHT; fy++) {
        for (int fx = 0; fx < LIB_FLOOR_WIDTH; fx++) {
            uint32_t color = 0;
            switch (fl->cells[fy][fx]) {
                case LIB_CELL_WALL:     color = 0xFF555577; break;
                case LIB_CELL_FLOOR:    color = 0xFF333344; break;
                case LIB_CELL_DOOR:     color = 0xFF5555AA; break;
                case LIB_CELL_ELEVATOR: color = 0xFF55AA55; break;
                case LIB_CELL_STAIRS:   color = 0xFF55AA55; break;
                case LIB_CELL_TERMINAL: color = 0xFF00FF00; break;
                case LIB_CELL_NPC:      color = 0xFFFF8800; break;
                default: break;
            }
            if (!color) continue;

            for (int sy = 0; sy < scale; sy++)
                for (int sx = 0; sx < scale; sx++) {
                    int px = ox + fx * scale + sx;
                    int py = oy + fy * scale + sy;
                    if (px >= LIBERATION_VIEWPORT_X && px < LIBERATION_VIEWPORT_X + vp_w &&
                        py >= LIBERATION_VIEWPORT_Y && py < LIBERATION_VIEWPORT_Y + vp_h &&
                        px < width && py < height)
                        pixels[py * width + px] = color;
                }
        }
    }

    // Player
    for (int sy = 0; sy < scale; sy++)
        for (int sx = 0; sx < scale; sx++) {
            int px = ox + ls->player_bx * scale + sx;
            int py = oy + ls->player_by * scale + sy;
            if (px >= LIBERATION_VIEWPORT_X && px < LIBERATION_VIEWPORT_X + vp_w &&
                py >= LIBERATION_VIEWPORT_Y && py < LIBERATION_VIEWPORT_Y + vp_h &&
                px < width && py < height)
                pixels[py * width + px] = 0xFFFFFFFF;
        }
}
