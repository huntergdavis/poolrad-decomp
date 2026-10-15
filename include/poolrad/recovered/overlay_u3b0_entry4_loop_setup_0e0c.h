#ifndef POOLRAD_RECOVERED_OVERLAY_U3B0_ENTRY4_LOOP_SETUP_0E0C_H
#define POOLRAD_RECOVERED_OVERLAY_U3B0_ENTRY4_LOOP_SETUP_0E0C_H

#include "poolrad/recovered/overlay_u3b0_entry4_nonnull_0dea.h"

typedef enum {
    PR_U3B0_0E0C_OK = 0,
    PR_U3B0_0E0C_ARGUMENT
} pr_u3b0_0e0c_status;

/* Execute +0E0C..+0E2F after the node-count caller; stop at +0E30. */
pr_u3b0_0e0c_status pr_u3b0_entry4_loop_setup_0e0c(
    uint8_t *ram, size_t ram_size, pr_u3b0_0dea_cpu *cpu);

#endif
