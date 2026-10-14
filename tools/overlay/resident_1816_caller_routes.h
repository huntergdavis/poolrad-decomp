#ifndef POOLRAD_RESIDENT_1816_CALLER_ROUTES_H
#define POOLRAD_RESIDENT_1816_CALLER_ROUTES_H

#include <stddef.h>
#include <stdint.h>

#define PR_1816_CALLER_RAM_SIZE (1u << 20)
#define PR_1816_CALLER_SITE_COUNT 8u

typedef struct {
    uint16_t ax, bx, cx, dx, si, di, bp, sp;
    uint16_t cs, ip, ds, es, ss, flags;
} pr_1816_caller_cpu;

typedef struct {
    uint16_t flag, row, column, node_offset, node_segment;
} pr_1816_caller_frame;

typedef enum {
    PR_1816_PUSH_NODE_SEGMENT,
    PR_1816_PUSH_NODE_OFFSET,
    PR_1816_PUSH_COLUMN,
    PR_1816_PUSH_ROW,
    PR_1816_PUSH_FLAG,
    PR_1816_CALL_DISPLAY
} pr_1816_caller_event;

/* The five PUSH events are read-only trace points. At CALL_DISPLAY, the
 * callback stands for the whole resident callee and may change RAM and CPU
 * registers other than CS, IP, and SP. The model then applies RETF 10. */
typedef int (*pr_1816_caller_callback)(
    void *ctx, uint8_t *ram, size_t ram_size, pr_1816_caller_cpu *cpu,
    unsigned site, pr_1816_caller_event event, uint16_t pushed_word,
    const pr_1816_caller_frame *frame);

typedef enum {
    PR_1816_CALLER_OK = 0,
    PR_1816_CALLER_ARGUMENT,
    PR_1816_CALLER_BOUNDARY
} pr_1816_caller_status;

/* Site order is documented in RESIDENT_1816_OVERLAY_CALLER_ROUTES.md.
 * The CPU enters at the first node-pointer PUSH, immediately before the
 * direct CALL FAR 00BA:1816. This models only that straight-line prefix. */
pr_1816_caller_status pr_1816_caller_route_step(
    unsigned site, uint8_t *ram, size_t ram_size, pr_1816_caller_cpu *cpu,
    pr_1816_caller_callback callback, void *ctx);

#endif
