#ifndef POOLRAD_RESIDENT_00BA_SUFFIX_FIRST_GLYPH_H
#define POOLRAD_RESIDENT_00BA_SUFFIX_FIRST_GLYPH_H

#include "resident_0709_064f_first_ega.h"

/* The nonzero display suffix's first printer glyph, starting at the reached
 * 0709:064F stop. Compose the existing glyph-frame and first EGA-change
 * transitions, stopping at 0709:0415. This does not model the rest of the
 * printer, its second glyph, or hardware EGA plane state. */
int pr_00ba_suffix_first_glyph(uint8_t *ram, size_t ram_size,
                               pr_1816_caller_cpu *cpu,
                               pr_064f_ega_report *report);

#endif
