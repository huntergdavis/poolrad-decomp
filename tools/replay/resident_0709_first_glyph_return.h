#ifndef POOLRAD_RESIDENT_0709_FIRST_GLYPH_RETURN_H
#define POOLRAD_RESIDENT_0709_FIRST_GLYPH_RETURN_H

#include "resident_1816_caller_routes.h"

/* Start immediately after the first changed EGA page store at 0709:0412.
 * Complete the reached mode-one glyph and RETF 0C to printer 0709:0694.
 * Return 2 for states outside this bounded first-store route. */
int pr_0415_first_glyph_return(uint8_t *ram, size_t ram_size,
                                pr_1816_caller_cpu *cpu);

#endif
