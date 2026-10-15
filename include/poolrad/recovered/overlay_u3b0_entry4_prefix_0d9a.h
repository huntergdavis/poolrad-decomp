#ifndef POOLRAD_RECOVERED_OVERLAY_U3B0_ENTRY4_PREFIX_0D9A_H
#define POOLRAD_RECOVERED_OVERLAY_U3B0_ENTRY4_PREFIX_0D9A_H

#include <stddef.h>
#include <stdint.h>

#define PR_U3B0_0D9A_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di, bp, sp, cs, ip, ds, es, ss, flags;
} pr_u3b0_0d9a_cpu;

typedef enum {
    PR_U3B0_0D9A_OK = 0,
    PR_U3B0_0D9A_ARGUMENT
} pr_u3b0_0d9a_status;

/* Execute U3B0 entry4 +0D9A..+0DAE; stop before the SYSTEM far call. */
pr_u3b0_0d9a_status pr_u3b0_entry4_prefix_0d9a(
    uint8_t *ram, size_t ram_size, pr_u3b0_0d9a_cpu *cpu);

#endif
