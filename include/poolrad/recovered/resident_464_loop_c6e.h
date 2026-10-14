#ifndef POOLRAD_RECOVERED_RESIDENT_464_LOOP_C6E_H
#define POOLRAD_RECOVERED_RESIDENT_464_LOOP_C6E_H

#include "poolrad/recovered/slot_a_directory_fullstate.h"

typedef enum {
    PR_R464_LOOP_NEXT_MASKED_BYTE = 0,
    PR_R464_LOOP_WRONG_ENTRY,
    PR_R464_LOOP_OTHER_BRANCH,
    PR_R464_LOOP_MEMORY_BOUNDARY,
    PR_R464_LOOP_ALIAS_BOUNDARY
} pr_r464_loop_status;

/* Measured RES_0464:+0C6E..+0C8C, JNE +0C31..+0C3D backedge.
 * This one-iteration step supports the reached nonzero plane counter and
 * unequal row-selector branches. Other successors stop without mutation.
 * The supplied CPU/RAM is the state immediately before +0C6E. */
pr_r464_loop_status pr_r464_loop_c6e(pr_slot_a_directory_state *state);

#endif
