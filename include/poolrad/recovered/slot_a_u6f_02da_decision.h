#ifndef POOLRAD_RECOVERED_SLOT_A_U6F_02DA_DECISION_H
#define POOLRAD_RECOVERED_SLOT_A_U6F_02DA_DECISION_H

#include "poolrad/recovered/slot_a_directory_fullstate.h"

typedef enum {
    PR_SLOT_A_U6F_02DA_OK = 0,
    PR_SLOT_A_U6F_02DA_BOUNDARY
} pr_slot_a_u6f_02da_status;

/* One instruction at 2A8E:02DA, 02DD, 02E1, 02E3, or 02E7.
 * 02E9 (other result) and 031F (zero or error 5) are terminal stops.
 * The caller supplies the CPU/RAM image after the preceding IOResult return.
 * A boundary result leaves both unchanged. */
pr_slot_a_u6f_02da_status pr_slot_a_u6f_02da_step(
    pr_slot_a_directory_state *state);

#endif
