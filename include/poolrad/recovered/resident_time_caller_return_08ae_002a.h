#ifndef POOLRAD_RECOVERED_RESIDENT_TIME_CALLER_RETURN_08AE_002A_H
#define POOLRAD_RECOVERED_RESIDENT_TIME_CALLER_RETURN_08AE_002A_H

#include "poolrad/recovered/resident_time_caller_prefix_08ae_001f.h"

#include <stdint.h>

/* RES_08AE:+002A far CALL, stopping at the arithmetic callee entry.
 * return_frame maps SS:[SP-4..SP-1] at entry. target_cs is the relocated
 * segment of RES_0AF8; the original immediate is 0AF8h. */
void pr_rec_res_08ae_002a_call_multiply(pr_rec_res_08ae_001f_cpu *cpu,
                                        uint8_t return_frame[4],
                                        uint16_t target_cs);

/* RES_08AE:+002F MOV CX,AX after an externally supplied callee return.
 * Stops before the next instruction at +0031. */
void pr_rec_res_08ae_002f_copy_result(pr_rec_res_08ae_001f_cpu *cpu);

#endif
