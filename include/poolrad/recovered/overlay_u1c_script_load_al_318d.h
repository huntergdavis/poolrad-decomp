#ifndef POOLRAD_RECOVERED_OVERLAY_U1C_SCRIPT_LOAD_AL_318D_H
#define POOLRAD_RECOVERED_OVERLAY_U1C_SCRIPT_LOAD_AL_318D_H

#include "poolrad/recovered/overlay_u1c_key_branch_33bd.h"

/* Execute only U1C 3582:318D MOV AL,DS:[6F7F] from supplied
 * first-visit +318B fallthrough state. Stop at +3190. */
int pr_rec_u1c_318d_load_latch_al(pr_rec_u1c_33bd_cpu *cpu,
                                  const uint8_t *ram, size_t ram_size);

#endif
