#include "liberation_dialogue.h"
#include <string.h>
#include <stdio.h>

void dialogue_tree_init(DialogueTree *tree) {
    memset(tree, 0, sizeof(*tree));
}

static unsigned add_node(DialogueTree *tree) {
    if (tree->node_count >= DIALOGUE_MAX_NODES) return 0;
    unsigned idx = tree->node_count++;
    memset(&tree->nodes[idx], 0, sizeof(DialogueNode));
    return idx;
}

unsigned dialogue_tree_add_text(DialogueTree *tree, const char *speaker,
                                 const char *text, unsigned next) {
    unsigned idx = add_node(tree);
    DialogueNode *n = &tree->nodes[idx];
    n->type = DIALOGUE_NODE_TEXT;
    if (speaker) snprintf(n->speaker, sizeof(n->speaker), "%s", speaker);
    if (text) snprintf(n->text, sizeof(n->text), "%s", text);
    n->next_node = (uint16_t)next;
    return idx;
}

unsigned dialogue_tree_add_choice(DialogueTree *tree, const char *speaker,
                                   const char *prompt) {
    unsigned idx = add_node(tree);
    DialogueNode *n = &tree->nodes[idx];
    n->type = DIALOGUE_NODE_CHOICE;
    if (speaker) snprintf(n->speaker, sizeof(n->speaker), "%s", speaker);
    if (prompt) snprintf(n->text, sizeof(n->text), "%s", prompt);
    return idx;
}

bool dialogue_tree_add_option(DialogueTree *tree, unsigned node_idx,
                               const char *label, unsigned target) {
    if (node_idx >= tree->node_count) return false;
    DialogueNode *n = &tree->nodes[node_idx];
    if (n->type != DIALOGUE_NODE_CHOICE) return false;
    if (n->choice_count >= DIALOGUE_MAX_CHOICES) return false;
    DialogueChoice *c = &n->choices[n->choice_count++];
    if (label) snprintf(c->label, sizeof(c->label), "%s", label);
    c->target_node = (uint16_t)target;
    return true;
}

unsigned dialogue_tree_add_exit(DialogueTree *tree, const char *text) {
    unsigned idx = add_node(tree);
    DialogueNode *n = &tree->nodes[idx];
    n->type = DIALOGUE_NODE_EXIT;
    if (text) snprintf(n->text, sizeof(n->text), "%s", text);
    return idx;
}

void dialogue_state_init(DialogueState *state, const DialogueTree *tree) {
    memset(state, 0, sizeof(*state));
    state->tree = tree;
}

void dialogue_state_start(DialogueState *state) {
    state->active = true;
    state->current_node = 0;
}

const DialogueNode *dialogue_state_current(const DialogueState *state) {
    if (!state->active || !state->tree) return NULL;
    if (state->current_node >= state->tree->node_count) return NULL;
    return &state->tree->nodes[state->current_node];
}

bool dialogue_state_advance(DialogueState *state) {
    const DialogueNode *node = dialogue_state_current(state);
    if (!node) return false;

    if (node->type == DIALOGUE_NODE_EXIT) {
        state->active = false;
        return false;
    }

    if (node->type == DIALOGUE_NODE_CHOICE) return false;

    if (node->set_var >= 0 && node->set_var < 32)
        state->variables[node->set_var] = node->set_val;

    state->current_node = node->next_node;

    const DialogueNode *next = dialogue_state_current(state);
    if (next && next->type == DIALOGUE_NODE_EXIT) {
        state->active = false;
        return false;
    }

    return state->active;
}

bool dialogue_state_choose(DialogueState *state, unsigned choice_idx) {
    const DialogueNode *node = dialogue_state_current(state);
    if (!node || node->type != DIALOGUE_NODE_CHOICE) return false;
    if (choice_idx >= node->choice_count) return false;

    state->current_node = node->choices[choice_idx].target_node;

    const DialogueNode *next = dialogue_state_current(state);
    if (next && next->type == DIALOGUE_NODE_EXIT) {
        state->active = false;
        return false;
    }

    return true;
}

bool dialogue_state_is_active(const DialogueState *state) {
    return state && state->active;
}
