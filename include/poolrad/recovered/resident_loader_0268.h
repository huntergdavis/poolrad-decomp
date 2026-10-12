#ifndef POOLRAD_RECOVERED_RESIDENT_LOADER_0268_H
#define POOLRAD_RECOVERED_RESIDENT_LOADER_0268_H

#include "poolrad/recovered/resident_handler_prefix_02e6.h"

typedef enum {
    PR_LOADER_0268_AT_DOS_SEEK = 0,
    PR_LOADER_0268_ARGUMENT
} pr_loader_0268_status;

/* Execute loaded +0268..+028E from the far-call target entry. Stop before
 * +028F INT 21h (AX=4200h). The caller supplies the far-call stack frame. */
pr_loader_0268_status pr_resident_loader_0268(
    uint8_t *ram, size_t ram_size, pr_handler_02e6_cpu *cpu);

#endif
