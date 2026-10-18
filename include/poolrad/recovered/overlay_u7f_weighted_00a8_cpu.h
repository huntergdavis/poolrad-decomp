#ifndef POOLRAD_RECOVERED_OVERLAY_U7F_WEIGHTED_00A8_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U7F_WEIGHTED_00A8_CPU_H

#include <stddef.h>
#include <stdint.h>

#define PR_U7F_00A8_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u7f_00a8_cpu;

typedef enum {
    PR_U7F_00A8_OK = 0,
    PR_U7F_00A8_ARGUMENT,  /* null RAM/CPU or RAM that is not exactly 1 MiB; nothing changed */
    PR_U7F_00A8_CODE,      /* a reached LCALL is not xxxx:0A3F, xxxx:0A5B or xxxx:0A76 as expected; stopped before it, IP at the site */
    PR_U7F_00A8_CALLEE,    /* the divide stopped with DIVIDE_ERROR (*callee_status = 2): DS:[0D30h] = 0; the CPU is as the
                              divide model leaves it, at its runtime-error target 00E2h */
    PR_U7F_00A8_TRANSFER   /* a callee's RETF popped a CS:IP other than the call's next instruction */
} pr_u7f_00a8_status;

/* Raw-CPU, full-state model of the complete GAME.OVR unit 0x7F routine
 * +00A8..+012F (136 bytes, 51 instructions), entered after a far call with
 * SS:[SP] return IP, [SP+2] return CS and [SP+4]/[SP+6] a far pointer P;
 * RETF 4. After PUSH BP; MOV BP,SP; SUB SP,26h it copies 28 bytes from P^
 * to the local LongInt array v[0..6] at BP-20h through the SYSTEM Move
 * (LCALL xxxx:0A3F). With the LongInt sum at BP-24h := 0, for j := 0 to 4
 * (byte at BP-25h, CBW): sum += w[j] * v[j], w[j] the LongInt at DS:0D24h +
 * 4j, through the SYSTEM LongInt multiply (LCALL xxxx:0A5B) and ADD/ADC to
 * memory. Then DX:AX := (sum + 100) div DS:[0D30h] through the SYSTEM
 * LongInt divide (LCALL xxxx:0A76, signed, truncating), stored at BP-4 and
 * reloaded; MOV SP,BP; POP BP; RETF 4. The callees are the merged raw-CPU
 * pr_system_move_0a3f, pr_system_longmul_0a5b and pr_system_longdiv_0a76;
 * each LCALL's segment word is read from the code bytes (a fixup in the
 * loaded image). Every operand is reread from RAM in source order, so w,
 * v and the divisor may alias the frame; j and the sum have no writer but
 * this routine, so the loop always runs five times. Flags: the final flags
 * are the divide's; at a CODE stop they are SUB SP,26h's, the second SHL
 * DI,1's (AF clear, as Unicorn leaves it) or ADC DX,0's for the three
 * sites in order; every other write is overwritten first and is folded.
 * Offsets wrap at 16 bits, physical addresses at 1 MiB; the LES pair, POP
 * and this routine's RETF frame are read physically contiguous, and each
 * callee keeps its own contract. *callee_status (may be NULL) receives the
 * last callee's status, or -1 when none ran. */
pr_u7f_00a8_status pr_u7f_weighted_00a8_cpu(uint8_t *ram, size_t ram_size, pr_u7f_00a8_cpu *cpu, int *callee_status);

#endif
