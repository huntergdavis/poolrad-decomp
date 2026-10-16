#ifndef POOLRAD_RECOVERED_OVERLAY_U9E_ENTRY8_FULLSTATE_PREFIX_0DE3_H
#define POOLRAD_RECOVERED_OVERLAY_U9E_ENTRY8_FULLSTATE_PREFIX_0DE3_H

#include <stddef.h>
#include <stdint.h>

#define PR_U9E_ENTRY8_PREFIX_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di, bp, sp, cs, ip, ds, es, ss, flags;
} pr_u9e_entry8_prefix_cpu;

typedef enum {
    PR_U9E_ENTRY8_PREFIX_AT_CALL = 0,
    PR_U9E_ENTRY8_PREFIX_AT_TAIL,
    PR_U9E_ENTRY8_PREFIX_ARGUMENT
} pr_u9e_entry8_prefix_status;

/* Execute original GAME.OVR U9E entry8 +0DE3 up to the first transfer:
 * nonzero count stops before the nested LCALL at +0E0E, and zero count
 * stops at the shared +0E1F tail. The loaded CS:+0DE3..+0E2D bytes must
 * be original. No SYSTEM call or U9E return-side instruction is executed. */
pr_u9e_entry8_prefix_status pr_u9e_entry8_fullstate_prefix_0de3(
    uint8_t *ram, size_t ram_size, pr_u9e_entry8_prefix_cpu *cpu);

#endif
