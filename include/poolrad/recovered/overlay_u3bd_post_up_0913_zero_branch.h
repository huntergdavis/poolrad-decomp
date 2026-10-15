#ifndef POOLRAD_RECOVERED_OVERLAY_U3BD_POST_UP_0913_ZERO_BRANCH_H
#define POOLRAD_RECOVERED_OVERLAY_U3BD_POST_UP_0913_ZERO_BRANCH_H

#include <stddef.h>
#include <stdint.h>

/* Reached caller continuation +0913..+091A after the +047B callee returns.
 * Stops at +0951 for zero AL, or +091C otherwise. */
typedef struct {
    uint8_t *ram;
    size_t ram_size;
    uint16_t cs, ss, bp, sp, ip, ax, flags;
} pr_u3bd_post_up_0913_zero_branch_state;

typedef struct {
    uint8_t result;
    uint16_t next_ip;
} pr_u3bd_post_up_0913_zero_branch_report;

typedef enum {
    PR_U3BD_POST_UP_0913_ZERO_BRANCH_OK = 0,
    PR_U3BD_POST_UP_0913_ZERO_BRANCH_ARGUMENT,
    PR_U3BD_POST_UP_0913_ZERO_BRANCH_BOUNDARY
} pr_u3bd_post_up_0913_zero_branch_status;

pr_u3bd_post_up_0913_zero_branch_status pr_u3bd_post_up_0913_zero_branch(
    pr_u3bd_post_up_0913_zero_branch_state *state,
    pr_u3bd_post_up_0913_zero_branch_report *report);

#endif
