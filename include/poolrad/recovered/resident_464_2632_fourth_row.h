#ifndef POOLRAD_RECOVERED_RESIDENT_464_2632_FOURTH_ROW_H
#define POOLRAD_RECOVERED_RESIDENT_464_2632_FOURTH_ROW_H

#include "poolrad/recovered/resident_464_2632_run014.h"

/* Saved fourth call: +26FB JGE to +2722, positive-row arithmetic,
 * stopping at +275B before the next JL. Unsupported shapes do not mutate. */
pr_r464_2632_status pr_r464_2632_fourth_row(
    uint8_t *ram, size_t ram_size, pr_r464_2632_cpu *cpu,
    pr_r464_2632_observer observer, void *context);

#endif
