#ifndef POOLRAD_RECOVERED_RESIDENT_464_MASKED_BYTE_0C3D_H
#define POOLRAD_RECOVERED_RESIDENT_464_MASKED_BYTE_0C3D_H

#include "poolrad/recovered/slot_a_directory_fullstate.h"

typedef enum {
    PR_R464_MASKED_BYTE_AT_0C6E = 0,
    PR_R464_MASKED_BYTE_WRONG_ENTRY,
    PR_R464_MASKED_BYTE_MEMORY_BOUNDARY
} pr_r464_masked_byte_status;

/* One reached RES_0464:08D4 masked-byte iteration, +0C3D..+0C6E.
 * The complete CPU and guest RAM at +0C3D are input. A successful step
 * stops before +0C6E; it does not interpret the surrounding loop/callbacks.
 * All reads and the one store are checked before any state is changed. */
pr_r464_masked_byte_status pr_r464_masked_byte_0c3d(
    pr_slot_a_directory_state *state);

#endif
