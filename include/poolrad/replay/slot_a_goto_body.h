#ifndef POOLRAD_REPLAY_SLOT_A_GOTO_BODY_H
#define POOLRAD_REPLAY_SLOT_A_GOTO_BODY_H
#include "poolrad/recovered/overlay_u1c_goto_00f4_cpu.h"
#include "poolrad/recovered/overlay_u1c_run_3171_cpu.h"
/* Authenticated snapshots: body 29, each stub 5, loader 310, word 36 bytes.
 * Caller supplies immutable guest code and disjoint host objects. */
typedef struct { const uint8_t *body, *loader_stub, *word_stub, *loader, *word; } pr_goto_refs;
int pr_slot_a_goto_body(uint8_t *ram, size_t size, const pr_goto_refs *refs,
    uint16_t system_cs, pr_u1c_3171_cpu *cpu, pr_u1c_goto_status *status,
    pr_u1c_goto_callee *callee, int *callee_status);
#endif
