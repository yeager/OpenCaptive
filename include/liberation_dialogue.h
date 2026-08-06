#ifndef LIBERATION_DIALOGUE_H
#define LIBERATION_DIALOGUE_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <limits.h>

#define DIALOGUE_MAX_CHOICES 6
#define DIALOGUE_MAX_TEXT 512
#define DIALOGUE_MAX_NODES 256
#define DIALOGUE_INVALID_NODE UINT_MAX

typedef enum {
    DIALOGUE_NODE_TEXT,
    DIALOGUE_NODE_CHOICE,
    DIALOGUE_NODE_TRADE,
    DIALOGUE_NODE_INFO,
    DIALOGUE_NODE_EXIT,
} DialogueNodeType;

typedef struct {
    char label[64];
    uint16_t target_node;
} DialogueChoice;

typedef struct {
    DialogueNodeType type;
    char text[DIALOGUE_MAX_TEXT];
    char speaker[32];
    uint16_t next_node;
    uint8_t  choice_count;
    DialogueChoice choices[DIALOGUE_MAX_CHOICES];
    uint16_t flags;
    int16_t  condition_var;
    int16_t  condition_val;
    int16_t  set_var;
    int16_t  set_val;
} DialogueNode;

typedef struct {
    DialogueNode nodes[DIALOGUE_MAX_NODES];
    unsigned node_count;
} DialogueTree;

typedef struct {
    const DialogueTree *tree;
    unsigned current_node;
    bool active;
    int16_t variables[32];
} DialogueState;

void dialogue_tree_init(DialogueTree *tree);
unsigned dialogue_tree_add_text(DialogueTree *tree, const char *speaker,
                                 const char *text, unsigned next);
unsigned dialogue_tree_add_choice(DialogueTree *tree, const char *speaker,
                                   const char *prompt);
bool dialogue_tree_add_option(DialogueTree *tree, unsigned node_idx,
                               const char *label, unsigned target);
unsigned dialogue_tree_add_exit(DialogueTree *tree, const char *text);

void dialogue_state_init(DialogueState *state, const DialogueTree *tree);
void dialogue_state_start(DialogueState *state);
const DialogueNode *dialogue_state_current(const DialogueState *state);
bool dialogue_state_advance(DialogueState *state);
bool dialogue_state_choose(DialogueState *state, unsigned choice_idx);
bool dialogue_state_is_active(const DialogueState *state);

#endif
