#ifndef POOLRAD_RECOVERED_OVERLAY_U1C_SCRIPT_CALL_FRAME_31A4_H
#define POOLRAD_RECOVERED_OVERLAY_U1C_SCRIPT_CALL_FRAME_31A4_H

#include "poolrad/recovered/overlay_u1c_key_branch_33bd.h"

/* Execute only U1C 3582:31A4 PUSH CS; +31A5 near CALL +2F2D from a
 * supplied first-visit ZF-set CPU/RAM state. Stop before the callee. */
int pr_rec_u1c_31a4_call_dispatch_frame(pr_rec_u1c_33bd_cpu *cpu,
                                         uint8_t *ram, size_t ram_size);

#endif
