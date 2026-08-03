#include "liberation_building_interact.h"
#include "i18n.h"
#include <string.h>
#include <stdio.h>

void building_interact_init(BuildingInteraction *bi) {
    memset(bi, 0, sizeof(*bi));
}

static InteractionType type_from_building(BuildingType bt) {
    switch (bt) {
        case BUILDING_SHOP:       return INTERACT_SHOP;
        case BUILDING_BAR:        return INTERACT_BAR;
        case BUILDING_BUSINESS:   return INTERACT_BUSINESS;
        case BUILDING_INDUSTRIAL: return INTERACT_INDUSTRIAL;
        case BUILDING_RESIDENCE:  return INTERACT_RESIDENCE;
        case BUILDING_LIBRARY:    return INTERACT_LIBRARY;
        case BUILDING_POLICE:     return INTERACT_POLICE;
        case BUILDING_RECORDS:    return INTERACT_RECORDS;
        case BUILDING_SPECIAL:    return INTERACT_SPECIAL;
        default:                  return INTERACT_NONE;
    }
}

static const char *generic_npc_for_type(InteractionType t) {
    switch (t) {
        case INTERACT_SHOP:       return "Shopkeeper";
        case INTERACT_BAR:        return "Bartender";
        case INTERACT_BUSINESS:   return "Receptionist";
        case INTERACT_LIBRARY:    return "Librarian";
        case INTERACT_POLICE:     return "Officer";
        case INTERACT_RECORDS:    return "Clerk";
        case INTERACT_RESIDENCE:  return "Resident";
        case INTERACT_INDUSTRIAL: return "Foreman";
        case INTERACT_SPECIAL:    return "Agent";
        default:                  return "NPC";
    }
}

static void build_generic_dialogue(BuildingInteraction *bi, const char *building_name) {
    const char *npc = generic_npc_for_type(bi->type);
    DialogueTree tree;
    dialogue_tree_init(&tree);

    char greeting[DIALOGUE_MAX_TEXT];
    unsigned exit_node = dialogue_tree_add_exit(&tree, _("You leave the building."));

    switch (bi->type) {
        case INTERACT_LIBRARY: {
            snprintf(greeting, sizeof(greeting),
                     _("Welcome to %s. The archives are available for public access."), building_name);
            unsigned info = dialogue_tree_add_text(&tree, npc,
                _("The city records show several generators in the surrounding buildings. "
                  "Check industrial and special zones."), exit_node);
            unsigned choice = dialogue_tree_add_choice(&tree, npc, greeting);
            dialogue_tree_add_option(&tree, choice, _("Search archives"), info);
            dialogue_tree_add_option(&tree, choice, _("Ask around"), info);
            dialogue_tree_add_option(&tree, choice, _("Leave"), exit_node);
            break;
        }
        case INTERACT_POLICE: {
            snprintf(greeting, sizeof(greeting),
                     _("This is the %s police station. State your business."), building_name);
            unsigned report = dialogue_tree_add_text(&tree, npc,
                _("We have reports of suspicious activity in the industrial district. "
                  "Stay alert out there."), exit_node);
            unsigned choice = dialogue_tree_add_choice(&tree, npc, greeting);
            dialogue_tree_add_option(&tree, choice, _("Ask for information"), report);
            dialogue_tree_add_option(&tree, choice, _("Leave"), exit_node);
            break;
        }
        case INTERACT_RECORDS: {
            snprintf(greeting, sizeof(greeting),
                     _("City Records Office. How can we help?"));
            unsigned lookup = dialogue_tree_add_text(&tree, npc,
                _("The building registry shows all commercial and industrial "
                  "properties in the city grid. Check the shops for supplies."), exit_node);
            unsigned choice = dialogue_tree_add_choice(&tree, npc, greeting);
            dialogue_tree_add_option(&tree, choice, _("Look up records"), lookup);
            dialogue_tree_add_option(&tree, choice, _("Leave"), exit_node);
            break;
        }
        case INTERACT_RESIDENCE: {
            snprintf(greeting, sizeof(greeting),
                     _("This is a private residence. What do you want?"));
            unsigned rumor = dialogue_tree_add_text(&tree, npc,
                _("I heard strange noises from the special building down the road. "
                  "Something's going on in there."), exit_node);
            unsigned choice = dialogue_tree_add_choice(&tree, npc, greeting);
            dialogue_tree_add_option(&tree, choice, _("Ask around"), rumor);
            dialogue_tree_add_option(&tree, choice, _("Leave"), exit_node);
            break;
        }
        case INTERACT_INDUSTRIAL: {
            snprintf(greeting, sizeof(greeting),
                     _("Welcome to %s. This area is restricted."), building_name);
            unsigned explore = dialogue_tree_add_text(&tree, npc,
                _("The machinery hums loudly. There might be useful equipment "
                  "stored in the back, but the foreman keeps it locked."), exit_node);
            unsigned choice = dialogue_tree_add_choice(&tree, npc, greeting);
            dialogue_tree_add_option(&tree, choice, _("Look around"), explore);
            dialogue_tree_add_option(&tree, choice, _("Leave"), exit_node);
            break;
        }
        case INTERACT_SPECIAL: {
            snprintf(greeting, sizeof(greeting),
                     _("%s. You shouldn't be here."), building_name);
            unsigned investigate = dialogue_tree_add_text(&tree, npc,
                _("This location appears to be connected to your mission. "
                  "The generator control panel is somewhere inside."), exit_node);
            unsigned choice = dialogue_tree_add_choice(&tree, npc, greeting);
            dialogue_tree_add_option(&tree, choice, _("Investigate"), investigate);
            dialogue_tree_add_option(&tree, choice, _("Leave"), exit_node);
            break;
        }
        default: {
            snprintf(greeting, sizeof(greeting),
                     _("This is %s. We're not taking visitors right now."), building_name);
            unsigned choice = dialogue_tree_add_choice(&tree, npc, greeting);
            dialogue_tree_add_option(&tree, choice, _("Leave"), exit_node);
            break;
        }
    }

    bi->shop.dialogue = tree;
    dialogue_state_init(&bi->dialogue, &bi->shop.dialogue);
    dialogue_state_start(&bi->dialogue);
    bi->dialogue.current_node = bi->shop.dialogue.node_count - 1;
}

bool building_interact_enter(BuildingInteraction *bi,
                             const CityGridState *grid,
                             const CityGrid *buildings,
                             int cell_x, int cell_y,
                             uint32_t *player_gold) {
    if (!bi || !grid || !buildings) return false;

    int offset = cell_y * CITYGRID_WIDTH + cell_x;
    if (offset < 0 || offset >= CITYGRID_CELLS) return false;

    uint8_t bid = grid->plane2[offset];
    if (bid == 0 || bid == 0xFF) return false;

    int bg_idx = (bid - 1) % buildings->total_buildings;
    if (bg_idx < 0 || bg_idx >= CITYGEN_MAX_BUILDINGS) return false;

    const CityBuilding *bld = &buildings->buildings[bg_idx];
    bi->type = type_from_building((BuildingType)bld->type);
    bi->building_index = bg_idx;
    bi->player_gold = player_gold;
    bi->active = true;

    lib_shop_init(&bi->shop, bld, (uint16_t)(offset ^ 0x1337));

    switch (bi->type) {
        case INTERACT_SHOP:
            lib_shop_generate_inventory(&bi->shop);
            lib_shop_generate_dialogue(&bi->shop, generic_npc_for_type(bi->type));
            dialogue_state_init(&bi->dialogue, &bi->shop.dialogue);
            dialogue_state_start(&bi->dialogue);
            bi->dialogue.current_node = bi->shop.dialogue.node_count - 1;
            break;
        case INTERACT_BAR:
            lib_shop_generate_bar_menu(&bi->shop);
            lib_shop_generate_dialogue(&bi->shop, generic_npc_for_type(bi->type));
            dialogue_state_init(&bi->dialogue, &bi->shop.dialogue);
            dialogue_state_start(&bi->dialogue);
            bi->dialogue.current_node = bi->shop.dialogue.node_count - 1;
            break;
        default:
            build_generic_dialogue(bi, bld->name);
            break;
    }

    return true;
}

void building_interact_choose(BuildingInteraction *bi, unsigned choice) {
    if (!bi || !bi->active) return;
    dialogue_state_choose(&bi->dialogue, choice);
    if (!dialogue_state_is_active(&bi->dialogue))
        bi->active = false;
}

void building_interact_advance(BuildingInteraction *bi) {
    if (!bi || !bi->active) return;
    if (!dialogue_state_advance(&bi->dialogue)) {
        if (bi->type == INTERACT_SPECIAL)
            bi->mission_complete = true;
        bi->active = false;
    }
}

bool building_interact_buy(BuildingInteraction *bi, unsigned item_idx) {
    if (!bi || !bi->active || !bi->player_gold) return false;
    if (bi->type != INTERACT_SHOP && bi->type != INTERACT_BAR) return false;
    if (item_idx >= bi->shop.item_count) return false;
    const LibShopItem *item = &bi->shop.items[item_idx];
    if (!lib_shop_buy_item(&bi->shop, item_idx, bi->player_gold)) return false;
    if (bi->purchased_count < 20) {
        snprintf(bi->purchased[bi->purchased_count].name,
                 sizeof(bi->purchased[0].name), "%s", item->name);
        bi->purchased[bi->purchased_count].item_type = item->item_type;
        bi->purchased_count++;
    }
    return true;
}

void building_interact_leave(BuildingInteraction *bi) {
    if (bi) bi->active = false;
}

const char *building_interact_text(const BuildingInteraction *bi) {
    if (!bi || !bi->active) return NULL;
    const DialogueNode *node = dialogue_state_current(&bi->dialogue);
    return node ? node->text : NULL;
}

unsigned building_interact_choice_count(const BuildingInteraction *bi) {
    if (!bi || !bi->active) return 0;
    const DialogueNode *node = dialogue_state_current(&bi->dialogue);
    if (!node || node->type != DIALOGUE_NODE_CHOICE) return 0;
    return node->choice_count;
}

const char *building_interact_choice_label(const BuildingInteraction *bi,
                                            unsigned idx) {
    if (!bi || !bi->active) return NULL;
    const DialogueNode *node = dialogue_state_current(&bi->dialogue);
    if (!node || node->type != DIALOGUE_NODE_CHOICE || idx >= node->choice_count)
        return NULL;
    return node->choices[idx].label;
}
