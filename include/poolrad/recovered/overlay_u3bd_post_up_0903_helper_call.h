#ifndef POOLRAD_RECOVERED_OVERLAY_U3BD_POST_UP_0903_HELPER_CALL_H
#define POOLRAD_RECOVERED_OVERLAY_U3BD_POST_UP_0903_HELPER_CALL_H

#include <stddef.h>
#include <stdint.h>

/* Reached caller instructions +0903..+0910. Stops at the merged +047B
 * callee entry, before executing any callee instruction. */
typedef struct {
    uint8_t *ram;
    size_t ram_size;
    uint16_t cs, ss, bp, sp, ip, ax, flags;
} pr_u3bd_post_up_0903_helper_call_state;

typedef struct {
    uint8_t x, y, selector;
    uint16_t return_ip;
} pr_u3bd_post_up_0903_helper_call_report;

typedef enum {
    PR_U3BD_POST_UP_0903_HELPER_CALLEE_047B = 0,
    PR_U3BD_POST_UP_0903_HELPER_ARGUMENT,
    PR_U3BD_POST_UP_0903_HELPER_BOUNDARY
} pr_u3bd_post_up_0903_helper_call_status;

pr_u3bd_post_up_0903_helper_call_status pr_u3bd_post_up_0903_helper_call(
    pr_u3bd_post_up_0903_helper_call_state *state,
    pr_u3bd_post_up_0903_helper_call_report *report);

#endif
