#ifndef POOLRAD_RECOVERED_OVERLAY_U3B0_ENTRY4_NODE_COUNT_0E5F_H
#define POOLRAD_RECOVERED_OVERLAY_U3B0_ENTRY4_NODE_COUNT_0E5F_H

#include "poolrad/recovered/overlay_u3b0_entry4_nonnull_0dea.h"

typedef enum {
    PR_U3B0_0E5F_OK = 0,
    PR_U3B0_0E5F_ARGUMENT,
    PR_U3B0_0E5F_LIMIT
} pr_u3b0_0e5f_status;

/* Execute original +0E5F..+0E96 to +0E97, with a finite link bound. */
pr_u3b0_0e5f_status pr_u3b0_entry4_node_count_0e5f(
    uint8_t *ram, size_t ram_size, pr_u3b0_0dea_cpu *cpu,
    size_t max_traversals);

#endif
