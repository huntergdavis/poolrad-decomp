#ifndef POOLRAD_RECOVERED_OVERLAY_U3C6_0105_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U3C6_0105_CPU_H

#include <stddef.h>
#include <stdint.h>

/* GAME.OVR unit 0x3C6 + 0x0105 (public entry 3, stub 0x3E0:0x2F), the whole
 * function [0105,0127), 34 bytes, executed against live CPU and 1 MiB A20-off
 * RAM. Evidence: docs/re/U3C6_0105_PALETTE.md.
 *
 * Code policy, declared by the caller:
 *   CODE_PINNED    - before anything runs, every byte of the entry body
 *                    [0105,0127), resident [2A0D,2A4A) at the live lcall
 *                    segment and Intr [000B,006C) at the live segment read
 *                    from 2A44 must match the original (64-bit FNV-1a pins of
 *                    the bytes with the three loader-fixup segment words
 *                    skipped); the fixup words must satisfy [0118] == [0123]
 *                    and [2A44] - [0118] == 0x0A32 - 0x0464 (one load base),
 *                    else CODE_NOT_ORIGINAL with nothing changed;
 *   CODE_SYNTHETIC - SYNTHETIC TESTS ONLY: just the two 9A 0D 2A lcalls with
 *                    one segment and 9A 0B 00 at 2A41 are checked; every other
 *                    instruction byte is unverified and the model runs the
 *                    original semantics regardless. No parity meaning.
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
 *
 * Intr policy, declared by the caller in the hooks (no hooks = UNMEASURED):
 *   INTR_UNMEASURED    - fail closed at 0x464:+2A41 before the far call, with
 *                        Intr's three argument words pushed (INTR_PENDING);
 *   INTR_HOOK_BOUNDED  - at +2A41, before the far call, the live argument
 *                        words must be DI = 0x4A02, the current DS and int
 *                        0x10 (low byte), and the record DS:[0x4A02,0x4A16)
 *                        must miss SS:[S,S+10), the bytes the call and its
 *                        arguments own, else INTR_FRAME. Then the caller
 *                        declares the interrupt handler's stack
 *                        depth D. With S the SP at Intr's first instruction
 *                        (entry SP - 0x16 on both calls), Intr writes its own
 *                        scratch SS:[S-20,S) and the handler, entered at
 *                        S-10, may write below that, so the model first checks
 *                        that the record misses all three code ranges and
 *                        that no byte of SS:[S-20-D,S) (16-bit offsets wrapped,
 *                        A20 off) is in a code range, the record
 *                        DS:[0x4A02,0x4A16), the vector IVT[0x10] at
 *                        0000:0040..0043 or the live frame SS:[S, entry SP + 4),
 *                        and otherwise stops at +2A41 with nothing changed
 *                        (INTR_STACK_ALIAS).
 * The hook receives the full CPU and RAM after that call's pushes, with CS:IP
 * at the live target; execution continues only at the pushed return
 * (0x464 segment : 0x2A46), with SS unchanged and SP = its entry SP + 10 (its
 * RETF 6 pops the return frame and both arguments), and only with the CPU
 * outputs Intr's original tail (0xA32:+0045..+006B) leaves: DS and BP as at
 * the call, ES = DS, DI = 0x4A16, AX = the record's Flags word DS:[0x4A14],
 * FLAGS = AX with DF clear, BX CX DX SI = the record words at DS:[0x4A04],
 * [0x4A06], [0x4A08], [0x4A0C]. Anything else stops the model. The hook's
 * read effects (the stale record inputs DS:[0x4A06..0x4A13], the interrupt
 * number word on its frame, the vector IVT[0x10]) and its RAM writes (the
 * record, SS:[S-20-D,S), any BIOS data, hardware or other handler effect) are
 * the hook's provenance and are not checked. A run through a hook is a
 * modelled subset with that provenance unknown: there is no claim on RAM below
 * S after Intr, on BIOS/BDA/hardware effects, or on handler register effects
 * beyond the relations above. Every write of this model is alias-checked
 * against the three code ranges, and a callee change to any of them stops the
 * model.
 *
 * RETF policy: both RETFs the model executes (the folded RETF 4 at 0x464:+2A47
 * and this body's RETF at +0126) follow one caller-declared policy with the
 * values and meaning of the merged public U3C6 +0000 model (PR #629), as in
 * U3C6 +0CB9 (PR #833) and +04BE (PR #837), mirrored without including that
 * header:
 *   UNMEASURED  - fail closed: stop before the first RETF reached (+2A47 when
 *                 the gate is taken, else +0126), after the preceding POP BP,
 *                 with every earlier effect kept (RETURN_PENDING);
 *   WRAPPED_SS  - synthetic: IP word at SS:SP, CS word at SS:(SP+2) with the
 *                 16-bit offset wrapped;
 *   CONTIGUOUS_PHYSICAL - synthetic: CS word two linear bytes above the IP word.
 * The Intr hook's own RETF is the hook's business. Declared synthetic word
 * rule: every push and pop word, including the return-IP word, is physically
 * contiguous (high byte at linear+1, A20 off), also across SS:FFFF. There is
 * no LES or LDS. */

#define PR_U3C6_0105_CPU_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di, bp, sp, cs, ip, ds, es, ss, flags;
} pr_u3c6_0105_cpu;

typedef enum {
    PR_U3C6_0105_CODE_PINNED = 0,
    PR_U3C6_0105_CODE_SYNTHETIC              /* synthetic tests only: call sites checked, no parity */
} pr_u3c6_0105_code_policy;

/* Same values and meaning as pr_u3c6_0000_retf_policy in the public +0000 model */
typedef enum {
    PR_U3C6_0105_RETF_UNMEASURED = 0,
    PR_U3C6_0105_RETF_WRAPPED_SS,
    PR_U3C6_0105_RETF_CONTIGUOUS_PHYSICAL
} pr_u3c6_0105_retf_policy;

typedef enum {
    PR_U3C6_0105_INTR_UNMEASURED = 0,
    PR_U3C6_0105_INTR_HOOK_BOUNDED
} pr_u3c6_0105_intr_policy;

#define PR_U3C6_0105_BIOS_DEPTH_MAX 0xFFEBu  /* 20 + D bytes fit below S in one segment */

typedef struct {
    void *context;
    pr_u3c6_0105_intr_policy intr_policy;
    uint16_t bios_depth;                     /* D, at most PR_U3C6_0105_BIOS_DEPTH_MAX */
    /* 0xA32:+000B Intr(int_no word, var Registers) from its first instruction,
     * including the interrupt and its own RETF 6; 0 when it returned (by any
     * means) with the resulting state in cpu/ram */
    int (*dos_intr_a32_000b)(void *context, uint8_t *ram, size_t ram_size, pr_u3c6_0105_cpu *cpu);
} pr_u3c6_0105_cpu_hooks;

typedef enum {
    PR_U3C6_0105_CPU_RETURNED = 0,     /* retf executed; CS:IP is what the frame held */
    PR_U3C6_0105_CPU_ARGUMENT,         /* bad pointers/size/policy/depth, IP not 0x0105, or (SYNTHETIC) a
                                        * call site not original; nothing changed */
    PR_U3C6_0105_CPU_NEED_HOOK,        /* INTR_HOOK_BOUNDED with no hook function; stopped at 0x464:+2A41,
                                        * arguments pushed */
    PR_U3C6_0105_CPU_CALLEE_FAILED,    /* the hook returned non-zero; state as it left it */
    PR_U3C6_0105_CPU_CALLEE_TRANSFER,  /* the hook returned to a CS:IP other than 0x464:2A46, or with
                                        * another SS or SP != its entry SP + 10 */
    PR_U3C6_0105_CPU_CODE_CHANGED,     /* the callee changed a byte of the entry, 0x2A0D or Intr code */
    PR_U3C6_0105_CPU_SYSTEM_TRANSFER,  /* the folded 0x2A0D returned to a CS:IP other than the pushed one */
    PR_U3C6_0105_CPU_UNSCREENED_ALIAS, /* a write would alias a code range; stopped before it */
    PR_U3C6_0105_CPU_RETURN_PENDING,   /* UNMEASURED: stopped before the first RETF reached, after its
                                        * POP BP; CS:IP = that RETF (0x464:2A47 or entry CS:0126) */
    PR_U3C6_0105_CPU_CALLEE_OUTPUT,    /* the hook's CPU outputs are not those of Intr's tail (see above);
                                        * state as it left it, at 0x464:2A46 */
    PR_U3C6_0105_CPU_INTR_PENDING,     /* INTR_UNMEASURED: stopped at 0x464:+2A41, arguments pushed */
    PR_U3C6_0105_CPU_INTR_STACK_ALIAS, /* the record meets a code range, or SS:[S-20-D,S) meets a code range,
                                        * the record, IVT[0x10] or the live frame; stopped at 0x464:+2A41,
                                        * arguments pushed */
    PR_U3C6_0105_CPU_INTR_FRAME,       /* the live Intr arguments are not DS:0x4A02 / int 0x10, or the record
                                        * meets SS:[S,S+10); stopped at 0x464:+2A41, arguments pushed */
    PR_U3C6_0105_CPU_CODE_NOT_ORIGINAL /* PINNED: a code byte or fixup relation not original; nothing changed */
} pr_u3c6_0105_cpu_status;

pr_u3c6_0105_cpu_status pr_u3c6_0105_cpu_run(
    uint8_t *ram, size_t ram_size, pr_u3c6_0105_cpu *cpu, pr_u3c6_0105_retf_policy retf_policy,
    pr_u3c6_0105_code_policy code_policy, const pr_u3c6_0105_cpu_hooks *hooks);

#endif
