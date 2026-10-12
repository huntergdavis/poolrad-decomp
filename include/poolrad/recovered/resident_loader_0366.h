#ifndef POOLRAD_RECOVERED_RESIDENT_LOADER_0366_H
#define POOLRAD_RECOVERED_RESIDENT_LOADER_0366_H

#include "poolrad/recovered/resident_handler_prefix_02e6.h"

typedef enum {
    PR_LOADER_0366_AT_FAR_CALL = 0,
    PR_LOADER_0366_AT_INTERNAL_CALL,
    PR_LOADER_0366_ARGUMENT
} pr_loader_0366_status;

/* Execute from +0366 through the first reached call boundary. The saved
 * route stops before +03AA CALL FAR [8504]. If DX > AX unsigned, it stops
 * before +0369 CALL +03E9, with the preceding PUSH DX applied. */
pr_loader_0366_status pr_resident_loader_0366(
    uint8_t *ram, size_t ram_size, pr_handler_02e6_cpu *cpu);

#endif
