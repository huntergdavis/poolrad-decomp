#ifndef POOLRAD_RECOVERED_RESIDENT_464_2632_FOURTH_FILLCHAR_H
#define POOLRAD_RECOVERED_RESIDENT_464_2632_FOURTH_FILLCHAR_H

#include "poolrad/recovered/resident_464_2632_run014.h"

/* Bounded saved fourth-call +291C relocated runtime FillChar path.
 * Stops at caller +2921 after the reached RETF 8. */
pr_r464_2632_status pr_r464_2632_fourth_fillchar(
    uint8_t *ram, size_t ram_size, pr_r464_2632_cpu *cpu,
    pr_r464_2632_observer observer, void *context);

#endif
