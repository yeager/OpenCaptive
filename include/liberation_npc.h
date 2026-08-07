#ifndef LIBERATION_NPC_H
#define LIBERATION_NPC_H
#include <stdint.h>
#include <stdbool.h>

#define LIB_MAX_NPCS 16

typedef enum {
    NPC_PEDESTRIAN = 0,
    NPC_POLICE = 1,
    NPC_VENDOR = 2,
} NPCType;

typedef struct {
    int16_t x, y;
    uint8_t dir;       // 0-3
    NPCType type;
    bool active;
    uint8_t frame;     // animation frame
    uint8_t speed;     // ticks between moves
    uint8_t timer;
} CityNPC;

typedef struct {
    CityNPC npcs[LIB_MAX_NPCS];
    int count;
} CityNPCList;

void city_npc_init(CityNPCList *list, uint32_t seed);
void city_npc_tick(CityNPCList *list, uint32_t tick);
void city_npc_render(const CityNPCList *list, uint32_t *fb, int w, int h,
                     int party_x, int party_y, int party_dir);
bool city_npc_police_chase(CityNPCList *list, int party_x, int party_y,
                           int crime_level);

#endif
