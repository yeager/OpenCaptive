#ifndef CTV_DECODER_H
#define CTV_DECODER_H

#include <stdint.h>
#include <stdbool.h>

/* Creative Voice File (CTV/VOC) decoder.
 * Captive DOS ships SB15.CTV, SB20.CTV and SBPRO.CTV containing
 * digitized sound effects for Sound Blaster playback. */

typedef struct {
    int8_t  *samples;
    uint32_t length;
    uint32_t sample_rate;
} CtvSample;

typedef struct {
    CtvSample *entries;
    int         count;
    int         capacity;
} CtvFile;

bool ctv_decode(CtvFile *out, const uint8_t *data, uint32_t size);
void ctv_free(CtvFile *ctv);

#endif
