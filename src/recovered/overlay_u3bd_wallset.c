/* Portable C model of GAME.OVR unit 0x3bd entry 7 (+0x10C7); see
 * docs/re/U3BD_WALLSET_ENTRY7.md. Local effects in the original's order;
 * runtime and resident call-outs are callbacks; the notice path is
 * terminal (its last call terminates the program). */
#include "poolrad/recovered/overlay_u3bd_wallset.h"

#include <string.h>

pr_u3bd_wallset_status pr_u3bd_entry7(pr_u3bd_wallset_state *state, uint8_t id,
                                      const pr_u3bd_wallset_callouts *callouts,
                                      pr_u3bd_wallset_report *report)
{
    pr_u3bd_wallset_report local;
    pr_u3bd_wallset_report *r = report ? report : &local;
    const uint8_t *block = NULL;
    uint16_t size = 0;
    size_t capacity = 0;
    unsigned q;
    memset(r, 0, sizeof *r);
    if (!state || !callouts) return PR_U3BD_WALLSET_ARGUMENT;

    /* +0x10CE..+0x1120: Str([0x5376]) appended to the 3-character literal, the 4-character
     * literal, then resident 0x802:0x0716 (id, &size, &block) */
    if (!callouts->read_block) return PR_U3BD_WALLSET_CALLOUT;
    callouts->read_block(callouts->ctx, state->file_number, id, &block, &size, &capacity);
    r->read_size = size;

    /* +0x1125..+0x1130: size == 0 -> notice; size != 0x402 -> notice; else +0x1160.
     * +0x1132..+0x1156 notice display and key read; +0x115B 0x899:0x0000 = DOS terminate. */
    if (size != PR_U3BD_WALLSET_BLOCK_SIZE) {
        if (!callouts->error_notice) return PR_U3BD_WALLSET_CALLOUT;
        callouts->error_notice(callouts->ctx);
        r->notice = 1;
        if (!callouts->exit_program) return PR_U3BD_WALLSET_CALLOUT;
        callouts->exit_program(callouts->ctx);
        r->exited = 1;
        return PR_U3BD_WALLSET_EXITED;
    }
    /* +0x1160..+0x11DF: four Move(block + 2 + q*0x100, [0x6A5C] + q*0x100, 0x100).
     * The original first touches far [0x6A5C] here, so the host buffer is checked here;
     * a loader that reported 0x402 bytes without a block is a host boundary. */
    if (!state->buffer) return PR_U3BD_WALLSET_ARGUMENT;
    if (!block || capacity < PR_U3BD_WALLSET_BLOCK_SIZE) return PR_U3BD_WALLSET_BLOCK_BOUNDS;
    for (q = 0; q < 4u; ++q)
        memcpy(state->buffer + q * PR_U3BD_WALLSET_QUARTER,
               block + PR_U3BD_WALLSET_DATA_OFFSET + q * PR_U3BD_WALLSET_QUARTER,
               PR_U3BD_WALLSET_QUARTER);
    r->copied = 1;
    /* +0x11E4..+0x11ED: FreeMem(block, size) */
    if (!callouts->release) return PR_U3BD_WALLSET_CALLOUT;
    callouts->release(callouts->ctx, block, size);
    /* +0x11F2..+0x11FB: window 0 word 0xC5 ([0x49D2] + 0x18A) = zero-extended id */
    state->window0_word_c5 = id;
    r->stored_id = id;
    return PR_U3BD_WALLSET_OK;
}
