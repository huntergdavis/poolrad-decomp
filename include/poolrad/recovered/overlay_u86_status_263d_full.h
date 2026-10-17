#ifndef POOLRAD_RECOVERED_OVERLAY_U86_STATUS_263D_FULL_H
#define POOLRAD_RECOVERED_OVERLAY_U86_STATUS_263D_FULL_H

#include <stddef.h>
#include <stdint.h>

/* GAME.OVR unit 0x86 + 0x263D (public entry slot 47), the complete 45-byte
 * body [263D,266A), 25 instructions, as a full-state model: registers, FLAGS
 * and 1 MiB of real-mode RAM, in source order. Its CS literal pool
 * [2631,263D) is a Pascal string of length 0x0B (sha256 294632af...829d);
 * the model only passes its address and never reads or embeds the text.
 * Evidence: docs/re/U86_STATUS_263D_FULL.md.
 *
 * Body: push bp; mov bp,sp; sub sp,0xC; push the byte DS:[0x681B] and four
 * zero bytes (AL loads, AH carried); the runtime string load 0xAF8:0x0B0B of
 * CS:0x2631 into SS:[bp-0xC] (its pointer stays pushed); push cs; call +08C3
 * (a unit-local routine ending in retf 0xE); mov sp,bp; pop bp; retf.
 *
 * Entry: the caller has transferred to CS:+263D and SS:SP is at the far
 * return address; no arguments. The entry IP is neither read nor checked;
 * IP is first written at the lcall. Preconditions, each failing closed:
 * - STACK_BOUNDARY: every stack word the model pushes or pops, including the
 *   IP and CS words of both modelled RETFs (the +08C3 retf 0xE and the body's
 *   retf), lies in [SP0 - 0x24, SP0 + 4); SP0 must keep that window from
 *   wrapping the SS offset (0x24 <= SP0 <= 0xFFFC). Wrapped-SS and offset
 *   0xFFFF straddle cases are UNMEASURED and not modelled. Inside the window
 *   word accesses are contiguous; physical addresses wrap at 1 MiB (8086).
 * - CODE_ALIAS: the window must not physically overlap the body
 *   [CS:263D, CS:266A), and after each hook every body byte other than the
 *   live call operands (CS:+265E..+2661, +2664..+2665) must equal its value
 *   at entry; self-modified code and the prefetch queue are not modelled. */

#define PR_U86_263D_FULL_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u86_263d_full_cpu;

typedef enum {
    PR_U86_263D_FULL_0B0B = 0,           /* lcall 0xAF8:0x0B0B at +265D; the callee ends in retf 4 */
    PR_U86_263D_FULL_08C3                /* push cs; call +08C3 at +2662/+2663; the callee ends in retf 0xE */
} pr_u86_263d_full_call;

/* Call hook. The call operands are read live from RAM at CS, in source
 * order, so code the earlier pushes alias or a host patches is honoured. The
 * lcall is raw: the model reads the far target from CS:+265E (offset) and
 * CS:+2660 (segment) (originally 0x0B0B and 0x0AF8, a loader fixup), pushes
 * the caller CS and the return IP (+2662) and enters that target; the hook
 * sees that entry state and runs the callee including its own retf 4; the
 * model then requires caller CS:+2662. The near-far call: after `push cs`
 * the model reads the displacement at CS:+2664 (originally 0xE25D) and adds
 * it to the return IP +2666 (16-bit), pushes that return IP and enters the
 * target (originally +08C3); the hook sees SS:SP = return IP, CS, the
 * arguments, supplies the callee's effects except its return, and the model
 * performs its retf 0xE from the live stack. A hook may patch the live
 * operands (the far return flushes the queue, so the near displacement is
 * read after it). A foreign return is TRANSFER, as is a broken stack
 * contract: after the lcall hook SS must be kept and SP must be its entry
 * SP + 8 (retf 4); after the +08C3 hook SS:SP must still be at the return IP;
 * after the model's retf 0xE SP must be BP - 0xC (all 18 pushed bytes
 * consumed, BP kept). Returning 0 aborts with the CPU as the hook left it. */
typedef int (*pr_u86_263d_full_external)(void *context, uint8_t *ram, size_t ram_size,
                                         pr_u86_263d_full_cpu *cpu, pr_u86_263d_full_call call);

typedef enum {
    PR_U86_263D_FULL_OK = 0,
    PR_U86_263D_FULL_ARGUMENT,           /* null RAM or CPU, RAM under 1 MiB, or a null hook */
    PR_U86_263D_FULL_CALL_ABORT,         /* a hook returned 0; the CPU is as that hook left it (entered at the callee) */
    PR_U86_263D_FULL_TRANSFER,           /* a callee returned somewhere other than its caller CS:return IP, or broke its SS/SP/BP contract */
    PR_U86_263D_FULL_STACK_BOUNDARY,     /* SP0 outside [0x24, 0xFFFC]: a stack word would wrap the SS offset; nothing executed */
    PR_U86_263D_FULL_CODE_ALIAS          /* the stack window overlaps the body (nothing executed), or a hook changed a non-operand body byte */
} pr_u86_263d_full_status;

pr_u86_263d_full_status pr_u86_status_263d_full(
    uint8_t *ram, size_t ram_size, pr_u86_263d_full_cpu *cpu,
    pr_u86_263d_full_external external, void *context);

#endif
