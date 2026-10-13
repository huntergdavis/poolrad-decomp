#ifndef POOLRAD_RECOVERED_SLOT_A_U6F_1937_PRECALL_H
#define POOLRAD_RECOVERED_SLOT_A_U6F_1937_PRECALL_H

#include "poolrad/recovered/slot_a_directory_fullstate.h"

typedef enum {
    PR_SLOT_A_U6F_1937_OK = 0,
    PR_SLOT_A_U6F_1937_BOUNDARY
} pr_slot_a_u6f_1937_status;

/* One instruction in the reached 2A8E:1937..1964 caller setup.
 * The first external call at 2A8E:1965 is a terminal boundary. */
pr_slot_a_u6f_1937_status pr_slot_a_u6f_1937_step(
    pr_slot_a_directory_state *state);

#endif
