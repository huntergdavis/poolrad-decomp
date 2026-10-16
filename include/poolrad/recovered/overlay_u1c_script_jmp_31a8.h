#ifndef POOLRAD_RECOVERED_OVERLAY_U1C_SCRIPT_JMP_31A8_H
#define POOLRAD_RECOVERED_OVERLAY_U1C_SCRIPT_JMP_31A8_H

#include "poolrad/recovered/overlay_u1c_key_branch_33bd.h"

/* Execute only U1C 3582:31A8 JMP short -43 from supplied CPU/RAM state.
 * Stop at +317F, before its loop test. Return zero without mutation when
 * the address, memory size, or loaded opcode bytes do not match. */
int pr_rec_u1c_31a8_jmp_loop(pr_rec_u1c_33bd_cpu *cpu,
                             const uint8_t *ram, size_t ram_size);

#endif
