#ifndef POOLRAD_RECOVERED_OVERLAY_U5E_HALF_0E52_H
#define POOLRAD_RECOVERED_OVERLAY_U5E_HALF_0E52_H

#include <stddef.h>
#include <stdint.h>

#define PR_U5E_0E52_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u5e_0e52_cpu;

typedef enum {
    PR_U5E_0E52_OK = 0,
    PR_U5E_0E52_ARGUMENT
} pr_u5e_0e52_status;

/* Complete +0E52..+0E7A, entered after CALL. SS:SP contains return IP/CS,
 * followed by a word whose low byte is the argument. All accesses share
 * A20-off RAM, including saved BP, two local bytes, and DS:[6DE3].
 * For disjoint state, returns
 * floor(((argument + (DS:[6DE3] & 1)) modulo 256) / 2) in AX;
 * RETF 2 consumes the argument word. AF is undefined by x86 SHR and is
 * represented as zero. Rejects invalid host arguments before any effect. */
pr_u5e_0e52_status pr_u5e_half_0e52(
    uint8_t *ram, size_t ram_size, pr_u5e_0e52_cpu *cpu);

#endif
