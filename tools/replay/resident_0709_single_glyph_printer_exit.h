#ifndef POOLRAD_RESIDENT_0709_SINGLE_GLYPH_PRINTER_EXIT_H
#define POOLRAD_RESIDENT_0709_SINGLE_GLYPH_PRINTER_EXIT_H

#include "resident_1816_caller_routes.h"

/* Execute printer 0709:0694 through RETF 0A to RES_00BA:1858 when its
 * copied Pascal source has exactly one glyph and index one is complete.
 * Return 2 for a different loop state; RAM is flat 20-bit real-mode RAM. */
int pr_0694_single_glyph_printer_exit(uint8_t *ram, size_t ram_size,
                                       pr_1816_caller_cpu *cpu);

#endif
