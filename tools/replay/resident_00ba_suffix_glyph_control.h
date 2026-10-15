#ifndef POOLRAD_RESIDENT_00BA_SUFFIX_GLYPH_CONTROL_H
#define POOLRAD_RESIDENT_00BA_SUFFIX_GLYPH_CONTROL_H

#include "resident_1816_caller_routes.h"

typedef enum {
    PR_00BA_SUFFIX_NEXT_GLYPH = 1,
    PR_00BA_SUFFIX_PRINTER_RETURN = 2
} pr_00ba_suffix_control_stop;

typedef struct {
    pr_00ba_suffix_control_stop stop;
    uint8_t glyph_index_before, glyph_index_after, total;
    uint8_t column_after;
} pr_00ba_suffix_control_report;

/* At the first glyph's return to RES_0709:0694, execute the printer loop
 * edge. Stop at the next 0709:01DF drawer entry or the printer's far
 * return at 00BA:188C. The glyph drawer and later display are external. */
int pr_00ba_suffix_glyph_control(uint8_t *ram, size_t ram_size,
                                  pr_1816_caller_cpu *cpu,
                                  pr_00ba_suffix_control_report *report);

#endif
