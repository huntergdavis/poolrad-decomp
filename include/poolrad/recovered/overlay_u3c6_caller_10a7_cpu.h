#ifndef POOLRAD_RECOVERED_OVERLAY_U3C6_CALLER_10A7_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U3C6_CALLER_10A7_CPU_H

#include <stddef.h>
#include <stdint.h>

#define PR_U3C6_10A7_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u3c6_10a7_cpu;

typedef enum {
    PR_U3C6_10A7_NO_CALLEE = -1,
    PR_U3C6_10A7_ENTRY15 = 0,   /* +0C5F slot search, folded here */
    PR_U3C6_10A7_ENTRY16,       /* +0CB9, the merged pr_u3c6_0cb9_cpu_run (WRAPPED_SS) */
    PR_U3C6_10A7_FILLCHAR,      /* SYSTEM 16A8, the merged pr_system_fillchar_16a8 */
    PR_U3C6_10A7_ENTRY5         /* +03A0, the merged pr_u3c6_rebuild_03a0 */
} pr_u3c6_10a7_callee;

typedef enum {
    PR_U3C6_10A7_OK = 0,
    PR_U3C6_10A7_ARGUMENT,  /* null RAM/CPU, RAM that is not exactly 1 MiB, IP not 10A7h, SP outside [0100h, FFF0h], or a
                               zero +03A0 budget; nothing changed */
    PR_U3C6_10A7_CODE,      /* the reached FillChar LCALL is not 9A A8 16; stopped at the site, nothing pushed */
    PR_U3C6_10A7_CALLEE,    /* +0CB9 or +03A0 stopped; *callee_status holds its status */
    PR_U3C6_10A7_TRANSFER,  /* a callee's RETF popped a CS:IP other than the call's next instruction */
    PR_U3C6_10A7_LIMIT      /* host cap on the first record loop's taken JNE +117F; IP at +117F */
} pr_u3c6_10a7_status;

/* Raw-CPU, full-state model of the complete GAME.OVR unit 0x3C6 routine
 * +10A7..+1297 (497 bytes, 191 instructions), entered after a near CALL
 * preceded by PUSH CS or after a far call: SS:[SP] return IP, [SP+2] return
 * CS, then [SP+4] flag byte F, [SP+6] Y, [SP+8] X (low bytes) and
 * [SP+0Ah]/[SP+0Ch] a record far pointer Q; RETF 0Ah. After PUSH BP; MOV
 * BP,SP; SUB SP,8:
 *  1. Unless DS:[49F3h] = 5, the result is 1 at once.
 *  2. s := entry 15 (+0C5F, Q's slot in the DS:65B9h pointer table, 0 when
 *     absent; folded here in full state from its 32 original instructions).
 *     Slot s gets type Q^[6Ch] & 7Fh at DS:5F2Ah + 4s, X at 5F27h + 4s and Y
 *     at 5F28h + 4s.
 *  3. Entry 16 (+0CB9, given Q, 8 and pointers to the locals c at BP-4 and v
 *     at BP-5) scans the cells around the slot. When c is nonzero, or v is
 *     zero, or DS:[2744h + 4v] = FFh, the slot's type is cleared and the
 *     result is 0 (no +03A0 call).
 *  4. Otherwise the result is 1. When F is nonzero and the byte +13h of the
 *     record at Q^[108h] is zero: for k := 1 to DS:[6715h] (loop 1), each
 *     7-byte entry DS:66D6h + 7k whose far pointer equals Q is cleared by
 *     FillChar, after its byte +6 (unless 1Fh) is kept at BP-5; then for
 *     k := 1 to DS:[6715h] (loop 2) an entry with a nonzero pointer and
 *     coordinates (X, Y) at +4/+5 sets a seen flag. If none was seen, the
 *     map byte at (DS:6716h far pointer) + 7 + 50Y + X becomes the byte at
 *     BP-5. Then entry 5 (+03A0) rebuilds the occupancy arrays.
 *  5. AL := result; MOV SP,BP; POP BP; RETF 0Ah.
 * The near callees sit at their original offsets in this CS, so the entry
 * must be +10A7. +0CB9 runs with the WRAPPED_SS policy and +03A0 with
 * max_slots and max_cells and the SYSTEM segment read from its FillChar
 * site operand (CS:03B5h); each keeps its own contract and profile (code
 * and bindings immutable, writes that avoid code). SP in [0100h, FFF0h]
 * keeps every far frame of the chain unwrapped, where WRAPPED_SS and
 * physically contiguous frame reads agree, and +03A0's SP in its accepted
 * range. Every operand is reread from RAM in source order. Loop 1's
 * FillChar can clear its own counter through DS = SS aliases, so
 * max_loopbacks caps its taken JNE +117F, and the next one stops there with
 * LIMIT; loop 2 writes only its seen flag, so it makes exactly DS:[6715h]
 * passes. Flags: the final flags are +03A0's on the paths through it,
 * CMP [49F3h],5's when that byte is not 5, and on the slot-clearing path
 * those of SHL DI,1 (DI := 4s), whose undefined AF is set as the pinned
 * DOSBox-X core sets it (the +0CB9 profile); at the FillChar site they are
 * ADD DI,66D6h's, and at LIMIT loop 1's last CMP. Offsets wrap at 16 bits,
 * physical addresses at 1 MiB; LES pairs, POP and this routine's RETF frames
 * are read physically contiguous. *callee and *callee_status (either may be
 * NULL) receive the last callee run and its status (0 for the folded entry
 * 15), or NO_CALLEE and -1. */
pr_u3c6_10a7_status pr_u3c6_caller_10a7_cpu(uint8_t *ram, size_t ram_size, pr_u3c6_10a7_cpu *cpu, unsigned max_loopbacks,
                                            size_t max_slots, size_t max_cells, pr_u3c6_10a7_callee *callee, int *callee_status);

#endif
