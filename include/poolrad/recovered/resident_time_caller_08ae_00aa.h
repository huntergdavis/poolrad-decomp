#ifndef POOLRAD_RECOVERED_RESIDENT_TIME_CALLER_08AE_00AA_H
#define POOLRAD_RECOVERED_RESIDENT_TIME_CALLER_08AE_00AA_H

#include "poolrad/recovered/resident_time_caller_prefix_08ae_001f.h"

/* RES_08AE:+00AA..+00B1: JG, JL, CMP AX,CX, JBE.
 * Entry FLAGS come from the preceding CMP DX,BX. Stops at +00B2
 * or +00C4 before executing either successor. */
void pr_rec_res_08ae_00aa_branch(pr_rec_res_08ae_001f_cpu *cpu);

#endif
