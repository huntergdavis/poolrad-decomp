#ifndef POOLRAD_RECOVERED_RESIDENT_464_29D7_EXTENT_BRANCH_H
#define POOLRAD_RECOVERED_RESIDENT_464_29D7_EXTENT_BRANCH_H

#include "poolrad/recovered/resident_464_2632_run014.h"

enum { PR_R464_29D7_RAM_BYTES = 0xA0000u };

typedef enum {
    PR_R464_29D7_OK = 0,
    PR_R464_29D7_INVALID_HOST = 1,
    PR_R464_29D7_UNSUPPORTED_PATH = 2
} pr_r464_29d7_status;

/* From the supplied +29D7 loop-exit frame, execute only through JBE +29E4.
 * Stop at +29E6 or +29F3 before either successor writes RAM. */
pr_r464_29d7_status pr_r464_29d7_extent_branch(
    const uint8_t *ram, size_t ram_size, pr_r464_2632_cpu *cpu);

#endif
