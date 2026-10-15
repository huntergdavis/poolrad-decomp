#ifndef POOLRAD_RECOVERED_RESIDENT_464_2632_FOURTH_JG_H
#define POOLRAD_RECOVERED_RESIDENT_464_2632_FOURTH_JG_H

#include "poolrad/recovered/resident_464_2632_run014.h"

/* Complete +275D JG / +275F CMP / +2761 JAE decisions, stopping
 * before successor +277D or +2763. */
pr_r464_2632_status pr_r464_2632_fourth_jg(
    uint8_t *ram, size_t ram_size, pr_r464_2632_cpu *cpu,
    pr_r464_2632_observer observer, void *context);

#endif
