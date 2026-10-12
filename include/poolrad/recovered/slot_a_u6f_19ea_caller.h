#ifndef POOLRAD_RECOVERED_SLOT_A_U6F_19EA_CALLER_H
#define POOLRAD_RECOVERED_SLOT_A_U6F_19EA_CALLER_H

#include "poolrad/recovered/slot_a_directory_fullstate.h"

typedef enum {
    PR_SLOT_A_U6F_19EA_OK = 0,
    PR_SLOT_A_U6F_19EA_BOUNDARY
} pr_slot_a_u6f_19ea_status;

/* One reached caller instruction at 2A8E:19EA, 19EE, 19F2, or 19F4.
 * The saved unequal path stops at 2A8E:1937. The equal branch at 19F2
 * and all instructions at 1937 and later remain outside this model. */
pr_slot_a_u6f_19ea_status pr_slot_a_u6f_19ea_step(
    pr_slot_a_directory_state *state);

#endif
