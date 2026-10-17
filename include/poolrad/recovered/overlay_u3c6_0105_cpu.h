#ifndef POOLRAD_RECOVERED_OVERLAY_U3C6_0105_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U3C6_0105_CPU_H

#include <stddef.h>
#include <stdint.h>

/* GAME.OVR unit 0x3C6 + 0x0105 (public entry 3, stub 0x3E0:0x2F), the whole
 * function [0105,0127), 34 bytes, executed against live CPU and 1 MiB A20-off
 * RAM. Evidence: docs/re/U3C6_0105_PALETTE.md.
 *
 * Enter at loaded CS:0105 with SS:SP on the far return IP and CS (no
 * arguments). When the byte DS:[0x4A1E] equals 1, the body calls resident
 * 0x464:+2A0D twice, with byte pairs (0, 0) and (8, 8) pushed as words (AH
 * carried); otherwise it returns at once. POP BP; RETF.
 *
 * Resident 0x464:+2A0D (61 bytes) is folded exactly from its decoded START.EXE
 * body at the live, relocated call segment: it writes DS:[0x4A03] = 0x10,
 * [0x4A02] = 0, [0x4A05] = value (value + 8 when value >= 8) and [0x4A04] =
 * register, pushes AX (AL = 0x10) and the far pointer DS:0x4A02, and calls
 * the DOS-unit Intr routine 0xA32:+000B, which is the one external hook.
 * The hook receives the full CPU and RAM after that call's pushes, with CS:IP
 * at the live target; execution continues only at the pushed return
 * (0x464 segment : 0x2A46). The loaded entry and 0x2A0D bodies must be
 * original; every write of this model is alias-checked against them, and a
 * callee change to either body stops the model. */

#define PR_U3C6_0105_CPU_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di, bp, sp, cs, ip, ds, es, ss, flags;
} pr_u3c6_0105_cpu;

typedef struct {
    void *context;
    /* 0xA32:+000B Intr(int_no word, var Registers) from its first instruction;
     * 0 when it returned (by any means) with the resulting state in cpu/ram */
    int (*dos_intr_a32_000b)(void *context, uint8_t *ram, size_t ram_size, pr_u3c6_0105_cpu *cpu);
} pr_u3c6_0105_cpu_hooks;

typedef enum {
    PR_U3C6_0105_CPU_RETURNED = 0,     /* retf executed; CS:IP is the caller's return */
    PR_U3C6_0105_CPU_ARGUMENT,         /* bad pointers/size, IP not 0x0105, or a live call operand not original */
    PR_U3C6_0105_CPU_NEED_HOOK,        /* Intr reached without a hook; stopped at 0x464:+2A41, arguments pushed */
    PR_U3C6_0105_CPU_CALLEE_FAILED,    /* the hook returned non-zero; state as it left it */
    PR_U3C6_0105_CPU_CALLEE_TRANSFER,  /* the hook returned to a CS:IP other than 0x464:2A46 */
    PR_U3C6_0105_CPU_CODE_CHANGED,     /* the callee changed a byte of the entry or 0x2A0D body */
    PR_U3C6_0105_CPU_SYSTEM_TRANSFER,  /* the folded 0x2A0D returned to a CS:IP other than the pushed one */
    PR_U3C6_0105_CPU_UNSCREENED_ALIAS  /* a write would alias the entry or 0x2A0D body; stopped before it */
} pr_u3c6_0105_cpu_status;

pr_u3c6_0105_cpu_status pr_u3c6_0105_cpu_run(
    uint8_t *ram, size_t ram_size, pr_u3c6_0105_cpu *cpu, const pr_u3c6_0105_cpu_hooks *hooks);

#endif
