#ifndef POOLRAD_RECOVERED_RESIDENT_TIME_CALLER_08AE_0036_H
#define POOLRAD_RECOVERED_RESIDENT_TIME_CALLER_08AE_0036_H

#include "poolrad/recovered/resident_time_caller_prefix_08ae_001f.h"

#include <stdint.h>

/* RES_08AE:+0036 MUL WORD PTR SS:[BP-8], stopped before +0039.
 * operand maps the two bytes at SS:[BP-8]. Undefined MUL flags follow
 * the pinned DOSBox-X CPU core used for the native replay. */
void pr_rec_res_08ae_0036_mul_local(pr_rec_res_08ae_001f_cpu *cpu,
                                    const uint8_t operand[2]);

#endif
