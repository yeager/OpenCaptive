#include "captive_dos_blitter.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    const size_t memory_size = 1024 * 1024;
    uint8_t *memory = calloc(memory_size, 1);
    uint8_t canvas[CAPTIVE_DOS_CANVAS_PIXELS];
    assert(memory);
    memset(canvas, 0x1e, sizeof(canvas));

    /* This group unpacks to indexes 1, 2, 3, 4, 5, 6, 7, 8. */
    const uint16_t source_segment = 0x5000;
    size_t source = ((size_t)source_segment << 4) + 0x20;
    memory[source + 0] = 0x61;
    memory[source + 1] = 0x02;
    memory[source + 2] = 0x82;
    memory[source + 3] = 0x35;
    memory[source + 4] = 0x47;

    CaptiveDosDescriptor descriptor = {
        .source_offset = 0x20, .destination_offset = 200 * 3,
        .width_bytes = 1, .height = 1, .flags = 0,
        .source_segment = source_segment,
    };
    assert(captive_dos_blit_descriptor(memory, memory_size, &descriptor,
                                       canvas, sizeof(canvas)));
    const uint8_t expected[] = {1, 2, 3, 4, 21, 6, 7, 8};
    for (size_t i = 0; i < 8; ++i)
        assert(canvas[3 * CAPTIVE_DOS_CANVAS_WIDTH + i] == expected[i]);

    memset(canvas, 0x1e, sizeof(canvas));
    memory[source + 0] = 0;
    descriptor.flags = 0x05; /* Mirror and retain destination at source index 0. */
    assert(captive_dos_blit_descriptor(memory, memory_size, &descriptor,
                                       canvas, sizeof(canvas)));
    assert(canvas[3 * CAPTIVE_DOS_CANVAS_WIDTH + 7] == 0x1e);
    assert(canvas[3 * CAPTIVE_DOS_CANVAS_WIDTH + 6] == 2);
    assert(canvas[3 * CAPTIVE_DOS_CANVAS_WIDTH + 0] == 8);

    descriptor.destination_offset = 1;
    assert(!captive_dos_blit_descriptor(memory, memory_size, &descriptor,
                                        canvas, sizeof(canvas)));
    free(memory);
    return 0;
}
