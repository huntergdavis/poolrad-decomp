#ifndef POOLRAD_REPLAY_SLOT_A_DISPATCH_FIRST_BRANCH_H
#define POOLRAD_REPLAY_SLOT_A_DISPATCH_FIRST_BRANCH_H
#include "poolrad/recovered/overlay_u1c_run_3171_cpu.h"
/* Conditional 2F33 CMP AL,0 and 2F35 JNE. Caller authenticates code and
 * provides disjoint host objects. Refusal preserves CPU and RAM. Stops before
 * EXIT call setup at 2F37 or next opcode comparison at 2F3E. */
int pr_slot_a_dispatch_first_branch(const uint8_t *ram, size_t size,
    const uint8_t *code, size_t code_size, pr_u1c_3171_cpu *cpu);
#endif
