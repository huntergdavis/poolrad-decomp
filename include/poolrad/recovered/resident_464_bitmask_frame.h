#ifndef POOLRAD_RECOVERED_RESIDENT_464_BITMASK_FRAME_H
#define POOLRAD_RECOVERED_RESIDENT_464_BITMASK_FRAME_H

#include "poolrad/recovered/slot_a_directory_fullstate.h"

/* Complete RES_0464:0000 frame-to-return effects for a readable nonwrapping
 * real-mode stack frame (SP in [8, FFF6]). Reuses pr_r464_bitmask's complete
 * value contract. The caller/audit verifies CS's loaded code independently.
 * Returns 1 on success, 0 without mutation on an inaccessible/invalid frame.
 * Reports final CPU/RAM, not a journal of intermediate local stores. */
int pr_r464_bitmask_frame(pr_slot_a_directory_state *state);

#endif
