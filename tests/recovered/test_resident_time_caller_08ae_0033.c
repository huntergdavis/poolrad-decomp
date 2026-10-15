#include "poolrad/recovered/resident_time_caller_08ae_0033.h"

#include <assert.h>

int main(void)
{
    pr_rec_res_08ae_001f_cpu native = {
        0x10d2u, 0x0033u, 0x14a0u, 0x1d16u, 0x1d16u, 0x3d9au,
        0x3da6u, 0x7e40u, 0x0005u, 0x7e40u, 0x0005u, 0x0001u,
        0x0000u, 0x7206u
    };
    pr_rec_res_08ae_001f_cpu synthetic = native;

    pr_rec_res_08ae_0033_load_ax(&native);
    assert(native.cs == 0x10d2u && native.ip == 0x0036u);
    assert(native.ax == 0x1770u && native.bx == 0x0005u);
    assert(native.cx == 0x7e40u && native.dx == 0x0005u);
    assert(native.sp == 0x3d9au && native.flags == 0x7206u);

    synthetic.ax = 0x0000u;
    synthetic.flags = 0x7101u;
    pr_rec_res_08ae_0033_load_ax(&synthetic);
    assert(synthetic.ax == 0x1770u && synthetic.ip == 0x0036u);
    assert(synthetic.flags == 0x7101u && synthetic.bx == 0x0005u);
    assert(synthetic.cx == 0x7e40u && synthetic.dx == 0x0005u);
    return 0;
}
