#ifndef POOLRAD_RECOVERED_OVERLAY_U3BD_POST_UP_02C4_TILE_CALL_H
#define POOLRAD_RECOVERED_OVERLAY_U3BD_POST_UP_02C4_TILE_CALL_H

#include <stddef.h>
#include <stdint.h>

/* Original unit 0x3BD:+02C4..+0300. ram is a mutable conventional-memory
 * image. Registers except SP describe the +02FB call entry after success;
 * the external callee's register effects remain outside this model. */
typedef struct {
    uint8_t *ram;
    size_t ram_size;
    uint16_t ss, ds, bp, sp;
    uint16_t ax, cx, dx, es, di, flags;
    uint16_t known_arith_flags;
} pr_u3bd_post_up_02c4_tile_call_state;

typedef struct {
    uint16_t x, y, item, one_a, one_b;
    uint16_t call_sp, next_ip; /* +02FB at boundary, +0300 after callback. */
    uint32_t item_physical;
} pr_u3bd_post_up_02c4_tile_call_report;

/* Receives the five original pushed words in named order. A zero return
 * means the external callee completed; nonzero preserves the call boundary. */
typedef int (*pr_u3bd_post_up_02c4_tile_callback)(
    void *ctx, uint16_t x, uint16_t y, uint16_t item,
    uint16_t one_a, uint16_t one_b);

typedef enum {
    PR_U3BD_POST_UP_02C4_TILE_CALL_OK = 0,
    PR_U3BD_POST_UP_02C4_TILE_CALL_ARGUMENT,
    PR_U3BD_POST_UP_02C4_TILE_CALL_BOUNDARY,
    PR_U3BD_POST_UP_02C4_TILE_CALL_CALLBACK_REQUIRED,
    PR_U3BD_POST_UP_02C4_TILE_CALL_CALLBACK_ERROR
} pr_u3bd_post_up_02c4_tile_call_status;

pr_u3bd_post_up_02c4_tile_call_status pr_u3bd_post_up_02c4_tile_call(
    pr_u3bd_post_up_02c4_tile_call_state *state,
    pr_u3bd_post_up_02c4_tile_callback callback, void *callback_ctx,
    pr_u3bd_post_up_02c4_tile_call_report *report);

#endif
