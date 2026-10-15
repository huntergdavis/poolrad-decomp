#ifndef POOLRAD_RECOVERED_OVERLAY_U3B0_0883_XOR_AH_H
#define POOLRAD_RECOVERED_OVERLAY_U3B0_0883_XOR_AH_H

#include "poolrad/recovered/overlay_u3b0_cpu.h"

typedef enum {
    PR_U3B0_0883_XOR_AH_OK = 0,
    PR_U3B0_0883_XOR_AH_ARGUMENT
} pr_u3b0_0883_xor_ah_status;

/* Execute only original +0883 XOR AH,AH on supplied CF-set fallthrough state. */
pr_u3b0_0883_xor_ah_status pr_u3b0_0883_xor_ah(
    uint8_t *ram, size_t ram_size, pr_u3b0_cpu *cpu);

#endif
