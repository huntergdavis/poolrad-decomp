#ifndef POOLRAD_RECOVERED_RESIDENT_464_2632_FOURTH_POSTFILL_H
#define POOLRAD_RECOVERED_RESIDENT_464_2632_FOURTH_POSTFILL_H

#include "poolrad/recovered/resident_464_2632_run014.h"

/* Starts at the saved fourth call's FillChar return, 0C88:+2921.
 * Executes the four local mask checks and loop setup, then stops before
 * the first DS global write at 0C88:+2968. This is a bounded path. */
pr_r464_2632_status pr_r464_2632_fourth_postfill(
    uint8_t *ram, size_t ram_size, pr_r464_2632_cpu *cpu,
    pr_r464_2632_observer observer, void *context);

#endif
