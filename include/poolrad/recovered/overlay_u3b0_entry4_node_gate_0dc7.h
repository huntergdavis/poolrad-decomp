#ifndef POOLRAD_RECOVERED_OVERLAY_U3B0_ENTRY4_NODE_GATE_0DC7_H
#define POOLRAD_RECOVERED_OVERLAY_U3B0_ENTRY4_NODE_GATE_0DC7_H

#include <stddef.h>
#include <stdint.h>

#define PR_U3B0_0DC7_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di, bp, sp, cs, ip, ds, es, ss, flags;
} pr_u3b0_0dc7_cpu;

typedef enum {
    PR_U3B0_0DC7_OK = 0,
    PR_U3B0_0DC7_ARGUMENT
} pr_u3b0_0dc7_status;

/* Execute +0DC7..+0DCE after the second SYSTEM copy returns. */
pr_u3b0_0dc7_status pr_u3b0_entry4_node_gate_0dc7(
    uint8_t *ram, size_t ram_size, pr_u3b0_0dc7_cpu *cpu);

#endif
