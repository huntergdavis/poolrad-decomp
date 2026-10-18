#ifndef POOLRAD_RECOVERED_OVERLAY_U3B0_STEP_SCAN_0B03_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U3B0_STEP_SCAN_0B03_CPU_H

#include <stddef.h>
#include <stdint.h>

#define PR_U3B0_0B03_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u3b0_0b03_cpu;

typedef enum {
    PR_U3B0_0B03_OK = 0,
    PR_U3B0_0B03_ARGUMENT,  /* null RAM/CPU or RAM that is not exactly 1 MiB; nothing changed */
    PR_U3B0_0B03_CALLEE,    /* U3B0 +0000's model rejected its entry (*callee_status = its status): the near CALL reaches
                               IP entry - 0B03h, which is 0 only when the routine is entered at its own offset */
    PR_U3B0_0B03_TRANSFER,  /* +0000's RETF 6 popped a CS:IP other than the call's next instruction */
    PR_U3B0_0B03_LIMIT      /* host cap on taken loop-backs; IP at the loop head the JMP reaches */
} pr_u3b0_0b03_status;

/* Raw-CPU, full-state model of the complete GAME.OVR unit 0x3B0 routine
 * +0B03..+0C32 (304 bytes, 103 instructions), a nested Pascal procedure
 * entered after a far call with SS:[SP] return IP, [SP+2] return CS,
 * [SP+4] the static link L (the parent's BP) and [SP+6] a byte d; RETF 4.
 * It reads the parent's frame through SS:[L+disp]: a limit word at L-59h,
 * a word at L-56h, a far pointer X at L+0Ah to an index word, and a list
 * head far pointer at L+14h/16h. With count c := 0 in [BP-2]:
 *   d <> 0: while c < limit (signed): node := +0000(head, X^) through
 *     PUSH CS; CALL +0000 (the strict raw-CPU pr_u3b0_entry1_0000, DX:AX);
 *     stop when node^[29h] = 0; c += 1; X^ += 1; if DS:[6B29] + limit - 1
 *     < X^, X^ := DS:[6B29]; if L[-56h] - 1 < X^, X^ := DS:[6B29].
 *   d = 0: the same walk with X^ -= 1; if X^ < DS:[6B29], X^ := DS:[6B29] +
 *     limit - 1; if L[-56h] - 1 < X^, X^ := L[-56h] - 1.
 * Every operand is reread from RAM in source order, so an index store that
 * aliases the counter, the limit or the parent's frame is seen by later
 * reads; such aliasing can make the original loop forever, so
 * max_loopbacks caps the taken JMP +0B17 / JMP +0B9F and the next one stops
 * at that loop head with LIMIT and every earlier effect kept. The final
 * flags are the last CMP's; INC, DEC, ADD and XOR writes are overwritten
 * before any stop and are folded. Offsets wrap at 16 bits, physical
 * addresses at 1 MiB; LES pairs, POP and this routine's RETF frame are read
 * physically contiguous, and +0000 keeps its own contract (its RETF pops
 * wrap per word). *callee_status (may be NULL) receives the last +0000
 * status, or -1 when it did not run. */
pr_u3b0_0b03_status pr_u3b0_step_scan_0b03_cpu(uint8_t *ram, size_t ram_size, pr_u3b0_0b03_cpu *cpu, unsigned max_loopbacks,
                                               int *callee_status);

#endif
