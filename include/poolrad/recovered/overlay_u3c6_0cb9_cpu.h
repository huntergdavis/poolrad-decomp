#ifndef POOLRAD_RECOVERED_OVERLAY_U3C6_0CB9_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U3C6_0CB9_CPU_H

#include <stddef.h>
#include <stdint.h>

/* GAME.OVR unit 0x3C6 + 0x0CB9 (public entry 16, stub 0x3E0:0x70), the whole
 * function [0CB9,0E11), 344 bytes, executed against live CPU and 1 MiB A20-off
 * RAM. Evidence: docs/re/U3C6_0CB9_SCAN.md.
 *
 * Enter at loaded CS:0CB9 with SS:SP on the far return IP and CS and 14
 * argument bytes above them: [bp+6] far byte pointer P6, [bp+0xA] far byte
 * pointer P10, [bp+0xE] byte k, [bp+0x10] far pointer Q. The body stores
 * *P10 = 0 and *P6 = 0x17, finds Q's slot s through entry 15, then for
 * i = 0..3 asks entry 1 for an offset pair and entry 6 for the two cell bytes
 * at that position, and updates *P10 / *P6 from them. MOV SP,BP; POP BP;
 * RETF 0xE.
 *
 * The three near callees (`push cs; call near`) are folded exactly from their
 * original bytes and run on the same live state, with their real pushes,
 * frames, flags and RETFs: entry 15 (+0C5F, retf 4), entry 1 (+0000,
 * retf 0xC) and entry 6 (+04BE, retf 0xC). No other call is reachable, so the
 * model has no hook. The loaded bytes of [0000,0068), [04BE,052B) and
 * [0C5F,0E11) must be original; every write, including pushes, is checked
 * against them and the model stops before an aliasing byte. Every pointer and
 * table read is live (P6/P10 are re-read through LES at each original site).
 *
 * Profile: LES takes the segment word at linear EA+2, minus 0x10000 when the
 * offset is above 0xFFFC (pinned DOSBox-X normal core); word accesses are
 * physically consecutive (high byte at linear+1, A20 off). Undefined flags
 * follow the same core's lazy flags (flags.cpp, instructions.h at 5a9c7c53):
 * AF = 0 after XOR/OR; after SHL by n != 0, AF = 1 and OF = MSB(result)
 * XOR MSB(operand); MUL sets ZF = (AX == 0) and PF from all 32 product bits
 * and keeps SF and AF.
 *
 * Every RETF the model executes (entry 15 +0CB6, entry 1 +0065, entry 6
 * +0528 and this body's +0E0E) follows one caller-declared policy, with the
 * values and meaning of the merged public U3C6 +0000 model
 * (overlay_u3c6_table_pair_0000.h/.c, PR #629), mirrored here without
 * including that header:
 *   UNMEASURED  - fail closed: stop before the first RETF reached (entry 15's
 *                 +0CB6 on every run), after the preceding POP BP, with every
 *                 earlier effect kept (status RETURN_PENDING, IP = that RETF).
 *   WRAPPED_SS  - synthetic: return IP word at SS:SP, CS word at SS:(SP+2) with
 *                 the 16-bit offset wrapped.
 *   CONTIGUOUS_PHYSICAL - synthetic: CS word at the linear address two bytes
 *                 above the IP word, without the offset wrap.
 * Both synthetic profiles are declared candidates, not native facts; they
 * differ only when a return IP word sits at SS:FFFE or SS:FFFF (at SS:FFFD
 * both read CS at SS:FFFF).
 *
 * Folded public +0000 LES seam: that model reads the segment word of its
 * LES [bp+6] / [bp+0xA] at linear EA+2, where the pinned profile above reads
 * EA+2-0x10000 for offsets FFFD..FFFF. When either operand would fall there
 * the model stops before the transfer at +0D29 (UNSUPPORTED_LES_SEAM) instead
 * of choosing one reading. */

#define PR_U3C6_0CB9_CPU_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di, bp, sp, cs, ip, ds, es, ss, flags;
} pr_u3c6_0cb9_cpu;

/* Same values and meaning as pr_u3c6_0000_retf_policy in the public +0000 model */
typedef enum {
    PR_U3C6_0CB9_RETF_UNMEASURED = 0,
    PR_U3C6_0CB9_RETF_WRAPPED_SS,
    PR_U3C6_0CB9_RETF_CONTIGUOUS_PHYSICAL
} pr_u3c6_0cb9_retf_policy;

typedef enum {
    PR_U3C6_0CB9_CPU_RETURNED = 0,     /* retf 0xE executed; CS:IP is what the frame held */
    PR_U3C6_0CB9_CPU_ARGUMENT,         /* bad pointers/size/policy, or IP not 0x0CB9; nothing changed */
    PR_U3C6_0CB9_CPU_UNSCREENED_ALIAS, /* a write would alias a modelled code byte; IP names the
                                        * instruction, which has not executed */
    PR_U3C6_0CB9_CPU_RETURN_CHANGED,   /* a folded callee's RETF popped a CS:IP other than the
                                        * one pushed; CS:IP hold the popped values */
    PR_U3C6_0CB9_CPU_RETURN_PENDING,   /* UNMEASURED: stopped before the first RETF reached, after
                                        * its POP BP; CS:IP = entry CS:that RETF (0CB6) */
    PR_U3C6_0CB9_CPU_UNSUPPORTED_LES_SEAM /* entry 1's LES operand would sit at SS:FFFD..FFFF,
                                        * where the public +0000 and pinned readings differ;
                                        * stopped before +0D29 PUSH CS */
} pr_u3c6_0cb9_cpu_status;

pr_u3c6_0cb9_cpu_status pr_u3c6_0cb9_cpu_run(uint8_t *ram, size_t ram_size, pr_u3c6_0cb9_cpu *cpu,
                                             pr_u3c6_0cb9_retf_policy retf_policy);

#endif
