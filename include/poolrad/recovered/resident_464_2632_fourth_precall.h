#ifndef POOLRAD_RECOVERED_RESIDENT_464_2632_FOURTH_PRECALL_H
#define POOLRAD_RECOVERED_RESIDENT_464_2632_FOURTH_PRECALL_H

#include "poolrad/recovered/resident_464_2632_run014.h"

/* Bounded fourth-call +28D9 selector and its saved +28E0 arithmetic/stack
 * prelude. Stops before runtime FillChar at +291C, or before +28DD when
 * the selector is not one. Neither successor is executed. */
pr_r464_2632_status pr_r464_2632_fourth_precall(
    uint8_t *ram, size_t ram_size, pr_r464_2632_cpu *cpu,
    pr_r464_2632_observer observer, void *context);

#endif
