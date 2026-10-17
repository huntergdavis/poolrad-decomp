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
 * an aliasing write stops before it. MUL's architecturally undefined SF, ZF,
 * AF and PF are left unchanged (CF = OF = DX != 0); both MULs are followed by
 * an ADD that rewrites all arithmetic flags before any observation. LES follows
 * the pinned DOSBox-X normal-core profile: the segment word is read at linear
 * EA+2, minus 0x10000 when the offset is above 0xFFFC. An aliasing store stops
 * with IP at that store's offset. */

#define PR_U3C6_04BE_CPU_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di, bp, sp, cs, ip, ds, es, ss, flags;
} pr_u3c6_04be_cpu;

typedef enum {
    PR_U3C6_04BE_CPU_RETURNED = 0,     /* retf 0xC executed; CS:IP is the caller's return */
    PR_U3C6_04BE_CPU_ARGUMENT,         /* bad pointers/size or IP not 0x04BE; nothing changed */
    PR_U3C6_04BE_CPU_UNSCREENED_ALIAS  /* a write would alias the loaded body; stopped before it */
} pr_u3c6_04be_cpu_status;

pr_u3c6_04be_cpu_status pr_u3c6_04be_cpu_run(uint8_t *ram, size_t ram_size, pr_u3c6_04be_cpu *cpu);

#endif
