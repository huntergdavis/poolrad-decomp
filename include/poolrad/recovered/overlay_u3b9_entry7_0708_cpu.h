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
 * resident 0x464:+1B52.
 *
 * Hook policy, declared by the caller in the hooks (no hooks = UNMEASURED):
 *   HOOK_UNMEASURED - fail closed at the first hook lcall reached, with the
 *                     caller's argument pushes done (HOOK_PENDING);
 *   HOOK_CONTRACT   - synthetic: the model performs the far CALL's return
 *                     push, sets CS:IP to the live target and hands the full
 *                     CPU and RAM to the hook. A hook returns 0 when the
 *                     callee returned, non-zero for a callee that does not
 *                     return to this caller or a host failure. Execution
 *                     continues only on the contract read from each original
 *                     callee body: back at the pushed return CS:IP, SS kept,
 *                     SP = its entry SP + 4 + n, BP kept, plus
 *                       0x464:+00FF  n = 0,    DS kept   (retf at +010B)
 *                       SYSTEM +0919 n = 6,    DS kept, the Text variable
 *                                              pointer left on the stack for
 *                                              +084A unchanged (retf 6, +0954)
 *                       SYSTEM +084A n = 4                (retf 4, +086B)
 *                       0x464:+0D71  n = 0x10             (retf 0x10, +1481)
 *                       0x464:+1B52  n = 0                (retf, +1E5B)
 *                     and 0x899:+0000, whose body reaches SYSTEM Halt (+00E9)
 *                     before its POP BP/RETF, must not return
 *                     (CALLEE_NONRETURNING when its hook returns 0).
 *                     With those, SP = BP - 0x46 at +07BA on every path. A run
 *                     through a hook is a modelled subset with unknown
 *                     provenance: no claim on RAM below the hook's SP, DOS,
 *                     video or hardware effects, or register effects beyond
 *                     the contract.
 *
 * Code policy, declared by the caller:
 *   CODE_PINNED    - before anything runs, every byte of [06E9,07C0) (the
 *                    private literal pool and this body) and of the folded
 *                    SYSTEM [0A3F,0A57) at the live segment must match the
 *                    original (64-bit FNV-1a pins, skipping only the seven
 *                    lcall segment words the GAME.OVR fixup table lists in
 *                    that range); the fixup words must name one SYSTEM, one
 *                    0x464 and one 0x899 segment with SYSTEM - 0x464 = 0x0694
 *                    and 0x899 - 0x464 = 0x0435 (one load base), else
 *                    CODE_NOT_ORIGINAL with nothing changed;
 *   CODE_SYNTHETIC - SYNTHETIC TESTS ONLY: just the seven 9A opcode/offset
 *                    pairs are checked; every other instruction byte is
 *                    unverified. No parity meaning.
 *
 * LES/LDS: all six (+070E, +076C, +0775, +0781 and the folded +0A43 LDS and
 * +0A47 LES) have SS operands. An operand offset in FFFD..FFFF is the
 * unmeasured profile seam: the model stops before that instruction with IP
 * at its offset and every earlier effect kept (UNSUPPORTED_LES_SEAM).
 * Elsewhere the segment word is SS:(offset + 2). +0781 re-reads the +0775
 * operand with BP unchanged, so its seam is reached only at +0775.
 *
 * RETF policy: both RETFs (the folded retf 0xA at SYSTEM +0A54, always
 * reached first, and this body's retf 6 at +07BD) follow one caller-declared
 * policy with the values and meaning of the merged public U3C6 +0000 model
 * (PR #629), mirrored without including that header:
 *   UNMEASURED  - fail closed: stop before the first RETF reached (+0A54),
 *                 with every earlier effect kept (RETURN_PENDING);
 *   WRAPPED_SS  - synthetic: IP word at SS:SP, CS word at SS:(SP+2) with the
 *                 16-bit offset wrapped;
 *   CONTIGUOUS_PHYSICAL - synthetic: CS word two linear bytes above the IP word.
 * Declared synthetic word rule: every push, pop and stack word, including the
 * return-IP word, is physically contiguous (high byte at linear+1, A20 off),
 * also across SS:FFFF.
 *
 * Architecturally undefined flags follow the pinned DOSBox-X lazy flags
 * (flags.cpp, revision 5a9c7c53): after SHL DI,3, AF = 1 and OF =
 * MSB(result) XOR MSB(operand); after XOR/OR, AF = 0. The flags of the third
 * SHL (+079C) reach the 0x464:+0D71 hook unchanged. */

#define PR_U3B9_0708_CPU_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di, bp, sp, cs, ip, ds, es, ss, flags;
} pr_u3b9_0708_cpu;

typedef enum {
    PR_U3B9_0708_CODE_PINNED = 0,
    PR_U3B9_0708_CODE_SYNTHETIC              /* synthetic tests only: call sites checked, no parity */
} pr_u3b9_0708_code_policy;

typedef enum {
    PR_U3B9_0708_HOOK_UNMEASURED = 0,
    PR_U3B9_0708_HOOK_CONTRACT
} pr_u3b9_0708_hook_policy;

/* Same values and meaning as pr_u3c6_0000_retf_policy in the public +0000 model */
typedef enum {
    PR_U3B9_0708_RETF_UNMEASURED = 0,
    PR_U3B9_0708_RETF_WRAPPED_SS,
    PR_U3B9_0708_RETF_CONTIGUOUS_PHYSICAL
} pr_u3b9_0708_retf_policy;

typedef int (*pr_u3b9_0708_cpu_callee)(void *context, uint8_t *ram, size_t ram_size, pr_u3b9_0708_cpu *cpu);

typedef struct {
    void *context;
    pr_u3b9_0708_hook_policy hook_policy;
    pr_u3b9_0708_cpu_callee resident_0464_00ff;   /* +072D */
    pr_u3b9_0708_cpu_callee system_0919;          /* +073F Write(text, string : 0) */
    pr_u3b9_0708_cpu_callee system_084a;          /* +0744 Writeln(text) */
    pr_u3b9_0708_cpu_callee resident_0899_0000;   /* +0749 */
    pr_u3b9_0708_cpu_callee resident_0464_0d71;   /* +07AC */
    pr_u3b9_0708_cpu_callee resident_0464_1b52;   /* +07B5 */
} pr_u3b9_0708_cpu_hooks;

typedef enum {
    PR_U3B9_0708_CPU_RETURNED = 0,     /* retf 6 executed; CS:IP is the caller's return */
    PR_U3B9_0708_CPU_ARGUMENT,         /* bad pointers/size/policy, IP not 0x0708, (SYNTHETIC) a call site
                                          not original, or a write alias; nothing changed */
    PR_U3B9_0708_CPU_NEED_HOOK,        /* HOOK_CONTRACT and a reached hook is NULL; stopped at that lcall,
                                          arguments pushed */
    PR_U3B9_0708_CPU_CALLEE_FAILED,    /* a hook returned non-zero; state as the hook left it */
    PR_U3B9_0708_CPU_CALLEE_TRANSFER,  /* a hook returned to a CS:IP other than the pushed return */
    PR_U3B9_0708_CPU_CODE_CHANGED,     /* a callee changed a byte of the folded body or [06E9,07C0) */
    PR_U3B9_0708_CPU_SYSTEM_TRANSFER,  /* the folded copy's live return address was not the pushed one */
    PR_U3B9_0708_CPU_UNSCREENED_ALIAS, /* after a callee, a write still ahead would alias a folded body */
    PR_U3B9_0708_CPU_RETURN_PENDING,   /* UNMEASURED: stopped before the folded retf 0xA; CS:IP = SYSTEM:0A54 */
    PR_U3B9_0708_CPU_UNSUPPORTED_LES_SEAM, /* an SS LES/LDS operand at FFFD..FFFF; stopped before it,
                                           * IP at its offset, earlier effects kept */
    PR_U3B9_0708_CPU_HOOK_PENDING,     /* HOOK_UNMEASURED: stopped at the first hook lcall, arguments pushed */
    PR_U3B9_0708_CPU_CALLEE_FRAME,     /* a hook returned with SS, SP, BP, DS or the kept Text variable
                                          pointer off its contract; state as the hook left it */
    PR_U3B9_0708_CPU_CALLEE_NONRETURNING, /* the 0x899:0000 hook returned 0; state as it left it */
    PR_U3B9_0708_CPU_CODE_NOT_ORIGINAL /* PINNED: a code byte or fixup relation not original; nothing changed */
} pr_u3b9_0708_cpu_status;

pr_u3b9_0708_cpu_status pr_u3b9_0708_cpu_run(
    uint8_t *ram, size_t ram_size, pr_u3b9_0708_cpu *cpu, pr_u3b9_0708_retf_policy retf_policy,
    pr_u3b9_0708_code_policy code_policy, const pr_u3b9_0708_cpu_hooks *hooks);

#endif
