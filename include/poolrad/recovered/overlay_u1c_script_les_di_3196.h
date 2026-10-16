#ifndef POOLRAD_RECOVERED_OVERLAY_U1C_SCRIPT_LES_DI_3196_H
#define POOLRAD_RECOVERED_OVERLAY_U1C_SCRIPT_LES_DI_3196_H

#include "poolrad/recovered/overlay_u1c_key_branch_33bd.h"

/* Execute only U1C 3582:3196 LES DI,DWORD PTR DS:[49DE] from a supplied
 * first-visit ZF-set CPU/RAM state. Stop at +319A. */
int pr_rec_u1c_3196_les_script_pointer(pr_rec_u1c_33bd_cpu *cpu,
                                        const uint8_t *ram, size_t ram_size);

#endif
