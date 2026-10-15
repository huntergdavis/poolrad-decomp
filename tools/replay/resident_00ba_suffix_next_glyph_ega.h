#ifndef POOLRAD_RESIDENT_00BA_SUFFIX_NEXT_GLYPH_EGA_H
#define POOLRAD_RESIDENT_00BA_SUFFIX_NEXT_GLYPH_EGA_H

#include "resident_0709_064f_first_ega.h"

/* Enter the copied suffix's second glyph at RES_0709:01DF. Stop just
 * after its first value-changing A000 store, at RES_0709:0415. */
int pr_00ba_suffix_next_glyph_ega(uint8_t *ram, size_t ram_size,
                                    pr_1816_caller_cpu *cpu,
                                    pr_064f_ega_report *report);

#endif
