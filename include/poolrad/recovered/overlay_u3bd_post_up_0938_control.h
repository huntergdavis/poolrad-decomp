#ifndef POOLRAD_RECOVERED_OVERLAY_U3BD_POST_UP_0938_CONTROL_H
#define POOLRAD_RECOVERED_OVERLAY_U3BD_POST_UP_0938_CONTROL_H

#include <stddef.h>
#include <stdint.h>

/* Reached unit 0x3BD caller path +0938 -> +0951..+097C -> +08FD..+0903.
 * The +093A art-call arm and the +0903 successor are outside this gate. */
typedef struct {
    uint8_t *ram;
    size_t ram_size;
    uint16_t ss, ds, bp, sp, ip;
    uint16_t ax, di, flags;
    uint16_t known_arith_flags; /* Defined bits among CF,PF,AF,ZF,SF,OF. */
} pr_u3bd_post_up_0938_control_state;

typedef struct {
    uint8_t loop_before, loop_after, index, dir_x, dir_y;
    uint16_t step_before, step_after;
    uint16_t x_before, x_after, y_before, y_after;
    uint16_t next_ip; /* +0903 on fallthrough; +097F on unsigned >=. */
} pr_u3bd_post_up_0938_control_report;

typedef enum {
    PR_U3BD_POST_UP_0938_CONTROL_OK = 0,
    PR_U3BD_POST_UP_0938_CONTROL_ARGUMENT,
    PR_U3BD_POST_UP_0938_CONTROL_BOUNDARY
} pr_u3bd_post_up_0938_control_status;

pr_u3bd_post_up_0938_control_status pr_u3bd_post_up_0938_control(
    pr_u3bd_post_up_0938_control_state *state,
    pr_u3bd_post_up_0938_control_report *report);

#endif
