#ifndef POOLRAD_RECOVERED_RESIDENT_LOADER_READ_GATE_0291_H
#define POOLRAD_RECOVERED_RESIDENT_LOADER_READ_GATE_0291_H

#include "poolrad/recovered/resident_handler_prefix_02e6.h"

typedef enum {
    PR_LOADER_READ_GATE_0291_AT_DOS_READ = 0,
    PR_LOADER_READ_GATE_0291_ARGUMENT
} pr_loader_read_gate_0291_status;

/* Execute loaded 12C6:0291..029E after DOS seek returns. Stop before
 * +029F INT 21h (AH=3Fh read); that external service is not modeled. */
pr_loader_read_gate_0291_status pr_resident_loader_read_gate_0291(
    const uint8_t *ram, size_t ram_size, pr_handler_02e6_cpu *cpu);

#endif
