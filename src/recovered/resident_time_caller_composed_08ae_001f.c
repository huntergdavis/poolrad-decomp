#include "poolrad/recovered/resident_time_caller_composed_08ae_001f.h"

#include "poolrad/recovered/resident_time_caller_08ae_0031.h"
#include "poolrad/recovered/resident_time_caller_return_08ae_002a.h"

void pr_rec_res_08ae_001f_to_0033(
    pr_rec_res_08ae_001f_cpu *cpu, const uint8_t local_word[2],
    uint8_t return_frame[4], uint16_t target_cs,
    pr_rec_res_08ae_001f_helper_return helper_return, void *ctx)
{
    pr_rec_res_08ae_001f_prepare_multiply(cpu, local_word);
    pr_rec_res_08ae_002a_call_multiply(cpu, return_frame, target_cs);
    helper_return(cpu, ctx);
    pr_rec_res_08ae_002f_copy_result(cpu);
    pr_rec_res_08ae_0031_copy_dx_to_bx(cpu);
}
