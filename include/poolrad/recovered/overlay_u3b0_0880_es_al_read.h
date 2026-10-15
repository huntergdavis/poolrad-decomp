#ifndef POOLRAD_RECOVERED_OVERLAY_U3B0_0880_ES_AL_READ_H
#define POOLRAD_RECOVERED_OVERLAY_U3B0_0880_ES_AL_READ_H

#include "poolrad/recovered/overlay_u3b0_cpu.h"

typedef enum {
    PR_U3B0_0880_ES_AL_READ_OK = 0,
    PR_U3B0_0880_ES_AL_READ_ARGUMENT
} pr_u3b0_0880_es_al_read_status;

/* Execute only original +0880 ES MOV AL,[DI] on supplied CF-set fallthrough CPU/RAM. */
pr_u3b0_0880_es_al_read_status pr_u3b0_0880_es_al_read(
    uint8_t *ram, size_t ram_size, pr_u3b0_cpu *cpu);

#endif
