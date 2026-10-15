#ifndef POOLRAD_RECOVERED_OVERLAY_U3C2_SIGN_ENTRY1_0000_H
#define POOLRAD_RECOVERED_OVERLAY_U3C2_SIGN_ENTRY1_0000_H

#include <stddef.h>
#include <stdint.h>

#define PR_U3C2_0000_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di, bp, sp, cs, ip, ds, es, ss, flags;
} pr_u3c2_0000_cpu;

typedef enum {
    PR_U3C2_0000_OK = 0,
    PR_U3C2_0000_ARGUMENT
} pr_u3c2_0000_status;

/* Execute complete U3C2 entry 1 +0000..+002D through RETF 2. */
pr_u3c2_0000_status pr_u3c2_sign_entry1_0000(
    uint8_t *ram, size_t ram_size, pr_u3c2_0000_cpu *cpu);

#endif
