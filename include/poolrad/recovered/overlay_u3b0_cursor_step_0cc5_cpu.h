#ifndef POOLRAD_RECOVERED_OVERLAY_U3B0_CURSOR_STEP_0CC5_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U3B0_CURSOR_STEP_0CC5_CPU_H

#include <stddef.h>
#include <stdint.h>

#define PR_U3B0_0CC5_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u3b0_0cc5_cpu;

typedef enum {
    PR_U3B0_0CC5_OK = 0,
    PR_U3B0_0CC5_ARGUMENT,  /* null RAM/CPU or RAM that is not exactly 1 MiB; nothing changed */
    PR_U3B0_0CC5_CALLEE,    /* the composed U3B0 +0B03 model stopped (*callee_status = its status, *leaf_status = the
                               +0000 status it reports): its loop cap, +0000's RETF going elsewhere, or a relocated
                               entry whose +0000 call does not reach IP 0 */
    PR_U3B0_0CC5_TRANSFER   /* +0B03's RETF 4 popped a CS:IP other than the call's next instruction */
} pr_u3b0_0cc5_status;

/* Raw-CPU, full-state model of the complete GAME.OVR unit 0x3B0 routine
 * +0CC5..+0D87 (195 bytes of code, 68 instructions; the classifier's 213-byte
 * extent also holds 18 bytes of string data after RETF 4), a nested Pascal procedure
 * entered after a far call with SS:[SP] return IP, [SP+2] return CS, [SP+4]
 * the static link L (the parent's BP) and [SP+6] a byte d; RETF 4. After
 * PUSH BP; MOV BP,SP; SUB SP,8 it moves the index word X^ (X the far
 * pointer at L+0Ah) one step with wraparound, G being DS:[6B29h], lim the
 * word at L-59h and B the word at L-56h (all comparisons signed):
 *   d <> 0: X^ += 1; if G + lim - 1 < X^, X^ := G; if B - 1 < X^, X^ := G.
 *   d = 0:  X^ -= 1; if X^ < G, X^ := G + lim - 1; if B - 1 < X^,
 *           X^ := B - 1.
 * It then pushes AX (AL := d, AH from the last AX load) and L, and calls
 * +0B03 through PUSH CS; CALL (the strict raw-CPU pr_u3b0_step_scan_0b03_cpu,
 * which composes +0000 and receives max_loopbacks), then MOV SP,BP; POP BP;
 * RETF 4. Every operand is reread from RAM in source order and X is
 * reloaded by each LES, so an X^ that aliases the parent's words or this
 * frame is seen by later reads. +0B03 always executes CMP before it can
 * stop, so every flag written here is overwritten and folded; the final
 * flags are +0B03's. Offsets wrap at 16 bits, physical addresses at 1 MiB;
 * LES pairs, POP and this routine's RETF frame are read physically
 * contiguous. *callee_status and *leaf_status (either may be NULL) receive
 * +0B03's status and the last +0000 status it reports, or -1 when that
 * routine did not run. */
pr_u3b0_0cc5_status pr_u3b0_cursor_step_0cc5_cpu(uint8_t *ram, size_t ram_size, pr_u3b0_0cc5_cpu *cpu, unsigned max_loopbacks,
                                                 int *callee_status, int *leaf_status);

#endif
