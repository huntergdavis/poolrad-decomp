#ifndef POOLRAD_RECOVERED_OVERLAY_U1C_KEY_JMP_33EA_H
#define POOLRAD_RECOVERED_OVERLAY_U1C_KEY_JMP_33EA_H

#include "poolrad/recovered/overlay_u1c_key_branch_33bd.h"

/* Execute only U1C 3582:33EA JMP +40 from independently supplied
 * return state after near callee +2DD3. Stop at +342C. */
int pr_rec_u1c_33ea_jmp_342c(pr_rec_u1c_33bd_cpu *cpu,
                              const uint8_t *ram, size_t ram_size);

#endif
