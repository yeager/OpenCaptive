#ifndef CAPTIVE_EMULATOR_H
#define CAPTIVE_EMULATOR_H

#include <stdbool.h>

/* Launch the supplied original CAPTIVE.BAT chain in the isolated DOSBox-X
 * profile. This path never copies, patches or generates game data. */
bool captive_emulator_launch(const char *data_path);

#endif
