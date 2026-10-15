#ifndef POOLRAD_RECOVERED_OVERLAY_U3BD_POST_UP_097F_RESET_CALL_H
#define POOLRAD_RECOVERED_OVERLAY_U3BD_POST_UP_097F_RESET_CALL_H

#include <stddef.h>
#include <stdint.h>

/* +097F..+09A7 resets local coordinates and sets up a second +047B call.
 * This gate stops before executing the call or its +09AA return branch. */
typedef struct {
    uint8_t *ram;
    size_t ram_size;
    uint16_t cs, ss, bp, sp, ip, ax, flags;
    uint16_t known_arith_flags;
} pr_u3bd_post_up_097f_reset_call_state;

typedef struct {
    uint16_t x, y;
    uint8_t query;
    uint16_t pushed_x, pushed_y, pushed_query, pushed_cs;
} pr_u3bd_post_up_097f_reset_call_report;

typedef enum {
    PR_U3BD_POST_UP_097F_RESET_CALL_OK = 0,
    PR_U3BD_POST_UP_097F_RESET_CALL_ARGUMENT,
    PR_U3BD_POST_UP_097F_RESET_CALL_BOUNDARY
} pr_u3bd_post_up_097f_reset_call_status;

pr_u3bd_post_up_097f_reset_call_status pr_u3bd_post_up_097f_reset_call(
    pr_u3bd_post_up_097f_reset_call_state *state,
    pr_u3bd_post_up_097f_reset_call_report *report);

#endif
