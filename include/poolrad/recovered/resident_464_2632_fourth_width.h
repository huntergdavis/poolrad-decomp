#ifndef POOLRAD_RECOVERED_RESIDENT_464_2632_FOURTH_WIDTH_H
#define POOLRAD_RECOVERED_RESIDENT_464_2632_FOURTH_WIDTH_H

#include "poolrad/recovered/resident_464_2632_run014.h"

/* Saved fourth call at +2699: JL/JG fallthrough, JAE to +26CD,
 * width remainder calculation, then stop before +26FB JGE.
 * Other branch outcomes are unsupported without mutation. */
pr_r464_2632_status pr_r464_2632_fourth_width(
    uint8_t *ram, size_t ram_size, pr_r464_2632_cpu *cpu,
    pr_r464_2632_observer observer, void *context);

#endif
