#ifndef POOLRAD_RESIDENT_0709_PRINTER_COPY_GATE_H
#define POOLRAD_RESIDENT_0709_PRINTER_COPY_GATE_H

#include "resident_1816_caller_routes.h"

/* Execute 0709:0626 through the first external CALL FAR 0AF8:0B25.
 * Input is the exact first-printer entry state. The runtime copy body and
 * rest of the printer are outside this boundary. */
int pr_0709_0626_first_copy_gate(uint8_t *ram, size_t ram_size,
                                  pr_1816_caller_cpu *cpu);

#endif
