#include "liberation_npc.h"
#include <string.h>

/* Simple PRNG */
static uint32_t npc_rng(uint32_t *state) {
    *state = *state * 1103515245u + 12345u;
    return (*state >> 16) & 0x7FFF;
}

static void put_px(uint32_t *fb, int w, int h, int x, int y, uint32_t c) {
    if (x >= 0 && x < w && y >= 0 && y < h)
        fb[y * w + x] = c;
}

void city_npc_init(CityNPCList *list, uint32_t seed) {
    if (!list) return;
    memset(list, 0, sizeof(*list));
    uint32_t rng = seed;
    int count = 8 + (int)(npc_rng(&rng) % 5); /* 8-12 */
    if (count > LIB_MAX_NPCS) count = LIB_MAX_NPCS;
    list->count = count;
    for (int i = 0; i < count; i++) {
        CityNPC *n = &list->npcs[i];
        n->active = true;
        n->x = (int16_t)(npc_rng(&rng) % 32);
        n->y = (int16_t)(npc_rng(&rng) % 32);
        n->dir = (uint8_t)(npc_rng(&rng) % 4);
        n->frame = 0;
        n->speed = (uint8_t)(4 + npc_rng(&rng) % 5); /* 4-8 */
        n->timer = 0;
        /* Distribute types: ~60% pedestrian, ~20% police, ~20% vendor */
        uint32_t r = npc_rng(&rng) % 10;
        if (r < 6) n->type = NPC_PEDESTRIAN;
        else if (r < 8) n->type = NPC_POLICE;
        else n->type = NPC_VENDOR;
    }
}

void city_npc_tick(CityNPCList *list, uint32_t tick) {
    if (!list) return;
    for (int i = 0; i < list->count; i++) {
        CityNPC *n = &list->npcs[i];
        if (!n->active) continue;
        n->timer++;
        if (n->timer < n->speed) continue;
        n->timer = 0;
        n->frame = (n->frame + 1) & 3;

        /* Move in current direction */
        static const int dx[] = {0, 1, 0, -1};
        static const int dy[] = {-1, 0, 1, 0};
        n->x += (int16_t)dx[n->dir];
        n->y += (int16_t)dy[n->dir];

        /* Wrap within city bounds */
        if (n->x < 0) n->x = 31;
        if (n->x > 31) n->x = 0;
        if (n->y < 0) n->y = 31;
        if (n->y > 31) n->y = 0;

        /* Randomly change direction at "intersections" (every few moves) */
        uint32_t rng = tick * 31 + (uint32_t)i * 7 + (uint32_t)n->x * 13 + (uint32_t)n->y * 17;
        if ((npc_rng(&rng) % 4) == 0) {
            n->dir = (uint8_t)(npc_rng(&rng) % 4);
        }
    }
}

void city_npc_render(const CityNPCList *list, uint32_t *fb, int w, int h,
                     int party_x, int party_y, int party_dir) {
    if (!list || !fb) return;
    (void)party_dir;

    for (int i = 0; i < list->count; i++) {
        const CityNPC *n = &list->npcs[i];
        if (!n->active) continue;

        int dx = n->x - party_x;
        int dy = n->y - party_y;
        if (dx < -4 || dx > 4 || dy < -4 || dy > 4) continue;

        /* Screen position: center of screen + offset */
        int sx = w / 2 + dx * 12;
        int sy = h / 2 + dy * 12;

        uint32_t color;
        switch (n->type) {
            case NPC_POLICE:     color = 0xFFFFFF00; break; /* yellow */
            case NPC_VENDOR:     color = 0xFF00CC00; break; /* green */
            default:             color = 0xFF4488FF; break; /* blue */
        }

        /* Draw 4x8 sprite */
        int anim_offset = (n->frame & 1) ? 1 : 0;
        for (int py = 0; py < 8; py++) {
            for (int px = 0; px < 4; px++) {
                put_px(fb, w, h, sx + px, sy + py + anim_offset, color);
            }
        }
    }
}

bool city_npc_police_chase(CityNPCList *list, int party_x, int party_y,
                           int crime_level) {
    if (!list || crime_level <= 0) return false;

    for (int i = 0; i < list->count; i++) {
        CityNPC *n = &list->npcs[i];
        if (!n->active || n->type != NPC_POLICE) continue;

        /* Move police toward party */
        int dx = party_x - n->x;
        int dy = party_y - n->y;

        if (dx > 0) n->x++;
        else if (dx < 0) n->x--;
        if (dy > 0) n->y++;
        else if (dy < 0) n->y--;

        /* Check if adjacent (arrest) */
        int adx = party_x - n->x;
        int ady = party_y - n->y;
        if (adx >= -1 && adx <= 1 && ady >= -1 && ady <= 1) {
            return true; /* Arrested */
        }
    }
    return false;
}
