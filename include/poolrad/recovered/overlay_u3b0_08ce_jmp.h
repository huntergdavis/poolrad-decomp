#ifndef POOLRAD_RECOVERED_OVERLAY_U3B0_08CE_JMP_H
#define POOLRAD_RECOVERED_OVERLAY_U3B0_08CE_JMP_H

#include "poolrad/recovered/overlay_u3b0_cpu.h"

typedef enum {
    PR_U3B0_08CE_JMP_OK = 0,
    PR_U3B0_08CE_JMP_ARGUMENT
} pr_u3b0_08ce_jmp_status;

/* Execute original +08CE near JMP on a supplied +08CB successor state. */
pr_u3b0_08ce_jmp_status pr_u3b0_08ce_jmp(
    uint8_t *ram, size_t ram_size, pr_u3b0_cpu *cpu);

#endif
