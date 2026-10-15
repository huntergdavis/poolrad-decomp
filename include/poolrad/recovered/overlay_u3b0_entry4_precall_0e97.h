#ifndef POOLRAD_RECOVERED_OVERLAY_U3B0_ENTRY4_PRECALL_0E97_H
#define POOLRAD_RECOVERED_OVERLAY_U3B0_ENTRY4_PRECALL_0E97_H

#include "poolrad/recovered/overlay_u3b0_entry4_nonnull_0dea.h"

typedef enum {
    PR_U3B0_0E97_OK = 0,
    PR_U3B0_0E97_ARGUMENT
} pr_u3b0_0e97_status;

/* Execute original +0E97..+0ED1; stop before near CALL +0CC5. */
pr_u3b0_0e97_status pr_u3b0_entry4_precall_0e97(
    uint8_t *ram, size_t ram_size, pr_u3b0_0dea_cpu *cpu);

#endif
