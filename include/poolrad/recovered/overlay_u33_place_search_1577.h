#ifndef POOLRAD_RECOVERED_OVERLAY_U33_PLACE_SEARCH_1577_H
#define POOLRAD_RECOVERED_OVERLAY_U33_PLACE_SEARCH_1577_H

#include <stddef.h>
#include <stdint.h>

#define PR_U33_1577_RAM_SIZE (1u << 20)

/* 14-word raw CPU state shared with the +1403, +01AD and +1434 models. */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u33_1577_cpu;

typedef enum {
    PR_U33_1577_OK = 0,
    PR_U33_1577_ARGUMENT,   /* null RAM/CPU or RAM that is not exactly 1 MiB; nothing changed */
    PR_U33_1577_CODE,       /* a far-call site or stub inside +01AD's or +1434's chain is not original */
    PR_U33_1577_CALLEE,     /* +1434's +0CB9 model stopped; its status is in *callee_status */
    PR_U33_1577_TRANSFER,   /* a callee's RETF did not return to its call frame */
    PR_U33_1577_LIMIT       /* the host cap on loop-head entries was reached; CS:IP = entry + 34h */
} pr_u33_1577_status;

/* Complete GAME.OVR unit 0x33:+1577..+19F4 (1,150 bytes, 470 instructions,
 * 35 conditional branches), entered after the far CALL with SS:[SP]
 * return IP, [SP+2] return CS and [SP+4] a word whose low byte is the slot
 * q; RETF 2. It searches the 11 by 6 grid around the origin
 * DS:[4650/4652 + sign(DS:4658)] for a cell that +1434 claims for slot q:
 * from a base on ring r of direction DS:[2E2 + 4*sign(DS:4656) + pass]/2 it
 * tries perpendicular offsets of growing length (states 1, 2, 3 in
 * [BP-7]); in-grid positions go to +1434, a position outside both axes
 * (+1403) or a step limit (DS:[4654 + sign(DS:4658)] on the first ring,
 * 0Bh later) moves to the next ring, and the ring/pass logic re-seeds the
 * origin from DS:6AAD/6AAE through +01AD for up to three passes. AL = 1
 * when the loop ends with [BP-2] nonzero and [BP-4] zero, else 0; the
 * final flags are CMP [BP-4],0's. [BP-2] is read before any write when
 * the first positions lie outside the grid, so stale stack bytes take
 * part, exactly as in the original.
 *
 * The five near calls push CS and the return offset relative to the
 * entry cpu->ip (0x1577 in the game) and enter the merged +1403
 * (ip - 174h), +01AD (ip - 13CAh) and +1434 (ip - 143h) models on the
 * same RAM and CPU words. Every operand is reread from RAM; offsets wrap
 * at 16 bits, physical addresses at 1 MiB; the RETF frame is read
 * physically contiguous. Dead intermediate flag writes are folded.
 *
 * With degenerate direction tables the original never returns, so the
 * host caps the number of entries into the loop head +15AB at
 * max_loop_entries: on the next entry the model stops before +15AB with
 * every earlier effect kept (status LIMIT). */
pr_u33_1577_status pr_u33_place_search_1577(
    uint8_t *ram, size_t ram_size, pr_u33_1577_cpu *cpu, unsigned max_loop_entries,
    int *callee_status);

#endif
