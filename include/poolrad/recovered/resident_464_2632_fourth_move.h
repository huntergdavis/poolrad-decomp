#ifndef POOLRAD_RECOVERED_RESIDENT_464_2632_FOURTH_MOVE_H
#define POOLRAD_RECOVERED_RESIDENT_464_2632_FOURTH_MOVE_H

#include "poolrad/recovered/resident_464_2632_run014.h"

/* Saved-view fourth call, resident +2646 through the taken JNE at +2651.
 * Stops at +2656 before the next CMP/JGE. The zero-pointer branch and
 * other call frames are unsupported without mutation. */
pr_r464_2632_status pr_r464_2632_fourth_move(
    uint8_t *ram, size_t ram_size, pr_r464_2632_cpu *cpu,
    pr_r464_2632_observer observer, void *context);

#endif
