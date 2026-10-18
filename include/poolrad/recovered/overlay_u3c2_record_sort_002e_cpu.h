#ifndef POOLRAD_RECOVERED_OVERLAY_U3C2_RECORD_SORT_002E_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U3C2_RECORD_SORT_002E_CPU_H

#include <stddef.h>
#include <stdint.h>

#define PR_U3C2_002E_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u3c2_002e_cpu;

typedef enum {
    PR_U3C2_002E_OK = 0,
    PR_U3C2_002E_ARGUMENT,  /* null RAM/CPU or RAM that is not exactly 1 MiB; nothing changed */
    PR_U3C2_002E_CODE,      /* a reached LCALL is not xxxx:0A3F (9A 3F 0A); stopped before it, IP at the site */
    PR_U3C2_002E_TRANSFER,  /* Move's RETF 0Ah popped a CS:IP other than the call's next instruction */
    PR_U3C2_002E_LIMIT      /* host cap on taken loop-backs; IP at the INC the loop-back reaches */
} pr_u3c2_002e_status;

/* Raw-CPU, full-state model of the complete GAME.OVR unit 0x3C2 routine
 * +002E..+019F (370 bytes, 158 instructions), entered after a far call with
 * SS:[SP] return IP and [SP+2] return CS; plain RETF. It sorts the 3-byte
 * records R[k] = DS:(6718h + 3k), k = 1..n, n = DS:[671Ah] (which is also
 * R[0]'s third byte). After PUSH BP; MOV BP,SP; SUB SP,0Ah:
 *   if n <= 1, return. [BP-8] := n - 1; for i = 1 up to [BP-8] ([BP-1]):
 *     [BP-9] := n (reread); for j = i + 1 up to [BP-9] ([BP-2]; skipped
 *     when the byte i + 1 exceeds [BP-9]): [BP-4] := R[i][2],
 *     [BP-3] := R[j][2]; swap R[i] and R[j] when R[j][1] < R[i][1], or
 *     when R[j][1] = R[i][1], R[j][2] < R[i][2] and
 *     R[j][2] mod 2 <= R[i][2] mod 2 (IDIV CX, CX = 2; signed word JG).
 * A swap is three real far calls LCALL xxxx:0A3F read from RAM and run by
 * the raw-CPU pr_system_move_0a3f: R[i] -> SS:BP-7, R[j] -> R[i],
 * SS:BP-7 -> R[j], 3 bytes each. Every reread follows source order, so an
 * earlier record write is seen by later reads. The loops test equality on
 * byte counters, which terminate unless a record store aliases them;
 * max_loopbacks caps the taken JMP +0072 and JMP +0056 loop-backs, and the
 * next one stops at that INC (entry IP + 44h or + 28h) with LIMIT and
 * every earlier effect kept. The final flags are the last CMP's (DF clear
 * once a Move ran); dead INC, DEC, XOR, SHL, ADD and IDIV writes are
 * folded, except SUB SP,0Ah and the ADD DI,6718h before each call, which a
 * CODE stop exposes. Offsets wrap at 16 bits, physical addresses at 1 MiB;
 * POP and the RETF frames are read physically contiguous. *callee_status
 * (may be NULL) receives the last Move's status, or -1 when none ran. */
pr_u3c2_002e_status pr_u3c2_record_sort_002e_cpu(uint8_t *ram, size_t ram_size, pr_u3c2_002e_cpu *cpu, unsigned max_loopbacks,
                                                 int *callee_status);

#endif
