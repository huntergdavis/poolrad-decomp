#ifndef POOLRAD_RECOVERED_SLOT_A_U6F_031F_RETURN_H
#define POOLRAD_RECOVERED_SLOT_A_U6F_031F_RETURN_H

#include "poolrad/recovered/slot_a_directory_fullstate.h"

typedef enum {
    PR_SLOT_A_U6F_031F_OK = 0,
    PR_SLOT_A_U6F_031F_BOUNDARY
} pr_slot_a_u6f_031f_status;

/* One saved error-5 instruction at 2A8E:031F, 0323, 0325, 0329,
 * 032B, 032D, or 032E. Successful RETF stops at 2A8E:19EA.
 * Other result/retry routes and the caller's next instruction are outside
 * this boundary. BOUNDARY preserves the state at the current instruction. */
pr_slot_a_u6f_031f_status pr_slot_a_u6f_031f_step(
    pr_slot_a_directory_state *state);

#endif
