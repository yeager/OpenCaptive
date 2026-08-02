#include "custom_features.h"
#include "game_state.h"
#include <string.h>

void automap_init(Automap *am) {
    memset(am->visited, 0, sizeof(am->visited));
}

void automap_mark(Automap *am, int level, int x, int y) {
    if (level < 0 || level >= MAX_LEVELS) return;
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT) return;
    am->visited[level * MAP_WIDTH * MAP_HEIGHT + y * MAP_WIDTH + x] = true;
}

bool automap_is_visited(const Automap *am, int level, int x, int y) {
    if (level < 0 || level >= MAX_LEVELS) return false;
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT) return false;
    return am->visited[level * MAP_WIDTH * MAP_HEIGHT + y * MAP_WIDTH + x];
}
