#ifndef POOLRAD_RECOVERED_RESIDENT_LOADER_PREFIX_0338_H
#define POOLRAD_RECOVERED_RESIDENT_LOADER_PREFIX_0338_H

#include "poolrad/recovered/resident_handler_prefix_02e6.h"

/* The caller supplies the state after a near CALL enters loaded +0338.
 * Each successful result names the next unresolved resident callee. */
typedef enum {
    PR_LOADER_0338_CALL_0553 = 0,
    PR_LOADER_0338_CALL_0435,
    PR_LOADER_0338_ARGUMENT
} pr_loader_0338_status;

/* Execute +0338 until the first reached CALL. The saved ES:[0010]=0
 * path stops before +0352 CALL +0553; the other path stops before
 * +03B6 CALL +0435. Neither callee nor the loader tail is modeled. */
pr_loader_0338_status pr_resident_loader_prefix_0338(
    uint8_t *ram, size_t ram_size, pr_handler_02e6_cpu *cpu);

#endif
