#ifndef POOLRAD_RECOVERED_RESIDENT_LOADER_0553_H
#define POOLRAD_RECOVERED_RESIDENT_LOADER_0553_H

#include "poolrad/recovered/resident_handler_prefix_02e6.h"

typedef enum {
    PR_LOADER_0553_RETURNED = 0,
    PR_LOADER_0553_ARGUMENT
} pr_loader_0553_status;

/* Execute loaded +0553..+055E from callee entry through its near RET.
 * The near return address is already at SS:SP. The caller supplies a
 * 1 MiB RAM image and a CPU state at +0553. */
pr_loader_0553_status pr_resident_loader_0553(
    uint8_t *ram, size_t ram_size, pr_handler_02e6_cpu *cpu);

#endif
