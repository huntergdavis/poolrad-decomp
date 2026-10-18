#ifndef POOLRAD_RECOVERED_OVERLAY_U31_CANDIDATE_CHECK_0255_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U31_CANDIDATE_CHECK_0255_CPU_H

#include <stddef.h>
#include <stdint.h>

#define PR_U31_0255_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u31_0255_cpu;

typedef enum {
    PR_U31_0255_NO_CALLEE = -1,
    PR_U31_0255_U3C2 = 0,        /* U3C2 entry 6 (+0909), the merged pr_u3c2_caller_0909 */
    PR_U31_0255_RESIDENT         /* the resident zero test 00BA:23B7, translated here */
} pr_u31_0255_callee;

typedef enum {
    PR_U31_0255_OK = 0,
    PR_U31_0255_ARGUMENT,  /* null RAM/CPU, RAM that is not exactly 1 MiB, a zero U3C2 budget, or an entry SP whose U3C2
                              entry SP (SP - 18h) lies outside the [008Eh, FFEEh] the U3C2 model accepts; nothing changed */
    PR_U31_0255_CODE,      /* a reached LCALL is not in its expected form (stopped at the site, nothing pushed), or the
                              03DB:003E stub it reaches is not the loaded EA jump to U3C2 +0909, a cold INT 3Fh stub
                              included (stopped at the stub, after the call's pushes) */
    PR_U31_0255_CALLEE,    /* the U3C2 model stopped (a budget, its CALLEE or its TRANSFER); *callee_status holds it */
    PR_U31_0255_TRANSFER   /* U3C2's RETF 0Eh or the resident routine's RETF 4 popped a CS:IP other than the call's
                              next instruction */
} pr_u31_0255_status;

/* Raw-CPU, full-state model of the complete GAME.OVR unit 0x31 routine
 * +0255..+02EA (150 bytes, 58 instructions), entered after a far call with
 * SS:[SP] the return IP and [SP+2] the return CS; four argument words
 * follow (the first unused); RETF 8. After PUSH BP; MOV BP,SP; SUB SP,4 and
 * flag := 0 at BP-1:
 *  1. It pushes the seven words of U3C2 entry 6: the low bytes of [BP+0Ch]
 *     and [BP+0Ah] (AH kept), the byte DS:[3209h + 16 * the low byte of
 *     [BP+8]] (AH := 0), 00FFh, 0001h, DS:[6718h] and DS:[6716h], and
 *     calls LCALL 03DB:003E, whose loaded stub EA jumps to +0909. That is
 *     the merged pr_u3c2_caller_0909, given pair_cs and its three budgets;
 *     it leaves its candidate count at DS:671Ah and 3-byte records after it.
 *  2. count := DS:[671Ah] at BP-3. For i := 1 to count (i at BP-2; no pass
 *     when count is 0): PUSH the active character DS:[5D94h]/[5D92h] and
 *     LCALL 00BA:23B7, the resident test that leaves AL = 1 when the
 *     record's byte +10Eh is zero and 0 otherwise; DL := AL. The candidate
 *     record pointer is DS:[65B9h + 4 * DS:[6718h + 3i]]; when its byte
 *     +10Eh differs from DL, flag := 1.
 *  3. AL := flag; MOV SP,BP; POP BP; RETF 8.
 * The resident routine (13 instructions: PUSH BP; MOV BP,SP; SUB SP,2; LES
 * DI,[BP+6]; CMP BYTE ES:[DI+10Eh],0; its local [BP-1] := 1 or 0; AL :=
 * that local; MOV SP,BP; POP BP; RETF 4) is translated here in full state,
 * because its merged host adapter pr_rec_res_00ba_23b7 carries no CPU state.
 * Each LCALL's operands are read before its pushes, as the CPU fetches
 * them; the stub is read after them, when the CPU reaches it. Every
 * operand is reread from RAM in source order, so the list, the pointer
 * table, the records and the frame may alias. The loop needs no cap: i
 * and count are written only by INC BYTE [BP-2], since the pushes and the
 * resident frame stay below BP-4, so it makes exactly count passes. U3C2's
 * preconditions (immutable loaded code and pair binding, writes that avoid
 * code) and budgets are its own, and its stops are CALLEE. Flags: the final
 * flags are the last CMP's (i against count, equal, or 1 against a zero
 * count); at the U3C2 site and stub they are XOR AH,AH's; at the resident
 * site they are CMP 1,count's on the first pass and INC BYTE's later (CF
 * from the CMP before it); after the resident routine they are its CMP
 * BYTE ...,0's. Offsets wrap at 16 bits, physical addresses at 1 MiB; LES
 * pairs, POP and every RETF frame are read physically contiguous.
 * *callee and *callee_status (either may be NULL) receive the last callee
 * run and its status (0 for the resident routine), or NO_CALLEE and -1. */
pr_u31_0255_status pr_u31_candidate_check_0255_cpu(uint8_t *ram, size_t ram_size, pr_u31_0255_cpu *cpu, uint16_t pair_cs,
                                                   size_t max_caller_iterations, size_t max_line_iterations,
                                                   unsigned max_sort_loopbacks, pr_u31_0255_callee *callee, int *callee_status);

#endif
