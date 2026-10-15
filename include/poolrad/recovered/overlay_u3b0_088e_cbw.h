#ifndef POOLRAD_RECOVERED_OVERLAY_U3B0_088E_CBW_H
#define POOLRAD_RECOVERED_OVERLAY_U3B0_088E_CBW_H

#include "poolrad/recovered/overlay_u3b0_cpu.h"

typedef enum {
    PR_U3B0_088E_CBW_OK = 0,
    PR_U3B0_088E_CBW_ARGUMENT
} pr_u3b0_088e_cbw_status;

/* Execute only original +088E CBW on supplied +088A successor state. */
pr_u3b0_088e_cbw_status pr_u3b0_088e_cbw(
    uint8_t *ram, size_t ram_size, pr_u3b0_cpu *cpu);

#endif
