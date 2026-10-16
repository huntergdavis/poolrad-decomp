#ifndef POOLRAD_RECOVERED_SYSTEM_SEED_FULLSTATE_120F_H
#define POOLRAD_RECOVERED_SYSTEM_SEED_FULLSTATE_120F_H

#include <stddef.h>
#include <stdint.h>

#define PR_SYSTEM_120F_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di, bp, sp, cs, ip, ds, es, ss, flags;
} pr_system_120f_cpu;

typedef enum {
    PR_SYSTEM_120F_RETURNED = 0,
    PR_SYSTEM_120F_ARGUMENT
} pr_system_120f_status;

/* Execute START.EXE SYSTEM +120F..+1244 at CS:120F on live A20-off RAM.
 * The near RET takes its IP from live SS:SP after the ordered DS seed writes.
 * Loaded CS:120F..1244 instruction bytes must be the pinned original body.
 * DS seed bytes overlapping those instruction bytes are rejected unchanged.
 * No overlay dispatch or caller's +11C0 wrapper is included. */
pr_system_120f_status pr_system_seed_fullstate_120f(
    uint8_t *ram, size_t ram_size, pr_system_120f_cpu *cpu);

#endif
