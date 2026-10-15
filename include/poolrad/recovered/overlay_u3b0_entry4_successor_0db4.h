#ifndef POOLRAD_RECOVERED_OVERLAY_U3B0_ENTRY4_SUCCESSOR_0DB4_H
#define POOLRAD_RECOVERED_OVERLAY_U3B0_ENTRY4_SUCCESSOR_0DB4_H

#include <stddef.h>
#include <stdint.h>

#define PR_U3B0_0DB4_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di, bp, sp, cs, ip, ds, es, ss, flags;
} pr_u3b0_0db4_cpu;

typedef enum {
    PR_U3B0_0DB4_OK = 0,
    PR_U3B0_0DB4_ARGUMENT
} pr_u3b0_0db4_status;

/* Execute original U3B0 +0DB4..+0DC1 after the first copy returns. */
pr_u3b0_0db4_status pr_u3b0_entry4_successor_0db4(
    uint8_t *ram, size_t ram_size, pr_u3b0_0db4_cpu *cpu);

#endif
