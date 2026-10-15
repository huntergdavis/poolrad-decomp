#ifndef POOLRAD_RECOVERED_RESIDENT_464_2632_SECOND_CALL_H
#define POOLRAD_RECOVERED_RESIDENT_464_2632_SECOND_CALL_H

#include "poolrad/recovered/resident_464_2632_run014.h"

/* Bounded saved run014 caller 2D14:01BE through the measured far return to
 * 2D14:01C3. The existing one-row resident model handles the body. The
 * observer sees its resident instruction boundaries, starting at +2632. */
pr_r464_2632_status pr_r464_2632_second_call(
    uint8_t *ram, size_t ram_size, pr_r464_2632_cpu *cpu,
    pr_r464_2632_observer observer, void *context);

#endif
