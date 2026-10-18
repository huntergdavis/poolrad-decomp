#ifndef POOLRAD_RECOVERED_OVERLAY_U31_TABLE_VALUE_128D_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U31_TABLE_VALUE_128D_CPU_H

#include <stddef.h>
#include <stdint.h>

#define PR_U31_128D_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u31_128d_cpu;

typedef enum {
    PR_U31_128D_OK = 0,
    PR_U31_128D_ARGUMENT,  /* null RAM/CPU or RAM that is not exactly 1 MiB; nothing changed */
    PR_U31_128D_CODE,      /* the reached LCALL is not xxxx:0A3F (9A 3F 0A); stopped before it, IP at the site */
    PR_U31_128D_TRANSFER   /* Move's RETF 0Ah popped a CS:IP other than the call's next instruction; cannot occur with
                              the modeled Move, which writes only the 16-byte local above its own frame */
} pr_u31_128d_status;

/* Raw-CPU, full-state model of the complete GAME.OVR unit 0x31 routine
 * +128D..+13C9 (317 bytes, 115 instructions), entered after a far call with
 * SS:[SP] return IP, [SP+2] return CS, [SP+4]/[SP+6] a far pointer P and
 * [SP+8]/[SP+0Ah] a far pointer Q; RETF 8. With BP = SP-2 after PUSH BP:
 *   SUB SP,12h; c := P^[2Eh]; Move(DS:(5582h + 16c), SS:BP-12h, 16) through
 *   the real far call LCALL xxxx:0A3F read from RAM, run by the raw-CPU
 *   pr_system_move_0a3f; then with L = the 16 copied bytes (L[k] = SS:[BP-12h+k])
 *   v := L[9] * L[10] (MUL; low byte kept);
 *   if P^[32h] > 0 (signed): v += 8 * P^[32h];  if L[11] > 0: v += 2 * L[11];
 *   if c = 55h and the word pair at R^[0Ah]/[0Ch] is nonzero, R = Q^[108h],
 *     and T^[76h] > 0 for T = R^[0Ah]: v := 8;
 *   if L[14] AND 8: v += 2 * (L[5] - 1);  if L[1] <= 1: v += 3;
 *   if Q^[100h] + L[1] > 3 (signed word compare): v := 0;
 *   if P^[3Eh] = 84h and (P^[3Dh] AND 0Fh) <> Q^[0A0h]: v := 0;
 *   if P^[3Dh] = 53h: v := 0;  if P^[36h] <> 0: v := 0;
 *   [BP-1] := v; AL := [BP-1]; MOV SP,BP; POP BP; RETF 8.
 * All sums keep only the low byte in [BP-2]. The final flags are
 * CMP P^[36h],0's with DF clear from Move's CLD; the other arithmetic flag
 * writes (XOR, SHL, ADD, MUL, DEC, AND, OR) are overwritten before any read
 * and are folded, except SUB SP,12h and ADD DI,5582h, whose flags a CODE
 * stop exposes. Every operand is reread from RAM in source order; offsets
 * wrap at 16 bits and physical addresses at 1 MiB; LES pairs, POP and the
 * RETF frames are read physically contiguous. *callee_status (may be NULL)
 * receives Move's status, or -1 when it did not run. */
pr_u31_128d_status pr_u31_table_value_128d_cpu(uint8_t *ram, size_t ram_size, pr_u31_128d_cpu *cpu, int *callee_status);

#endif
