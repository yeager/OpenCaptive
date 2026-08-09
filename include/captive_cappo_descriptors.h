#ifndef CAPTIVE_CAPPO_DESCRIPTORS_H
#define CAPTIVE_CAPPO_DESCRIPTORS_H

#include "captive_dos_descriptor.h"

/*
 * Complete descriptor table used by the relocated DOS CAPPO runtime.
 *
 * The records are copied verbatim from the real CAPPO.EXE v1.06 at
 * unpacked file offset 0x216b0. The first three records at 0x21698 are
 * the file image's preceding data and are not at runtime DS:00c0.
 * DOSBox-X MEMDUMP.BIN confirms this alignment: runtime DS:00c0 is
 * executable record 3. No records below are generated or approximated.
 */
#define CAPTIVE_CAPPO_DESCRIPTOR_COUNT 959
#define CAPTIVE_CAPPO_DESCRIPTOR_FILE_RECORD_OFFSET 3

extern const CaptiveDosDescriptor captive_cappo_descriptors[
    CAPTIVE_CAPPO_DESCRIPTOR_COUNT];

#endif
