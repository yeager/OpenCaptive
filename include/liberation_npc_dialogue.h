#ifndef LIBERATION_NPC_DIALOGUE_H
#define LIBERATION_NPC_DIALOGUE_H

#include "liberation_dialogue.h"
#include "liberation_text_table.h"
#include "liberation_citygen.h"
#include "liberation_citygen_grid.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    NPC_STATE_NORMAL = 0,
    NPC_STATE_DEAD = 1,
    NPC_STATE_ABSENT = 2,
    NPC_STATE_ATTACK = 3,
    NPC_STATE_FOLLOW = 4,
    NPC_STATE_DEAD_ATTACK = 5,
    NPC_STATE_DEAD_FOLLOW = 6,
} NPCState;

typedef struct {
    LibTextTable dte_table;
    uint8_t *dte_data;
    size_t dte_size;
    bool loaded;
} NPCDialogueData;

bool npc_dialogue_data_load(NPCDialogueData *ndd,
                            const uint8_t *dte_compressed, size_t comp_size);
void npc_dialogue_data_free(NPCDialogueData *ndd);

bool npc_dialogue_generate(const NPCDialogueData *ndd,
                           unsigned building_type_id,
                           NPCState npc_state,
                           uint32_t seed,
                           DialogueTree *tree);

#endif
