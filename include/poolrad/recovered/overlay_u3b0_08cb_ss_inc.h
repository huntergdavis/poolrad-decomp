#ifndef POOLRAD_RECOVERED_OVERLAY_U3B0_08CB_SS_INC_H
#define POOLRAD_RECOVERED_OVERLAY_U3B0_08CB_SS_INC_H

#include "poolrad/recovered/overlay_u3b0_cpu.h"

typedef enum {
    PR_U3B0_08CB_SS_INC_OK = 0,
    PR_U3B0_08CB_SS_INC_ARGUMENT
} pr_u3b0_08cb_ss_inc_status;

/* Execute only original +08CB INC byte SS:[BP-9] on supplied taken state. */
pr_u3b0_08cb_ss_inc_status pr_u3b0_08cb_ss_inc(
    uint8_t *ram, size_t ram_size, pr_u3b0_cpu *cpu);

#endif
