#include "liberation_npc_dialogue.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void test_load_unload(void) {
    NPCDialogueData ndd;
    assert(!npc_dialogue_data_load(&ndd, NULL, 0));
    assert(!ndd.loaded);

    uint8_t bad[] = {0, 0, 0, 0};
    assert(!npc_dialogue_data_load(&ndd, bad, sizeof(bad)));
    printf("PASS: load_unload\n");
}

static void test_with_game_data(void) {
    const char *basedir = getenv("OPENCAPTIVE_TEST_DATA");
    if (!basedir) basedir = getenv("HOME");
    if (!basedir) return;

    char path[512];
    snprintf(path, sizeof(path), "%s/.opencaptive/liberation/disk3/DTE.txt", basedir);
    FILE *f = fopen(path, "rb");
    if (!f) { printf("SKIP: DTE.txt not available\n"); return; }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *src = malloc(sz);
    fread(src, 1, sz, f);
    fclose(f);

    NPCDialogueData ndd;
    assert(npc_dialogue_data_load(&ndd, src, sz));
    assert(ndd.loaded);
    free(src);

    DialogueTree tree;
    assert(npc_dialogue_generate(&ndd, 1, NPC_STATE_NORMAL, 42, &tree));
    assert(tree.node_count >= 2);

    const DialogueNode *greet = &tree.nodes[tree.node_count - 1];
    assert(greet->type == DIALOGUE_NODE_CHOICE);
    assert(strlen(greet->text) > 10);
    printf("  Section 1 normal: \"%.*s...\"\n", 60, greet->text);

    DialogueTree tree2;
    assert(npc_dialogue_generate(&ndd, 1, NPC_STATE_DEAD, 42, &tree2));
    assert(tree2.node_count >= 2);
    const DialogueNode *dead = &tree2.nodes[tree2.node_count - 1];
    assert(dead->type == DIALOGUE_NODE_TEXT);
    assert(strlen(dead->text) > 10);
    printf("  Section 1 dead: \"%.*s...\"\n", 60, dead->text);

    assert(npc_dialogue_generate(&ndd, 2, NPC_STATE_NORMAL, 100, &tree));
    assert(npc_dialogue_generate(&ndd, 3, NPC_STATE_NORMAL, 200, &tree));

    assert(!npc_dialogue_generate(&ndd, 999, NPC_STATE_NORMAL, 0, &tree));

    npc_dialogue_data_free(&ndd);
    assert(!ndd.loaded);
    printf("PASS: game_data_npc_dialogue\n");
}

static void test_deterministic(void) {
    const char *basedir = getenv("OPENCAPTIVE_TEST_DATA");
    if (!basedir) basedir = getenv("HOME");
    if (!basedir) return;

    char path[512];
    snprintf(path, sizeof(path), "%s/.opencaptive/liberation/disk3/DTE.txt", basedir);
    FILE *f = fopen(path, "rb");
    if (!f) { printf("SKIP: deterministic (no data)\n"); return; }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *src = malloc(sz);
    fread(src, 1, sz, f);
    fclose(f);

    NPCDialogueData ndd;
    assert(npc_dialogue_data_load(&ndd, src, sz));
    free(src);

    DialogueTree t1, t2;
    assert(npc_dialogue_generate(&ndd, 1, NPC_STATE_NORMAL, 12345, &t1));
    assert(npc_dialogue_generate(&ndd, 1, NPC_STATE_NORMAL, 12345, &t2));
    assert(strcmp(t1.nodes[t1.node_count - 1].text,
                 t2.nodes[t2.node_count - 1].text) == 0);

    npc_dialogue_data_free(&ndd);
    printf("PASS: deterministic\n");
}

int main(void) {
    test_load_unload();
    test_with_game_data();
    test_deterministic();
    printf("PASS: all liberation_npc_dialogue tests\n");
    return 0;
}
