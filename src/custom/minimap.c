#include "custom_features.h"
#include "game_state.h"
#include <string.h>

static uint32_t alpha_blend(uint32_t bg, uint32_t fg, float opacity) {
    uint8_t fr = (fg >> 16) & 0xFF;
    uint8_t fg_ = (fg >> 8) & 0xFF;
    uint8_t fb = fg & 0xFF;
    uint8_t br = (bg >> 16) & 0xFF;
    uint8_t bg_ = (bg >> 8) & 0xFF;
    uint8_t bb = bg & 0xFF;
    uint8_t r = (uint8_t)(fr * opacity + br * (1.0f - opacity));
    uint8_t g = (uint8_t)(fg_ * opacity + bg_ * (1.0f - opacity));
    uint8_t b = (uint8_t)(fb * opacity + bb * (1.0f - opacity));
    return 0xFF000000 | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

void minimap_render(uint32_t *fb, int fb_w, int fb_h,
                    const void *level_ptr, int party_x, int party_y,
                    int party_dir, const bool *visited,
                    const CustomFeatures *feat) {
    if (!feat->minimap || !level_ptr) return;

    const DungeonLevel *level = (const DungeonLevel *)level_ptr;
    int size = feat->minimap_size;
    if (size < 32) size = 32;
    if (size > 192) size = 192;
    float opacity = feat->minimap_opacity;
    if (opacity < 0.1f) opacity = 0.1f;
    if (opacity > 1.0f) opacity = 1.0f;

    int map_x = fb_w - size - 4;
    int map_y = 4;

    int view_radius = size / 4;
    int cx = party_x;
    int cy = party_y;

    for (int py = 0; py < size; py++) {
        for (int px = 0; px < size; px++) {
            int mx = cx - view_radius + (px * view_radius * 2) / size;
            int my = cy - view_radius + (py * view_radius * 2) / size;

            uint32_t color = 0xFF101010;

            if (mx >= 0 && mx < MAP_WIDTH && my >= 0 && my < MAP_HEIGHT) {
                bool show = true;
                if (visited) {
                    int idx = level->level * MAP_WIDTH * MAP_HEIGHT + my * MAP_WIDTH + mx;
                    show = visited[idx];
                }

                if (show) {
                    CellType ct = level->cells[my][mx].type;
                    switch (ct) {
                        case CELL_WALL:        color = 0xFF404040; break;
                        case CELL_FLOOR:       color = 0xFF808080; break;
                        case CELL_DOOR:        color = 0xFF8B4513; break;
                        case CELL_DOOR_LOCKED: color = 0xFFFF4500; break;
                        case CELL_STAIRS_UP:   color = 0xFF00FF00; break;
                        case CELL_STAIRS_DOWN: color = 0xFF00CC00; break;
                        case CELL_SHOP:        color = 0xFFFFD700; break;
                        case CELL_TELEPORTER:  color = 0xFF8A2BE2; break;
                        case CELL_GENERATOR:   color = 0xFFFF0000; break;
                        case CELL_TERMINAL:    color = 0xFF00BFFF; break;
                    }
                }

                if (mx == party_x && my == party_y) {
                    color = 0xFFFFFFFF;
                }
            }

            int dx = map_x + px;
            int dy = map_y + py;
            if (dx >= 0 && dx < fb_w && dy >= 0 && dy < fb_h) {
                fb[dy * fb_w + dx] = alpha_blend(fb[dy * fb_w + dx], color, opacity);
            }
        }
    }

    // Draw direction indicator
    int arrow_cx = map_x + size / 2;
    int arrow_cy = map_y + size / 2;
    int arrow_len = 3;
    static const int dir_dx[] = {0, 1, 0, -1};
    static const int dir_dy[] = {-1, 0, 1, 0};
    for (int i = 1; i <= arrow_len; i++) {
        int ax = arrow_cx + dir_dx[party_dir] * i;
        int ay = arrow_cy + dir_dy[party_dir] * i;
        if (ax >= 0 && ax < fb_w && ay >= 0 && ay < fb_h)
            fb[ay * fb_w + ax] = 0xFFFFFF00;
    }
}
