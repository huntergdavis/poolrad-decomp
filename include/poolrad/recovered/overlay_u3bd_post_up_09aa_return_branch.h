#ifndef POOLRAD_RECOVERED_OVERLAY_U3BD_POST_UP_09AA_RETURN_BRANCH_H
#define POOLRAD_RECOVERED_OVERLAY_U3BD_POST_UP_09AA_RETURN_BRANCH_H

#include <stddef.h>
#include <stdint.h>

/* Distinct +09AA return from the +09A7 helper call. Stops at +09E8 or
 * +09B3, before either successor body or the later display flush. */
typedef struct {
    uint8_t *ram;
    size_t ram_size;
    uint16_t cs, ss, bp, sp, ip, ax, flags;
} pr_u3bd_post_up_09aa_return_branch_state;

typedef struct {
    uint8_t result;
    uint16_t next_ip;
} pr_u3bd_post_up_09aa_return_branch_report;

typedef enum {
    PR_U3BD_POST_UP_09AA_RETURN_BRANCH_OK = 0,
    PR_U3BD_POST_UP_09AA_RETURN_BRANCH_ARGUMENT,
    PR_U3BD_POST_UP_09AA_RETURN_BRANCH_BOUNDARY
} pr_u3bd_post_up_09aa_return_branch_status;

pr_u3bd_post_up_09aa_return_branch_status pr_u3bd_post_up_09aa_return_branch(
    pr_u3bd_post_up_09aa_return_branch_state *state,
    pr_u3bd_post_up_09aa_return_branch_report *report);

#endif
