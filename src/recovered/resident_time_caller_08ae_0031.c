#include "poolrad/recovered/resident_time_caller_08ae_0031.h"

void pr_rec_res_08ae_0031_copy_dx_to_bx(pr_rec_res_08ae_001f_cpu *cpu)
{
    cpu->bx = cpu->dx;
    cpu->ip = 0x0033u;
}
