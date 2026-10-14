#ifndef POOLRAD_RECOVERED_RESIDENT_TIME_CALLER_08AE_0031_H
#define POOLRAD_RECOVERED_RESIDENT_TIME_CALLER_08AE_0031_H

#include "poolrad/recovered/resident_time_caller_prefix_08ae_001f.h"

/* RES_08AE:+0031 MOV BX,DX, stopped before +0033.
 * The caller supplies the arithmetic helper's returned CPU state. */
void pr_rec_res_08ae_0031_copy_dx_to_bx(pr_rec_res_08ae_001f_cpu *cpu);

#endif
