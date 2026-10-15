#ifndef POOLRAD_RECOVERED_RESIDENT_464_2632_THIRD_CALL_H
#define POOLRAD_RECOVERED_RESIDENT_464_2632_THIRD_CALL_H

#include "poolrad/recovered/resident_464_2632_run014.h"

/* Bounded saved run014 caller 2D14:01E4 through its far return to 01E9.
 * Direct native return capture is pending; current comparison is against
 * hash-locked original-byte replay from the saved pre/entry images. */
pr_r464_2632_status pr_r464_2632_third_call(
    uint8_t *ram, size_t ram_size, pr_r464_2632_cpu *cpu,
    pr_r464_2632_observer observer, void *context);

#endif
