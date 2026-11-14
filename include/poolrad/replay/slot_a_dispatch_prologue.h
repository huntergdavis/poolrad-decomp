#ifndef POOLRAD_REPLAY_SLOT_A_DISPATCH_PROLOGUE_H
#define POOLRAD_REPLAY_SLOT_A_DISPATCH_PROLOGUE_H
#include "poolrad/recovered/overlay_u1c_run_3171_cpu.h"
/* Conditional 2F2D..2F32 prologue, stopping before CMP at 2F33. Caller
 * authenticates the six code bytes and provides disjoint host objects.
 * Full 1 MiB RAM is required; refusal leaves RAM and CPU untouched. */
int pr_slot_a_dispatch_prologue(uint8_t *ram, size_t size,
    const uint8_t *code, size_t code_size, pr_u1c_3171_cpu *cpu);
#endif
