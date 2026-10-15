#ifndef POOLRAD_RECOVERED_OVERLAY_U3BD_POST_UP_028B_BOUNDS_GATE_H
#define POOLRAD_RECOVERED_OVERLAY_U3BD_POST_UP_028B_BOUNDS_GATE_H

#include <stddef.h>
#include <stdint.h>

/* Original unit 0x3BD:+028B..+02A3. ss is one guest stack segment. */
typedef struct {
    const uint8_t *ss;
    size_t ss_size;
    uint16_t bp;
    uint16_t flags;
} pr_u3bd_post_up_028b_bounds_state;

typedef struct {
    uint16_t outer;
    uint16_t inner;
    uint16_t next_ip; /* +02A3 in range, +0300 otherwise. */
    unsigned comparisons; /* Number of reached CMP instructions. */
} pr_u3bd_post_up_028b_bounds_report;

typedef enum {
    PR_U3BD_POST_UP_028B_BOUNDS_OK = 0,
    PR_U3BD_POST_UP_028B_BOUNDS_ARGUMENT,
    PR_U3BD_POST_UP_028B_BOUNDS_BOUNDARY
} pr_u3bd_post_up_028b_bounds_status;

pr_u3bd_post_up_028b_bounds_status pr_u3bd_post_up_028b_bounds_gate(
    pr_u3bd_post_up_028b_bounds_state *state,
    pr_u3bd_post_up_028b_bounds_report *report);

#endif
