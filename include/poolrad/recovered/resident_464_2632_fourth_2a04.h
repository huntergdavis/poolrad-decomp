#ifndef POOLRAD_RECOVERED_RESIDENT_464_2632_FOURTH_2A04_H
#define POOLRAD_RECOVERED_RESIDENT_464_2632_FOURTH_2A04_H

#include "poolrad/recovered/resident_464_2632_run014.h"

/* Fourth-call +2A04 jump, +2962 body, and +297F JBE; stops at successor. */
pr_r464_2632_status pr_r464_2632_fourth_2a04(
    uint8_t *ram, size_t ram_size, pr_r464_2632_cpu *cpu,
    pr_r464_2632_observer observer, void *context);

#endif
