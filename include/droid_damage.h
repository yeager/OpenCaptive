#ifndef DROID_DAMAGE_H
#define DROID_DAMAGE_H

#include "game_state.h"

/* Apply damage that is not a creature weapon hit (for example a pit or an
 * industrial hazard).  Shared droid damage still passes through an equipped
 * shield before reducing HP. */
void droid_apply_environmental_damage(Droid *droid, int damage);

#endif
