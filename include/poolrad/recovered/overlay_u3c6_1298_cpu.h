#ifndef POOLRAD_RECOVERED_OVERLAY_U3C6_1298_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U3C6_1298_CPU_H

#include <stddef.h>
#include <stdint.h>

/* GAME.OVR unit 0x3C6 + 0x1298 (public entry 19, stub 0x3E0:0x7F), the whole
 * function [1298,12FB), 99 bytes, executed against live CPU and 1 MiB A20-off
 * RAM. Evidence: docs/re/U3C6_1298_RECORD.md.
 *
 * Enter at loaded CS:1298 with SS:SP on the far return IP and CS and 8
 * argument bytes above them: [bp+6] byte a, [bp+8] byte b, [bp+0xA] far
 * pointer Q. With R the far pointer at DS:0x6716 (re-read at every use) the
 * body stores R[4] = a and R[5] = [0x5F2A + 4 * entry15(Q)], calls entry 10
 * (+0996) with byte arguments (8, b, entry13(Q), entry12(Q)) pushed as words,
 * then stores R[4] = 0 and R[5] = 1. POP BP; RETF 8.
 *
 * Entries 15 (+0C5F, retf 4), 12 (+0BE7, retf 4) and 13 (+0C0F, retf 4) are
 * folded exactly from their original bytes and run on the same live state,
 * with their real pushes, frames and flags. Entry 10 (+0996, 354 bytes,
 * reaching U3CE entry 5 and resident 0x464:+1B52) is the one hook: it gets
 * the full CPU and RAM after the real pushes (four argument words, CS, return
 * IP 0x12E5) with CS:IP at the live CS:0996, and must model the whole callee,
 * its own RETF 8 included. Execution continues only at the pushed return; a
 * change to any modelled code byte by the hook stops the model. The loaded
 * bytes of [0BE7,0C37), [0C5F,0CB9) and [1298,12FB) must be original; every
 * write, including pushes, is checked against them and the model stops
 * before an aliasing byte.
 *
 * RETF policy: every RETF the model executes (entry 15 +0CB6, entry 12
 * +0C0C, entry 13 +0C34 and this body's +12F8) follows one caller-declared
 * policy with the values and meaning of the merged public U3C6 +0000 model
 * (overlay_u3c6_table_pair_0000.h/.c, PR #629), mirrored without including
 * that header:
 *   UNMEASURED  - fail closed: stop before the first RETF reached (entry 15's
 *                 +0CB6 on every run), after the preceding POP BP, with every
 *                 earlier effect kept (RETURN_PENDING, IP = that RETF);
 *   WRAPPED_SS  - synthetic: IP word at SS:SP, CS word at SS:(SP+2) with the
 *                 16-bit offset wrapped;
 *   CONTIGUOUS_PHYSICAL - synthetic: CS word two linear bytes above the IP word.
 * The hook's own RETF is the hook's business.
 *
 * Profile: LES takes the segment word at linear EA+2, minus 0x10000 when the
 * offset is above 0xFFFC (pinned DOSBox-X normal core 5a9c7c53); word
 * accesses are physically consecutive (high byte at linear+1, A20 off).
 * Undefined flags follow the same core's lazy flags (flags.cpp): AF = 0 after
 * XOR; after SHL by 1, AF = 1 and OF = MSB(result) XOR MSB(operand). The flags
 * of entry 13's SHL at +0C25 reach the hook unchanged. */

#define PR_U3C6_1298_CPU_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di, bp, sp, cs, ip, ds, es, ss, flags;
} pr_u3c6_1298_cpu;

/* Same values and meaning as pr_u3c6_0000_retf_policy in the public +0000 model */
typedef enum {
    PR_U3C6_1298_RETF_UNMEASURED = 0,
    PR_U3C6_1298_RETF_WRAPPED_SS,
    PR_U3C6_1298_RETF_CONTIGUOUS_PHYSICAL
} pr_u3c6_1298_retf_policy;

typedef struct {
    void *context;
    /* U3C6 entry 10 +0996 from its first instruction; 0 when it returned (by
     * any means) with the resulting state in cpu/ram */
    int (*entry10_0996)(void *context, uint8_t *ram, size_t ram_size, pr_u3c6_1298_cpu *cpu);
} pr_u3c6_1298_cpu_hooks;

typedef enum {
    PR_U3C6_1298_CPU_RETURNED = 0,     /* retf 8 executed; CS:IP is what the frame held */
    PR_U3C6_1298_CPU_ARGUMENT,         /* bad pointers/size/policy, or IP not 0x1298; nothing changed */
    PR_U3C6_1298_CPU_NEED_HOOK,        /* +0996 reached without a hook; stopped at CS:0996, pushes done */
    PR_U3C6_1298_CPU_CALLEE_FAILED,    /* the hook returned non-zero; state as it left it */
    PR_U3C6_1298_CPU_CALLEE_TRANSFER,  /* the hook returned to a CS:IP other than CS:12E5 */
    PR_U3C6_1298_CPU_CODE_CHANGED,     /* the hook changed a modelled code byte */
    PR_U3C6_1298_CPU_UNSCREENED_ALIAS, /* a write would alias a modelled code byte; IP names the
                                        * instruction, which has not executed */
    PR_U3C6_1298_CPU_RETURN_CHANGED,   /* a folded callee's RETF popped a CS:IP other than the one
                                        * pushed; CS:IP hold the popped values */
    PR_U3C6_1298_CPU_RETURN_PENDING    /* UNMEASURED: stopped before the first RETF reached, after
                                        * its POP BP; CS:IP = entry CS:that RETF (0CB6) */
} pr_u3c6_1298_cpu_status;

pr_u3c6_1298_cpu_status pr_u3c6_1298_cpu_run(uint8_t *ram, size_t ram_size, pr_u3c6_1298_cpu *cpu,
                                             pr_u3c6_1298_retf_policy retf_policy,
                                             const pr_u3c6_1298_cpu_hooks *hooks);

#endif
