#ifndef LIBERATION_SERVICE_MC_H
#define LIBERATION_SERVICE_MC_H

#include <stdint.h>

/* Recovered from the Liberation game binary (CD32, CODE hunk) by reverse
 * engineering: the interpreter variable `mc` — which the CITY_TEXT (CTE)
 * conversation script branches on to pick building-service dialogue — is the
 * current person/NPC's profession field, and for a GENERIC building service it
 * is derived from the building's category byte via the function at 0xa738:
 *
 *   mc = 0xa738(category_byte) + 3
 *
 * where the interaction entry (FUN_0xef8) takes `category_byte` from byte 5 of
 * the interaction record.  0xa738's main path (verified by both radare2 and the
 * Ghidra decompiler) decodes the category byte as below.  Named service NPCs
 * (bank = mc 13, repair = mc 16/18) do NOT come through this path — their
 * profession is loaded from the people list — so this function only yields the
 * generic-service range (mc 0-9). */

/* Compute mc for a generic building service from its category byte.
 * Returns a value in the generic-service range (0-9 for real category bytes). */
int liberation_service_mc_from_category(uint8_t category_byte);

#endif
