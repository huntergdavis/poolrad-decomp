#ifndef POOLRAD_RESIDENT_0709_NEXT_GLYPH_CALL_H
#define POOLRAD_RESIDENT_0709_NEXT_GLYPH_CALL_H

#include "resident_1816_caller_routes.h"

/* From the first RES_0709:01DF return at 0709:0694, execute the printer's
 * loop edge and next argument setup. Stop at the second 0709:01DF entry.
 * Return 2 when there is no next character or the loop state is outside
 * this bounded route. RAM is a flat 20-bit real-mode image. */
int pr_0694_next_glyph_entry(uint8_t *ram, size_t ram_size,
                             pr_1816_caller_cpu *cpu);

#endif
