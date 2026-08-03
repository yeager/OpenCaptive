#include "liberation_npc_dialogue.h"
#include "arcd_decoder.h"
#include "i18n.h"
#include <stdlib.h>
#include <string.h>

bool npc_dialogue_data_load(NPCDialogueData *ndd,
                            const uint8_t *dte_compressed, size_t comp_size) {
    if (!ndd || !dte_compressed || !comp_size) return false;
    memset(ndd, 0, sizeof(*ndd));

    size_t dsz = arcd_decompressed_size(dte_compressed, comp_size);
    if (!dsz) return false;

    ndd->dte_data = malloc(dsz);
    if (!ndd->dte_data) return false;

    int r = arcd_decode(dte_compressed, comp_size, ndd->dte_data, dsz);
    if (r <= 0) {
        free(ndd->dte_data);
        ndd->dte_data = NULL;
        return false;
    }
    ndd->dte_size = (size_t)r;

    if (!lib_text_table_parse(&ndd->dte_table, ndd->dte_data, ndd->dte_size)) {
        free(ndd->dte_data);
        ndd->dte_data = NULL;
        return false;
    }

    ndd->loaded = true;
    return true;
}

void npc_dialogue_data_free(NPCDialogueData *ndd) {
    if (!ndd) return;
    free(ndd->dte_data);
    memset(ndd, 0, sizeof(*ndd));
}

bool npc_dialogue_generate(const NPCDialogueData *ndd,
                           unsigned building_type_id,
                           NPCState npc_state,
                           uint32_t seed,
                           DialogueTree *tree) {
    if (!ndd || !ndd->loaded || !tree) return false;

    dialogue_tree_init(tree);

    const LibTextSection *sec = lib_text_table_find(&ndd->dte_table, building_type_id);
    if (!sec) return false;

    LibTextState tstate;
    lib_text_state_init(&tstate, seed);

    char description[LIB_TEXT_MAX_OUTPUT];
    if (!lib_text_expand(&ndd->dte_table, sec, &tstate, (int)npc_state,
                         description, sizeof(description)))
        return false;

    unsigned exit_node = dialogue_tree_add_exit(tree, _("You leave."));

    if (npc_state == NPC_STATE_NORMAL) {
        unsigned trade_node = dialogue_tree_add_text(tree, "NPC",
            _("I don't have anything to trade right now. Try the shops."), exit_node);
        unsigned info_node = dialogue_tree_add_text(tree, "NPC",
            _("Check the library or records office if you need information. "
              "The police might know about threats in the area."), exit_node);
        unsigned greet = dialogue_tree_add_choice(tree, "NPC", description);
        dialogue_tree_add_option(tree, greet, _("Trade"), trade_node);
        dialogue_tree_add_option(tree, greet, _("Ask around"), info_node);
        dialogue_tree_add_option(tree, greet, _("Leave"), exit_node);
    } else {
        dialogue_tree_add_text(tree, NULL, description, exit_node);
    }

    return true;
}
