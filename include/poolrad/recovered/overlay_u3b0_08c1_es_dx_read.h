#ifndef POOLRAD_RECOVERED_OVERLAY_U3B0_08C1_ES_DX_READ_H
#define POOLRAD_RECOVERED_OVERLAY_U3B0_08C1_ES_DX_READ_H

#include "poolrad/recovered/overlay_u3b0_cpu.h"

typedef enum {
    PR_U3B0_08C1_ES_DX_READ_OK = 0,
    PR_U3B0_08C1_ES_DX_READ_ARGUMENT
} pr_u3b0_08c1_es_dx_read_status;

/* Execute only original +08C1 ES MOV DX,[DI+2Ch] on supplied taken state. */
pr_u3b0_08c1_es_dx_read_status pr_u3b0_08c1_es_dx_read(
    uint8_t *ram, size_t ram_size, pr_u3b0_cpu *cpu);

#endif
