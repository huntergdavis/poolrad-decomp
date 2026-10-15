#ifndef POOLRAD_RECOVERED_RESIDENT_464_FOURTH_BRANCH_275B_H
#define POOLRAD_RECOVERED_RESIDENT_464_FOURTH_BRANCH_275B_H

#include "poolrad/recovered/resident_464_2632_run014.h"

enum { PR_R464_FOURTH_RAM_BYTES = 0xA0000u };

typedef enum {
    PR_R464_FOURTH_OK = 0,
    PR_R464_FOURTH_INVALID_HOST,
    PR_R464_FOURTH_UNSUPPORTED_PATH
} pr_r464_fourth_status;

/* One measured fourth-call branch gate: loaded RES_0464 +275B JL falls
 * through, +275D JG falls through, CMP AX,CX at +275F executes, and +2761
 * JAE reaches +277D. RAM is read-only. The observer sees each native stop
 * before its instruction; unsupported inputs leave CPU and RAM unchanged. */
pr_r464_fourth_status pr_r464_fourth_branch_275b(
    const uint8_t *ram, size_t ram_size, pr_r464_2632_cpu *cpu,
    pr_r464_2632_observer observer, void *context);

#endif
