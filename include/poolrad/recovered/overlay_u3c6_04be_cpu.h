#ifndef POOLRAD_RECOVERED_OVERLAY_U3C6_04BE_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U3C6_04BE_CPU_H

#include <stddef.h>
#include <stdint.h>

/* GAME.OVR unit 0x3C6 + 0x04BE (public entry 6, stub 0x3E0:0x3E), the whole
 * leaf function [04BE,052B), 109 bytes, executed against live CPU and 1 MiB
 * A20-off RAM. Evidence: docs/re/U3C6_04BE_CELL.md.
 *
 * Enter at loaded CS:04BE with SS:SP on the far return IP, CS and 12 argument
 * bytes: [BP+6] far pointer A, [BP+0xA] far pointer B, [BP+0xE] byte y and
 * [BP+0x10] byte x (signed; high bytes unread). If x is outside 0..0x31 or y
 * outside 0..0x18, the function stores 0 through B and then through A.
 * Otherwise, with i = y * 0x32 + x (CBW and 16-bit MUL), it stores the byte
 * at (far DS:[0x6716]) + i + 7 through A and the byte DS:[0x60DB + i] through
 * B. Then POP BP; RETF 0xC. There are no calls.
 *
 * Every frame/DS read and both stores go through RAM in original order. The
 * stores and the BP push are alias-checked against the loaded body [04BE,052B);
 * an aliasing write stops before it. MUL follows the pinned 386-type normal
 * core (instructions.h MULW): ZF = (AX == 0), PF from all 32 product bits,
 * CF = OF = DX != 0, SF and AF kept; both MULs are followed by an ADD that
 * rewrites all arithmetic flags before any write or return. LES reads the
 * offset word at EA and the segment word at EA+2. The fixed-offset LES of
 * DS:0x6716 never reaches the FFFD..FFFF seam; for the stack operands [BP+6]
 * and [BP+0xA] (+04F4, +0511, +0519, +0520) an offset above 0xFFFC is an
 * unmeasured profile seam, and the model stops before that LES with IP at its
 * offset and every earlier effect kept (UNSUPPORTED_LES_SEAM). An aliasing
 * store stops with IP at that store's offset.
 *
 * Declared synthetic word rule: the PUSH BP, POP BP and return-IP words are
 * read and written physically contiguous (high byte at linear+1, A20 off),
 * also across SS:FFFF; this is a profile assumption, not a measured fact.
 *
 * The RETF 0xC at +0528 follows one caller-declared policy with the values and
 * meaning of the merged public U3C6 +0000 model (PR #629), as in U3C6 +0CB9
 * (PR #833), mirrored without including that header:
 *   UNMEASURED  - fail closed: stop at +0528 after POP BP, before the RETF,
 *                 with both stores kept (status RETURN_PENDING);
 *   WRAPPED_SS  - synthetic: IP word at SS:SP, CS word at SS:(SP+2) with the
 *                 16-bit offset wrapped;
 *   CONTIGUOUS_PHYSICAL - synthetic: CS word two linear bytes above the IP word.
 * The two synthetic profiles differ only when the return IP word sits at
 * SS:FFFE or SS:FFFF (at SS:FFFD both read CS at SS:FFFF). */

#define PR_U3C6_04BE_CPU_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di, bp, sp, cs, ip, ds, es, ss, flags;
} pr_u3c6_04be_cpu;

/* Same values and meaning as pr_u3c6_0000_retf_policy in the public +0000 model */
typedef enum {
    PR_U3C6_04BE_RETF_UNMEASURED = 0,
    PR_U3C6_04BE_RETF_WRAPPED_SS,
    PR_U3C6_04BE_RETF_CONTIGUOUS_PHYSICAL
} pr_u3c6_04be_retf_policy;

typedef enum {
    PR_U3C6_04BE_CPU_RETURNED = 0,     /* retf 0xC executed; CS:IP is what the frame held */
    PR_U3C6_04BE_CPU_ARGUMENT,         /* bad pointers/size/policy or IP not 0x04BE; nothing changed */
    PR_U3C6_04BE_CPU_UNSCREENED_ALIAS, /* a write would alias the loaded body; stopped before it */
    PR_U3C6_04BE_CPU_RETURN_PENDING,   /* UNMEASURED: stopped at +0528 after POP BP; CS:IP = entry CS:0528 */
    PR_U3C6_04BE_CPU_UNSUPPORTED_LES_SEAM /* a stack LES operand at SS:FFFD..FFFF; stopped before
                                        * that LES (IP 04F4/0511/0519/0520), earlier effects kept */
} pr_u3c6_04be_cpu_status;

pr_u3c6_04be_cpu_status pr_u3c6_04be_cpu_run(uint8_t *ram, size_t ram_size, pr_u3c6_04be_cpu *cpu,
                                             pr_u3c6_04be_retf_policy retf_policy);

#endif
