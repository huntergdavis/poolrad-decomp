#ifndef POOLRAD_RECOVERED_RESIDENT_LOADER_0538_H
#define POOLRAD_RECOVERED_RESIDENT_LOADER_0538_H

#include "poolrad/recovered/resident_handler_prefix_02e6.h"

typedef enum {
    PR_LOADER_0538_RETURNED = 0,
    PR_LOADER_0538_ARGUMENT
} pr_loader_0538_status;

/* Execute the complete loaded +0538..+0552 near-call helper. RAM is 1 MiB;
 * the CPU is positioned at +0538 with the caller's return word at SS:SP. */
pr_loader_0538_status pr_resident_loader_0538(
    uint8_t *ram, size_t ram_size, pr_handler_02e6_cpu *cpu);

#endif
