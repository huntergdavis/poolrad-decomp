#ifndef POOLRAD_RECOVERED_OVERLAY_U3B0_0887_SS_DI_READ_H
#define POOLRAD_RECOVERED_OVERLAY_U3B0_0887_SS_DI_READ_H

#include "poolrad/recovered/overlay_u3b0_cpu.h"

typedef enum {
    PR_U3B0_0887_SS_DI_READ_OK = 0,
    PR_U3B0_0887_SS_DI_READ_ARGUMENT
} pr_u3b0_0887_ss_di_read_status;

/* Execute only original +0887 MOV DI,SS:[BP+6] on supplied successor state. */
pr_u3b0_0887_ss_di_read_status pr_u3b0_0887_ss_di_read(
    uint8_t *ram, size_t ram_size, pr_u3b0_cpu *cpu);

#endif
