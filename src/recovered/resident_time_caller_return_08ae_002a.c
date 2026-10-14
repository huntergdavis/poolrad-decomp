#include "poolrad/recovered/resident_time_caller_return_08ae_002a.h"

void pr_rec_res_08ae_002a_call_multiply(pr_rec_res_08ae_001f_cpu *cpu,
                                        uint8_t return_frame[4],
                                        uint16_t target_cs)
{
    const uint16_t caller_cs = cpu->cs;

    /* A far CALL pushes CS, then the offset of the following instruction. */
    return_frame[0] = 0x2fu;
    return_frame[1] = 0x00u;
    return_frame[2] = (uint8_t)caller_cs;
    return_frame[3] = (uint8_t)(caller_cs >> 8);
    cpu->sp = (uint16_t)(cpu->sp - 4u);
    cpu->cs = target_cs;
    cpu->ip = 0x0a5bu;
}

void pr_rec_res_08ae_002f_copy_result(pr_rec_res_08ae_001f_cpu *cpu)
{
    cpu->cx = cpu->ax;
    cpu->ip = 0x0031u;
}
