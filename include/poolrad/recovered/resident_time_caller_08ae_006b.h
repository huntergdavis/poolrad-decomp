#ifndef POOLRAD_RECOVERED_RESIDENT_TIME_CALLER_08AE_006B_H
#define POOLRAD_RECOVERED_RESIDENT_TIME_CALLER_08AE_006B_H

#include "poolrad/recovered/resident_time_caller_prefix_08ae_001f.h"

#include <stdint.h>

/* Reached RES_08AE:+006B RETF. ram is the 640 KiB real-mode image. */
void pr_rec_res_08ae_006b_return(pr_rec_res_08ae_001f_cpu *cpu,
                                 const uint8_t ram[655360]);

/* Reached caller +008D..+00A9; stop before the +00AA conditional gate. */
void pr_rec_res_08ae_008d_to_00aa(pr_rec_res_08ae_001f_cpu *cpu,
                                  uint8_t ram[655360]);

#endif
