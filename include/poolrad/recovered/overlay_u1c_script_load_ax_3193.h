#ifndef POOLRAD_RECOVERED_OVERLAY_U1C_SCRIPT_LOAD_AX_3193_H
#define POOLRAD_RECOVERED_OVERLAY_U1C_SCRIPT_LOAD_AX_3193_H

#include "poolrad/recovered/overlay_u1c_key_branch_33bd.h"

/* Execute only U1C 3582:3193 MOV AX,DS:[49ED] from supplied
 * first-visit ZF-set CPU/RAM state. Stop at +3196. */
int pr_rec_u1c_3193_load_script_ax(pr_rec_u1c_33bd_cpu *cpu,
                                    const uint8_t *ram, size_t ram_size);

#endif
