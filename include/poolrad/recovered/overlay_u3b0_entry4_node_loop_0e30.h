#ifndef POOLRAD_RECOVERED_OVERLAY_U3B0_ENTRY4_NODE_LOOP_0E30_H
#define POOLRAD_RECOVERED_OVERLAY_U3B0_ENTRY4_NODE_LOOP_0E30_H

#include "poolrad/recovered/overlay_u3b0_entry4_nonnull_0dea.h"

typedef enum {
    PR_U3B0_0E30_OK = 0,
    PR_U3B0_0E30_ARGUMENT,
    PR_U3B0_0E30_LIMIT
} pr_u3b0_0e30_status;

/* Execute +0E30..+0E5E until +0E5F, with a finite traversal bound. */
pr_u3b0_0e30_status pr_u3b0_entry4_node_loop_0e30(
    uint8_t *ram, size_t ram_size, pr_u3b0_0dea_cpu *cpu,
    size_t max_traversals);

#endif
