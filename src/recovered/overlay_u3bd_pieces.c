/* Portable C model of GAME.OVR unit 0x3bd entry 6 (+0x0E08); see
 * docs/re/U3BD_PIECES_ENTRY6.md for the instruction-level derivation.
 * Local effects are reproduced in the original's order; the runtime and
 * resident call-outs are callbacks. The notice path is terminal: its last
 * call, resident 0x899:0x0000, terminates the program. */
#include "poolrad/recovered/overlay_u3bd_pieces.h"

#include <string.h>

int pr_u3bd_slot_in_set(uint8_t slot)
{
    return slot >= 1u && slot <= 3u;     /* set constant at +0x0DE8: members {1, 2, 3} */
}

pr_u3bd_status pr_u3bd_entry6(pr_u3bd_state *state, uint8_t id, uint8_t slot,
                              const pr_u3bd_callouts *callouts, pr_u3bd_report *report)
{
    pr_u3bd_report local;
    pr_u3bd_report *r = report ? report : &local;
    const uint8_t *block = NULL;
    uint16_t size = 0;
    size_t capacity = 0;
    unsigned offset, i, count;
    uint8_t delta;
    memset(r, 0, sizeof *r);
    if (!state || !state->pieces || !callouts) return PR_U3BD_ARGUMENT;

    /* +0x0E0F..+0x0E1B: cmp byte [bp+6],1; jb exit; cmp byte [bp+6],3; ja exit */
    if (slot < 1u || slot > 3u) return PR_U3BD_OK;
    r->slot_ok = 1;

    /* +0x0E1E..+0x0E70: Str([0x5376]) appended to the 7-character literal, the 4-character
     * literal, then resident 0x802:0x0716 (id, &size, &block) */
    if (!callouts->read_block) return PR_U3BD_CALLOUT;
    callouts->read_block(callouts->ctx, state->file_number, id, &block, &size, &capacity);
    r->read_size = size;

    /* +0x0E75..+0x0E91: if size == 0, or size / 780 + slot > 4, take the notice path:
     * +0x0E93..+0x0EB7 display and key read, then +0x0EBC 0x899:0x0000 = DOS terminate.
     * Nothing after this point executes in the original. */
    if (size == 0 || (unsigned)(size / PR_U3BD_PIECE_SIZE) + slot > 4u) {
        if (!callouts->error_notice) return PR_U3BD_CALLOUT;
        callouts->error_notice(callouts->ctx);
        r->notice = 1;
        if (!callouts->exit_program) return PR_U3BD_CALLOUT;
        callouts->exit_program(callouts->ctx);
        r->exited = 1;
        return PR_U3BD_EXITED;
    }
    /* From here size >= 1 and size / 780 + slot <= 4, so every copied piece lands in
     * slot .. slot + ceil(size / 780) - 1 <= 4: the buffer cannot be overrun. */

    /* +0x0EC1..+0x0EDA: delta = table[slot] - table[1]; offset = 0; i = 1 */
    delta = (uint8_t)(state->table_2722[slot] - state->table_2722[1]);
    offset = 0;
    i = 1;
    /* +0x0EDF..+0x0F1A: do { Move(block + offset, pieces + (slot + i - 2) * 780, 780);
     * offset += 780; i++; } while (offset < size) */
    do {
        unsigned target = slot + i - 1u;
        if ((size_t)offset + PR_U3BD_PIECE_SIZE > capacity) return PR_U3BD_BLOCK_BOUNDS;
        memcpy(state->pieces + (target - 1u) * PR_U3BD_PIECE_SIZE, block + offset, PR_U3BD_PIECE_SIZE);
        r->pieces_copied++;
        offset += PR_U3BD_PIECE_SIZE;
        i++;
    } while (offset < size);
    /* +0x0F1C..+0x0F25: FreeMem(block, size) */
    if (!callouts->release) return PR_U3BD_CALLOUT;
    callouts->release(callouts->ctx, block, size);
    /* +0x0F2A..+0x0F3F: count = i - 1 (byte); the loop is skipped when count < 1, which
     * cannot happen after at least one copy */
    count = (uint8_t)(i - 1u);
    r->count = count;
    for (i = 1; ; ++i) {
        unsigned target = slot + i - 1u;
        /* +0x0F4C..+0x0F62: slot-set test; members {1, 2, 3} */
        if (pr_u3bd_slot_in_set((uint8_t)target)) {
            uint8_t *piece = state->pieces + (target - 1u) * PR_U3BD_PIECE_SIZE;
            unsigned row, k;
            uint16_t arg;
            /* +0x0F65..+0x0F8F: both table words of the slot = 0xFFFF */
            state->slots[target].w6aae = 0xFFFFu;
            state->slots[target].w6ab0 = 0xFFFFu;
            /* +0x0F8F..+0x1012: for row 1..5, for k 0..0x9B: byte at (row-1)*156 + k;
             * if byte >= [0x272C] (word compare) add delta (byte, wrapping) */
            for (row = 0; row < PR_U3BD_PIECE_ROWS; ++row)
                for (k = 0; k < PR_U3BD_PIECE_ROW; ++k) {
                    uint8_t *at = piece + row * PR_U3BD_PIECE_ROW + k;
                    if ((uint16_t)*at >= state->threshold_272c) *at = (uint8_t)(*at + delta);
                }
            r->remapped++;
            /* +0x1014..+0x1051: announce through unit 0x40c entry 1 */
            if (count > 1u) {
                if (id == 0) id = 10;                    /* mov byte [bp+8], 0xA: persists */
                arg = (uint16_t)((unsigned)id * 10u + i);
            } else {
                arg = id;
            }
            if (!callouts->piece_loaded) return PR_U3BD_CALLOUT;
            callouts->piece_loaded(callouts->ctx, arg, (uint8_t)target);
            r->announced++;
        }
        /* +0x1056..+0x105E: until i == count */
        if (i == count) break;
    }
    /* +0x1061..+0x1093: if id == 10 then id = 0; slots[slot] = (id, slot) */
    if (id == 10u) id = 0;
    state->slots[slot].w6aae = id;
    state->slots[slot].w6ab0 = slot;
    r->final_id = id;
    return PR_U3BD_OK;
}
