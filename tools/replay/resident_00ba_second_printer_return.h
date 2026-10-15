#ifndef POOLRAD_RESIDENT_00BA_SECOND_PRINTER_RETURN_H
#define POOLRAD_RESIDENT_00BA_SECOND_PRINTER_RETURN_H

#include "resident_1816_caller_routes.h"

typedef struct {
    uint16_t destination_offset, destination_segment;
    uint16_t color, row, column;
} pr_1887_second_printer_frame;

/* Execute the single 00BA:1887 far call and stop at 0709:0626, before
 * printer effects. The report records the five retained argument words. */
int pr_1887_second_printer_entry(uint8_t *ram, size_t ram_size,
                                 pr_1816_caller_cpu *cpu,
                                 pr_1887_second_printer_frame *frame);

/* Resume only after the external printer has returned to 00BA:188C.
 * Execute MOV SP,BP; POP BP; RETF 0A to the overlay caller. */
int pr_188c_display_return(uint8_t *ram, size_t ram_size,
                            pr_1816_caller_cpu *cpu);

#endif
