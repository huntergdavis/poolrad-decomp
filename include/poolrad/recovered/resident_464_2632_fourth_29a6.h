#ifndef POOLRAD_RECOVERED_RESIDENT_464_2632_FOURTH_29A6_H
#define POOLRAD_RECOVERED_RESIDENT_464_2632_FOURTH_29A6_H

#include "poolrad/recovered/resident_464_2632_run014.h"

/* Saved fourth-call +29A6 successor through the +29C6 runtime Move
 * call entry. Does not execute the call or the alternate +29AB arm. */
pr_r464_2632_status pr_r464_2632_fourth_29a6(
    uint8_t *ram, size_t ram_size, pr_r464_2632_cpu *cpu,
    pr_r464_2632_observer observer, void *context);

#endif
