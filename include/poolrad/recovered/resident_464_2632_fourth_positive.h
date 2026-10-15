#ifndef POOLRAD_RECOVERED_RESIDENT_464_2632_FOURTH_POSITIVE_H
#define POOLRAD_RECOVERED_RESIDENT_464_2632_FOURTH_POSITIVE_H

#include "poolrad/recovered/resident_464_2632_run014.h"

/* Saved fourth call, resident +2656 CMP/JGE through the positive-index
 * arm. Stops at +2699 before the next JL. Negative and other unmeasured
 * paths are unsupported without mutation. */
pr_r464_2632_status pr_r464_2632_fourth_positive(
    uint8_t *ram, size_t ram_size, pr_r464_2632_cpu *cpu,
    pr_r464_2632_observer observer, void *context);

#endif
