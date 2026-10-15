#ifndef POOLRAD_RECOVERED_OVERLAY_U3BD_POST_UP_TILE_LOOP_GATE_H
#define POOLRAD_RECOVERED_OVERLAY_U3BD_POST_UP_TILE_LOOP_GATE_H

#include <stddef.h>
#include <stdint.h>

/* Unit 0x3BD:+0300..+030E after one reached entry-2 tile return.
 * ss is one 16-bit segment window; offsets wrap exactly as on 8086. */
typedef struct {
    uint8_t *ss;
    size_t ss_size;
    uint16_t bp;
    uint16_t ax;
    uint16_t flags;
} pr_u3bd_post_up_tile_loop_gate_state;

typedef struct {
    uint8_t counter_before;
    uint8_t counter_after;
    uint16_t lhs;
    uint16_t rhs;
    uint16_t next_ip; /* +030E when equal, +0288 otherwise */
} pr_u3bd_post_up_tile_loop_gate_report;

typedef enum {
    PR_U3BD_POST_UP_TILE_LOOP_GATE_OK = 0,
    PR_U3BD_POST_UP_TILE_LOOP_GATE_ARGUMENT,
    PR_U3BD_POST_UP_TILE_LOOP_GATE_BOUNDARY
} pr_u3bd_post_up_tile_loop_gate_status;

pr_u3bd_post_up_tile_loop_gate_status pr_u3bd_post_up_tile_loop_gate(
    pr_u3bd_post_up_tile_loop_gate_state *state,
    pr_u3bd_post_up_tile_loop_gate_report *report);

#endif
