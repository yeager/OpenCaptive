#ifndef LIBERATION_BUILDING_INTERACT_H
#define LIBERATION_BUILDING_INTERACT_H

#include <stdbool.h>
#include <stdint.h>
#include "liberation_citygen.h"
#include "liberation_citygen_grid.h"
#include "liberation_shop.h"
#include "liberation_dialogue.h"

typedef enum {
    INTERACT_NONE,
    INTERACT_SHOP,
    INTERACT_BAR,
    INTERACT_BUSINESS,
    INTERACT_LIBRARY,
    INTERACT_POLICE,
    INTERACT_RECORDS,
    INTERACT_RESIDENCE,
    INTERACT_INDUSTRIAL,
    INTERACT_SPECIAL,
} InteractionType;

typedef struct {
    InteractionType type;
    bool active;
    LibShopState shop;
    DialogueState dialogue;
    int building_index;
    int *player_gold;
    struct {
        char     name[24];
        uint16_t item_type;
    } purchased[20];
    int purchased_count;
    bool mission_complete;
    bool bar_fight;
    bool fine_refused;
    bool fine_paid;
    bool industrial_hazard;
    bool shop_menu_active;
    bool special_investigated;
    int fine_node;
    int reputation;
    bool reputation_priced;
} BuildingInteraction;

void building_interact_init(BuildingInteraction *bi);

bool building_interact_enter(BuildingInteraction *bi,
                             const CityGridState *grid,
                             const CityGrid *buildings,
                             int cell_x, int cell_y,
                             int *player_gold,
                             const ItemDatabase *item_db);
void building_interact_set_bar_fight(BuildingInteraction *bi, bool pending);

void building_interact_choose(BuildingInteraction *bi, unsigned choice);
void building_interact_advance(BuildingInteraction *bi);
bool building_interact_buy(BuildingInteraction *bi, unsigned item_idx);
void building_interact_leave(BuildingInteraction *bi);
void building_interact_set_reputation(BuildingInteraction *bi, int reputation);

const char *building_interact_text(const BuildingInteraction *bi);
unsigned building_interact_choice_count(const BuildingInteraction *bi);
const char *building_interact_choice_label(const BuildingInteraction *bi,
                                            unsigned idx);

#endif
