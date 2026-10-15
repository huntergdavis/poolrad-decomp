#ifndef POOLRAD_RECOVERED_RESIDENT_464_2632_FOURTH_DS_WRITE_H
#define POOLRAD_RECOVERED_RESIDENT_464_2632_FOURTH_DS_WRITE_H

#include "poolrad/recovered/resident_464_2632_run014.h"

/* Fourth saved-view resident +2968 global byte write and local update,
 * through +297F JBE to the first successor. Stops at +2981 or +2999. */
pr_r464_2632_status pr_r464_2632_fourth_ds_write(
    uint8_t *ram, size_t ram_size, pr_r464_2632_cpu *cpu,
    pr_r464_2632_observer observer, void *context);

#endif
