#ifndef POOLRAD_RECOVERED_OVERLAY_U3BD_POST_UP_OUTER_LOOP_GATE_H
#define POOLRAD_RECOVERED_OVERLAY_U3BD_POST_UP_OUTER_LOOP_GATE_H

#include <stddef.h>
#include <stdint.h>

/* Unit 0x3BD:+030E..+0319 after the reached inner tile-loop comparison.
 * ss is one 16-bit segment window; offsets wrap as on 8086. */
typedef struct {
    const uint8_t *ss;
    size_t ss_size;
    uint16_t bp;
    uint16_t ax;
    uint16_t flags;
} pr_u3bd_post_up_outer_loop_gate_state;

typedef struct {
    uint16_t lhs;
    uint16_t rhs;
    uint16_t next_ip; /* +0319 when equal, +026E otherwise. */
} pr_u3bd_post_up_outer_loop_gate_report;

typedef enum {
    PR_U3BD_POST_UP_OUTER_LOOP_GATE_OK = 0,
    PR_U3BD_POST_UP_OUTER_LOOP_GATE_ARGUMENT,
    PR_U3BD_POST_UP_OUTER_LOOP_GATE_BOUNDARY
} pr_u3bd_post_up_outer_loop_gate_status;

pr_u3bd_post_up_outer_loop_gate_status pr_u3bd_post_up_outer_loop_gate(
    pr_u3bd_post_up_outer_loop_gate_state *state,
    pr_u3bd_post_up_outer_loop_gate_report *report);

#endif
