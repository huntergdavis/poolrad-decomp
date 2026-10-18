#ifndef POOLRAD_RECOVERED_OVERLAY_U77_WEIGHTED_28BE_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U77_WEIGHTED_28BE_CPU_H

#include <stddef.h>
#include <stdint.h>

#define PR_U77_28BE_CPU_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u77_28be_cpu;

typedef enum {
    PR_U77_28BE_CPU_OK = 0,
    PR_U77_28BE_CPU_ARGUMENT,  /* null RAM/CPU or RAM that is not exactly 1 MiB; nothing changed */
    PR_U77_28BE_CPU_CODE,      /* a reached LCALL is not xxxx:0A5B or xxxx:0A76 as expected; stopped before it, IP at the site */
    PR_U77_28BE_CPU_CALLEE,    /* the divide stopped with DIVIDE_ERROR (*callee_status = 2): DS:[0D30h] = 0; the CPU is as the
                                  divide model leaves it, at its runtime-error target 00E2h */
    PR_U77_28BE_CPU_TRANSFER   /* a callee's RETF popped a CS:IP other than the call's next instruction */
} pr_u77_28be_cpu_status;

/* Raw-CPU, full-state model of the complete GAME.OVR unit 0x77 entry 11
 * routine +28BE..+2938 (123 bytes of code, 44 instructions; the 99 bytes
 * after RETF 4 are another entry's text data), entered after a far call
 * with SS:[SP] return IP, [SP+2] return CS and [SP+4]/[SP+6] a far record
 * pointer P; RETF 4. It is the raw-CPU counterpart of the provisional
 * value model pr_u77_weighted_28be. After PUSH BP; MOV BP,SP; SUB SP,0Ah,
 * with the LongInt sum at BP-8 := 0, for j := 0 to 4 (byte at BP-9, CBW):
 * LES DI,[BP+6] reloads P each pass; BX:CX := the word P^[88h + 2j]
 * zero-extended; DX:AX := w[j], the LongInt at DS:0D24h + 4j; the SYSTEM
 * LongInt multiply (LCALL xxxx:0A5B) and ADD/ADC to memory add the
 * product. Then DX:AX := (sum + 100) div DS:[0D30h] through the SYSTEM
 * LongInt divide (LCALL xxxx:0A76, signed, truncating), stored at BP-4 and
 * reloaded; MOV SP,BP; POP BP; RETF 4. The callees are the merged raw-CPU
 * pr_system_longmul_0a5b and pr_system_longdiv_0a76; each LCALL's segment
 * word is read from the code bytes (a fixup in the loaded image). Every
 * operand is reread from RAM in source order, so the record, the weights
 * and the divisor may alias the frame; j and the sum have no writer but
 * this routine, so the loop always runs five times. Flags: the final flags
 * are the divide's; at a CODE stop they are the second SHL DI,1's (AF
 * clear, as Unicorn leaves it) or ADC DX,0's for the two sites in order;
 * every other write is overwritten first and is folded. Offsets wrap at 16
 * bits, physical addresses at 1 MiB; the LES pairs, POP and this routine's
 * RETF frame are read physically contiguous, and each callee keeps its own
 * contract. *callee_status (may be NULL) receives the last callee's status,
 * or -1 when none ran. */
pr_u77_28be_cpu_status pr_u77_weighted_28be_cpu(uint8_t *ram, size_t ram_size, pr_u77_28be_cpu *cpu, int *callee_status);

#endif
