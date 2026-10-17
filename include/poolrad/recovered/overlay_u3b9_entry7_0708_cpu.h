#ifndef POOLRAD_RECOVERED_OVERLAY_U3B9_ENTRY7_0708_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U3B9_ENTRY7_0708_CPU_H

#include <stddef.h>
#include <stdint.h>

/* GAME.OVR unit 0x3B9 + 0x0708 (public entry 7, stub 0x3D0:0x43), the whole
 * function [0708,07C0) executed against live CPU and 1 MiB A20-off RAM.
 * Evidence: docs/re/U3B9_ENTRY7_0708_SPRITE.md (CPU model section).
 *
 * Enter at loaded CS:0708 with SS:SP on the far return IP, CS and the
 * arguments ([BP+6] far record pointer, [BP+0xA] index word). Every push,
 * frame/local re-read (including indexes above 8, which read whatever stack
 * RAM holds), far-pointer read and DS read goes through RAM.
 *
 * SYSTEM +0A3F (the 0x42-byte value-parameter copy) is folded exactly from
 * its decoded START.EXE body at the live, relocated call segment. Six callees
 * are external hooks: resident 0x464:+00FF, SYSTEM +0919 (Write string),
 * SYSTEM +084A (Writeln), resident 0x899:+0000, resident 0x464:+0D71 and
 * resident 0x464:+1B52. For each, the model performs the caller's pushes and
 * the far CALL's return push, sets CS:IP to the live target and hands the
 * full CPU and RAM to the hook; a hook returns 0 when the callee returned,
 * and execution continues only at the pushed return CS:IP. A hook returns
 * non-zero for a callee that does not return to this caller (for example
 * program termination) or a host failure.
 *
 * Architecturally undefined flags are written by convention: OF after
 * SHL DI,3 as MSB(result) XOR CF, AF after SHL and after XOR/OR as 0. The
 * flags of the third SHL reach the 0x464:+0D71 hook unchanged. */

#define PR_U3B9_0708_CPU_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di, bp, sp, cs, ip, ds, es, ss, flags;
} pr_u3b9_0708_cpu;

typedef int (*pr_u3b9_0708_cpu_callee)(void *context, uint8_t *ram, size_t ram_size, pr_u3b9_0708_cpu *cpu);

typedef struct {
    void *context;
    pr_u3b9_0708_cpu_callee resident_0464_00ff;   /* +072D */
    pr_u3b9_0708_cpu_callee system_0919;          /* +073F Write(text, string : 0) */
    pr_u3b9_0708_cpu_callee system_084a;          /* +0744 Writeln(text) */
    pr_u3b9_0708_cpu_callee resident_0899_0000;   /* +0749 */
    pr_u3b9_0708_cpu_callee resident_0464_0d71;   /* +07AC */
    pr_u3b9_0708_cpu_callee resident_0464_1b52;   /* +07B5 */
} pr_u3b9_0708_cpu_hooks;

typedef enum {
    PR_U3B9_0708_CPU_RETURNED = 0,     /* retf 6 executed; CS:IP is the caller's return */
    PR_U3B9_0708_CPU_ARGUMENT,         /* bad pointers/size, IP not 0x0708, a live call operand offset
                                          not original, or a write alias; nothing changed */
    PR_U3B9_0708_CPU_NEED_HOOK,        /* a reached hook is NULL; stopped at that lcall, arguments pushed */
    PR_U3B9_0708_CPU_CALLEE_FAILED,    /* a hook returned non-zero; state as the hook left it */
    PR_U3B9_0708_CPU_CALLEE_TRANSFER,  /* a hook returned to a CS:IP other than the pushed return */
    PR_U3B9_0708_CPU_CODE_CHANGED,     /* a callee changed a byte of the folded body or [06E9,07C0) */
    PR_U3B9_0708_CPU_SYSTEM_TRANSFER,  /* the folded copy's live return address was not the pushed one */
    PR_U3B9_0708_CPU_UNSCREENED_ALIAS  /* after a callee, a write still ahead would alias a folded body */
} pr_u3b9_0708_cpu_status;

pr_u3b9_0708_cpu_status pr_u3b9_0708_cpu_run(
    uint8_t *ram, size_t ram_size, pr_u3b9_0708_cpu *cpu, const pr_u3b9_0708_cpu_hooks *hooks);

#endif
