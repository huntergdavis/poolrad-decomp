#ifndef POOLRAD_RESIDENT_0709_064F_FIRST_EGA_H
#define POOLRAD_RESIDENT_0709_064F_FIRST_EGA_H

#include "resident_1816_caller_routes.h"

typedef struct {
    uint8_t glyph_index;
    uint8_t color;
    uint8_t plane;
    uint8_t stores;
    uint16_t page_offset;
    uint16_t page_segment;
} pr_064f_ega_report;

/* The bounded one-character route starting at RES_0709:064F. */
int pr_064f_first_glyph_entry(uint8_t *ram, size_t ram_size,
                              pr_1816_caller_cpu *cpu);

/* Execute the upcase and mode-one plane route to the first value-changing
 * page store, stopping at RES_0709:0415. Return 2 for another route. */
int pr_064f_first_ega_change(uint8_t *ram, size_t ram_size,
                              pr_1816_caller_cpu *cpu,
                              pr_064f_ega_report *report);

#endif
