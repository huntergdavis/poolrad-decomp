#ifndef POOLRAD_RECOVERED_RESIDENT_TIME_CALLER_08AE_00C4_H
#define POOLRAD_RECOVERED_RESIDENT_TIME_CALLER_08AE_00C4_H

#include "poolrad/recovered/resident_time_caller_prefix_08ae_001f.h"

#include <stdint.h>

/* RES_08AE:+00C4..+00D5: compare caller local WORD pairs and branch.
 * locals maps SS:[BP-8..BP-1]. Stops before +0089 PUSH CS / CALL
 * or +00D6 epilogue; neither successor is executed here. */
void pr_rec_res_08ae_00c4_branch(pr_rec_res_08ae_001f_cpu *cpu,
                                 const uint8_t locals[8]);

#endif
