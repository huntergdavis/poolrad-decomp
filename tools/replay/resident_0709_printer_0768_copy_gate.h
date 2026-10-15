#ifndef POOLRAD_RESIDENT_0709_PRINTER_0768_COPY_GATE_H
#define POOLRAD_RESIDENT_0709_PRINTER_0768_COPY_GATE_H

#include "resident_1816_caller_routes.h"

/* Execute RES_0709:0768 through its first external CALL FAR 0AF8:0B25.
 * Input is a real-mode 1 MiB RAM image and a CPU at 0709:0768. The runtime
 * copy and the rest of this printer helper remain outside this boundary. */
int pr_0709_0768_first_copy_gate(uint8_t *ram, size_t ram_size,
                                  pr_1816_caller_cpu *cpu);

#endif
