#ifndef POOLRAD_RECOVERED_OVERLAY_U3B0_08BD_ES_AX_READ_H
#define POOLRAD_RECOVERED_OVERLAY_U3B0_08BD_ES_AX_READ_H

#include "poolrad/recovered/overlay_u3b0_cpu.h"

typedef enum {
    PR_U3B0_08BD_ES_AX_READ_OK = 0,
    PR_U3B0_08BD_ES_AX_READ_ARGUMENT
} pr_u3b0_08bd_es_ax_read_status;

/* Execute only original +08BD ES MOV AX,[DI+2Ah] on CF-clear taken state. */
pr_u3b0_08bd_es_ax_read_status pr_u3b0_08bd_es_ax_read(
    uint8_t *ram, size_t ram_size, pr_u3b0_cpu *cpu);

#endif
