#ifndef POOLRAD_RECOVERED_RESIDENT_464_2632_FOURTH_SELECT_H
#define POOLRAD_RECOVERED_RESIDENT_464_2632_FOURTH_SELECT_H

#include "poolrad/recovered/resident_464_2632_run014.h"

/* Bounded +277D selector: stop before +2792 for byte 0 or 2, or
 * before +28D9 for every other byte. Neither successor is executed. */
pr_r464_2632_status pr_r464_2632_fourth_select(
    uint8_t *ram, size_t ram_size, pr_r464_2632_cpu *cpu,
    pr_r464_2632_observer observer, void *context);

#endif
