#ifndef POOLRAD_RECOVERED_OVERLAY_U1C_SCRIPT_STORE_AL_3190_H
#define POOLRAD_RECOVERED_OVERLAY_U1C_SCRIPT_STORE_AL_3190_H

#include "poolrad/recovered/overlay_u1c_key_branch_33bd.h"

/* Execute only U1C 3582:3190 MOV DS:[6F7E],AL from supplied
 * first-visit ZF-set CPU/RAM state. Stop at +3193. */
int pr_rec_u1c_3190_store_latch_al(pr_rec_u1c_33bd_cpu *cpu,
                                   uint8_t *ram, size_t ram_size);

#endif
