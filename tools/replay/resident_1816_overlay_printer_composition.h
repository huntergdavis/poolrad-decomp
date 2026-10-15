#ifndef POOLRAD_RESIDENT_1816_OVERLAY_PRINTER_COMPOSITION_H
#define POOLRAD_RESIDENT_1816_OVERLAY_PRINTER_COMPOSITION_H

#include "resident_1816_caller_routes.h"

typedef struct {
    unsigned event_count;
    pr_1816_caller_event order[6];
    uint16_t pushed_words[5];
    pr_1816_caller_frame frame;
} pr_1816_printer_composition_report;

/* Execute one verified overlay prefix, enter RES_00BA:1816, and stop at
 * the first instruction of RES_0709:0626. No printer body is executed. */
int pr_1816_overlay_to_first_printer(
    unsigned site, uint8_t *ram, size_t ram_size, pr_1816_caller_cpu *cpu,
    pr_1816_printer_composition_report *report);

#endif
