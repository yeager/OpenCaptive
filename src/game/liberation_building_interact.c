#include "liberation_building_interact.h"
#include "i18n.h"
#include <string.h>
#include <stdio.h>
#include <limits.h>

void building_interact_init(BuildingInteraction *bi) {
    if (!bi) return;
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
        case INTERACT_SHOP:       return _("Shopkeeper");
        case INTERACT_BAR:        return _("Bartender");
        case INTERACT_BUSINESS:   return _("Receptionist");
        case INTERACT_LIBRARY:    return _("Librarian");
        case INTERACT_POLICE:     return _("Officer");
        case INTERACT_RECORDS:    return _("Clerk");
        case INTERACT_RESIDENCE:  return _("Resident");
        case INTERACT_INDUSTRIAL: return _("Foreman");
        case INTERACT_SPECIAL:    return _("Agent");
        default:                  return _("NPC");
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
            char lib_info[DIALOGUE_MAX_TEXT];
            snprintf(lib_info, sizeof(lib_info),
                _("Records indicate %d buildings in this city. "
                  "The target is in a special-class building. "
                  "Check the industrial and commercial districts."),
                bi->building_index + 5);
            unsigned info = dialogue_tree_add_text(&tree, npc, lib_info, exit_node);
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
            unsigned fine_node = dialogue_tree_add_text(&tree, npc,
                _("You have been fined 100 gold for disturbing the peace."), exit_node);
            bi->fine_node = (int)fine_node;
            unsigned choice = dialogue_tree_add_choice(&tree, npc, greeting);
            dialogue_tree_add_option(&tree, choice, _("Ask for information"), report);
            if (bi->bar_fight && bi->player_gold && *bi->player_gold >= 100) {
                dialogue_tree_add_option(&tree, choice, _("Pay fine"), fine_node);
            }
            dialogue_tree_add_option(&tree, choice, _("Leave"), exit_node);
            break;
        }
        case INTERACT_RECORDS: {
            snprintf(greeting, sizeof(greeting),
                     "%s", _("City Records Office. How can we help?"));
            char rec_info[DIALOGUE_MAX_TEXT];
            snprintf(rec_info, sizeof(rec_info),
                _("Building registry entry %d: %s. "
                  "Cross-reference with police reports for suspect locations."),
                bi->building_index, building_name);
            unsigned lookup = dialogue_tree_add_text(&tree, npc, rec_info, exit_node);
            unsigned choice = dialogue_tree_add_choice(&tree, npc, greeting);
            dialogue_tree_add_option(&tree, choice, _("Look up records"), lookup);
            dialogue_tree_add_option(&tree, choice, _("Leave"), exit_node);
            break;
        }
        case INTERACT_RESIDENCE: {
            snprintf(greeting, sizeof(greeting),
                     "%s", _("This is a private residence. What do you want?"));
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
            unsigned hazard_node = dialogue_tree_add_text(&tree, npc,
                _("WARNING: Machinery malfunction! Electrical discharge hits your droids!"), exit_node);
            unsigned explore = dialogue_tree_add_text(&tree, npc,
                _("The machinery hums loudly. There might be useful equipment "
                  "stored in the back, but the foreman keeps it locked."), exit_node);
            unsigned choice = dialogue_tree_add_choice(&tree, npc, greeting);
            if (bi->building_index % 3 == 0) {
                dialogue_tree_add_option(&tree, choice, _("Look around"), hazard_node);
                bi->industrial_hazard = true;
            } else {
                dialogue_tree_add_option(&tree, choice, _("Look around"), explore);
            }
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
                             int *player_gold) {
    if (bi) bi->active = false;
    if (!bi || !grid || !buildings || buildings->total_buildings <= 0 ||
        buildings->total_buildings > CITYGEN_MAX_BUILDINGS ||
        cell_x < 0 || cell_x >= CITYGRID_WIDTH ||
        cell_y < 0 || cell_y >= CITYGRID_HEIGHT) return false;

    int offset = cell_y * CITYGRID_WIDTH + cell_x;
    if (offset < 0 || offset >= CITYGRID_CELLS) return false;

    uint8_t raw_bid = grid->plane2[offset];
    if (raw_bid == 0 || raw_bid == 0xFF) return false;
    uint8_t bid = raw_bid & 0x7F;
    if (bid == 0) return false;

    int bg_idx = (bid - 1) % buildings->total_buildings;
    if (bg_idx < 0 || bg_idx >= buildings->total_buildings ||
        bg_idx >= CITYGEN_MAX_BUILDINGS) return false;

    const CityBuilding *bld = &buildings->buildings[bg_idx];
    InteractionType interaction = type_from_building((BuildingType)bld->type);
    if (interaction == INTERACT_NONE) return false;
    bi->purchased_count = 0;
    bi->mission_complete = false;
    bi->bar_fight = false;
    bi->fine_refused = false;
    bi->fine_paid = false;
    bi->industrial_hazard = false;
    bi->shop_menu_active = false;
    bi->special_investigated = false;
    bi->fine_node = -1;
    bi->reputation_priced = false;
    bi->type = interaction;
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

void building_interact_set_bar_fight(BuildingInteraction *bi, bool pending) {
    if (!bi || !bi->active || bi->type != INTERACT_POLICE) return;
    bi->bar_fight = pending;
    /* The police choice list depends on whether a fine is pending.  Rebuild
     * it before the first frame is shown; changing the flag alone is too late
     * because building_interact_enter already created the old tree. */
    build_generic_dialogue(bi, bi->shop.shop_name);
}

void building_interact_choose(BuildingInteraction *bi, unsigned choice) {
    if (!bi || !bi->active) return;
    const DialogueNode *node = dialogue_state_current(&bi->dialogue);
    bool valid_special_investigate =
        bi->type == INTERACT_SPECIAL && node &&
        node->type == DIALOGUE_NODE_CHOICE &&
        node->choice_count <= DIALOGUE_MAX_CHOICES &&
        choice < node->choice_count && choice == 0;
    if (valid_special_investigate)
        bi->special_investigated = true;
    if (bi->type == INTERACT_POLICE && bi->bar_fight && node &&
        node->type == DIALOGUE_NODE_CHOICE &&
        node->choice_count <= DIALOGUE_MAX_CHOICES &&
        node->choice_count > 0U && choice < node->choice_count &&
        choice == node->choice_count - 1U)
        bi->fine_refused = true;
    bi->shop_menu_active = false;
    if (node && node->type == DIALOGUE_NODE_CHOICE &&
        node->choice_count <= DIALOGUE_MAX_CHOICES && choice < node->choice_count &&
        (bi->type == INTERACT_SHOP || bi->type == INTERACT_BAR) && choice == 0) {
        bi->shop_menu_active = true;
    }
    if (node && node->type == DIALOGUE_NODE_CHOICE &&
        node->choice_count <= DIALOGUE_MAX_CHOICES && choice < node->choice_count &&
        bi->type == INTERACT_POLICE && bi->fine_node >= 0 &&
        node->choices[choice].target_node == (unsigned)bi->fine_node &&
        bi->bar_fight && bi->player_gold && *bi->player_gold >= 100) {
        *bi->player_gold -= 100;
        bi->bar_fight = false;
        bi->fine_paid = true;
    }
    dialogue_state_choose(&bi->dialogue, choice);
    if (!dialogue_state_is_active(&bi->dialogue))
        bi->active = false;
}

void building_interact_advance(BuildingInteraction *bi) {
    if (!bi || !bi->active) return;
    if (!dialogue_state_advance(&bi->dialogue)) {
        if (bi->type == INTERACT_SPECIAL && bi->special_investigated)
            bi->mission_complete = true;
        bi->active = false;
    }
}

bool building_interact_buy(BuildingInteraction *bi, unsigned item_idx) {
    if (!bi || !bi->active || !bi->player_gold) return false;
    if (bi->type != INTERACT_SHOP && bi->type != INTERACT_BAR) return false;
    if (bi->type == INTERACT_SHOP && bi->reputation <= -50) return false;
    if (bi->purchased_count < 0 || bi->purchased_count >= 20) return false;
    if (bi->shop.item_count > LIB_SHOP_MAX_ITEMS ||
        item_idx >= bi->shop.item_count || item_idx >= LIB_SHOP_MAX_ITEMS) return false;
    const LibShopItem *item = &bi->shop.items[item_idx];
    if (*bi->player_gold < 0) return false;
    uint32_t gold = (uint32_t)*bi->player_gold;
    if (!lib_shop_buy_item(&bi->shop, item_idx, &gold)) return false;
    *bi->player_gold = gold > (uint32_t)INT_MAX ? INT_MAX : (int)gold;
    if (bi->purchased_count < 20) {
        snprintf(bi->purchased[bi->purchased_count].name,
                 sizeof(bi->purchased[0].name), "%s", item->name);
        bi->purchased[bi->purchased_count].item_type = item->item_type;
        bi->purchased_count++;
    }
    if (bi->type == INTERACT_BAR) {
        /* Liberation: a drink purchase has a 25% bar-fight chance.
         * Keep the roll in the shop state so the result is deterministic
         * for replays and save-independent within one building visit. */
        bi->shop.prng_seed = (uint16_t)(bi->shop.prng_seed * 0x5E5U + 0x29U);
        if ((bi->shop.prng_seed & 3U) == 0U)
            bi->bar_fight = true;
    }
    return true;
}

void building_interact_leave(BuildingInteraction *bi) {
    if (bi) bi->active = false;
}

void building_interact_set_reputation(BuildingInteraction *bi, int reputation) {
    if (!bi) return;
    if (reputation < -100) reputation = -100;
    if (reputation > 100) reputation = 100;
    bi->reputation = reputation;
    if (bi->type != INTERACT_SHOP || bi->reputation_priced || reputation < 50) return;
    for (unsigned i = 0; i < bi->shop.item_count && i < LIB_SHOP_MAX_ITEMS; ++i) {
        bi->shop.items[i].price = (uint16_t)((bi->shop.items[i].price * 3U) / 4U);
    }
    bi->reputation_priced = true;
}

const char *building_interact_text(const BuildingInteraction *bi) {
    if (!bi || !bi->active) return NULL;
    const DialogueNode *node = dialogue_state_current(&bi->dialogue);
    return node ? node->text : NULL;
}

unsigned building_interact_choice_count(const BuildingInteraction *bi) {
    if (!bi || !bi->active) return 0;
    const DialogueNode *node = dialogue_state_current(&bi->dialogue);
    if (!node || node->type != DIALOGUE_NODE_CHOICE ||
        node->choice_count > DIALOGUE_MAX_CHOICES) return 0;
    return node->choice_count;
}

const char *building_interact_choice_label(const BuildingInteraction *bi,
                                            unsigned idx) {
    if (!bi || !bi->active) return NULL;
    const DialogueNode *node = dialogue_state_current(&bi->dialogue);
    if (!node || node->type != DIALOGUE_NODE_CHOICE ||
        node->choice_count > DIALOGUE_MAX_CHOICES || idx >= node->choice_count)
        return NULL;
    return node->choices[idx].label;
}
