#ifndef POOLRAD_RECOVERED_OVERLAY_U3B0_ENTRY4_NONNULL_0DEA_H
#define POOLRAD_RECOVERED_OVERLAY_U3B0_ENTRY4_NONNULL_0DEA_H

#include <stddef.h>
#include <stdint.h>

#define PR_U3B0_0DEA_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di, bp, sp, cs, ip, ds, es, ss, flags;
} pr_u3b0_0dea_cpu;

typedef enum {
    PR_U3B0_0DEA_OK = 0,
    PR_U3B0_0DEA_ARGUMENT
} pr_u3b0_0dea_status;

/* Execute original entry4 +0DEA..+0DFE; stop before +0DFF caller. */
pr_u3b0_0dea_status pr_u3b0_entry4_nonnull_0dea(
    uint8_t *ram, size_t ram_size, pr_u3b0_0dea_cpu *cpu);

#endif
