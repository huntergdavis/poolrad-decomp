#ifndef POOLRAD_RECOVERED_RESIDENT_464_2632_FOURTH_29AB_H
#define POOLRAD_RECOVERED_RESIDENT_464_2632_FOURTH_29AB_H

#include "poolrad/recovered/resident_464_2632_run014.h"

/* Original-byte derived fourth-call +29AB increment arm to the next
 * +29C6 Move entry. Does not execute that call or claim native parity. */
pr_r464_2632_status pr_r464_2632_fourth_29ab(
    uint8_t *ram, size_t ram_size, pr_r464_2632_cpu *cpu,
    pr_r464_2632_observer observer, void *context);

#endif
