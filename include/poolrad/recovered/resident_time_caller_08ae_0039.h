#ifndef POOLRAD_RECOVERED_RESIDENT_TIME_CALLER_08AE_0039_H
#define POOLRAD_RECOVERED_RESIDENT_TIME_CALLER_08AE_0039_H

#include "poolrad/recovered/resident_time_caller_prefix_08ae_001f.h"

/* RES_08AE:+0039..+003E: XOR DX,DX; ADD AX,CX; ADC DX,BX.
 * Stops before MOV CX,AX at +003F. No memory is touched. */
void pr_rec_res_08ae_0039_accumulate(pr_rec_res_08ae_001f_cpu *cpu);

#endif
