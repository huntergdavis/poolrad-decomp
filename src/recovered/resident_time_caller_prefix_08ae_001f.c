#include "poolrad/recovered/resident_time_caller_prefix_08ae_001f.h"

void pr_rec_res_08ae_001f_prepare_multiply(pr_rec_res_08ae_001f_cpu *cpu,
                                            const uint8_t local_word[2])
{
    /* MOV AX,SS:[BP-6]. */
    cpu->ax = (uint16_t)((uint16_t)local_word[0] |
                         ((uint16_t)local_word[1] << 8));
    /* XOR DX,DX: the zero result sets ZF/PF, independent of AX. */
    cpu->dx = 0u;
    cpu->flags = (uint16_t)((cpu->flags & (uint16_t)~0x08d5u) | 0x0044u);
    cpu->cx = 0x7e40u;
    cpu->bx = 0x0005u;
    cpu->ip = 0x002au;
}
