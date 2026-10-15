#ifndef POOLRAD_RESIDENT_00BA_1858_CONTINUATION_H
#define POOLRAD_RESIDENT_00BA_1858_CONTINUATION_H

#include "resident_1816_caller_routes.h"

/* Continue RES_00BA:1816 after its first printer returns at 00BA:1858.
 * Flag low byte zero: execute RETF 0A to the overlay caller.
 * Flag low byte nonzero: stop at SYSTEM 0AF8:0B0B copy entry, before its
 * effects. RAM is a flat 20-bit real-mode image. */
int pr_1858_display_continuation(uint8_t *ram, size_t ram_size,
                                  pr_1816_caller_cpu *cpu);

#endif
