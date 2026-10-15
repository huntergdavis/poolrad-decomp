#ifndef POOLRAD_RECOVERED_RESIDENT_464_2632_FOURTH_29D7_H
#define POOLRAD_RECOVERED_RESIDENT_464_2632_FOURTH_29D7_H

#include "poolrad/recovered/resident_464_2632_run014.h"

/* Fourth-call +29D7 exit decision. Stops at the first +29E4 JBE
 * successor, +29E6 or +29F3, before either successor's body. */
pr_r464_2632_status pr_r464_2632_fourth_29d7(
    uint8_t *ram, size_t ram_size, pr_r464_2632_cpu *cpu,
    pr_r464_2632_observer observer, void *context);

#endif
