#include "poolrad/recovered/resident_time_caller_08ae_0031.h"

#include <assert.h>

int main(void)
{
    pr_rec_res_08ae_001f_cpu native = {
        0x10d2u, 0x0031u, 0x14a0u, 0x1d16u, 0x1d16u, 0x3d9au,
        0x3da6u, 0x7e40u, 0x0005u, 0x7e40u, 0x0005u, 0x0001u,
        0x0000u, 0x7206u
    };
    pr_rec_res_08ae_001f_cpu synthetic = native;

    pr_rec_res_08ae_0031_copy_dx_to_bx(&native);
    assert(native.cs == 0x10d2u && native.ip == 0x0033u);
    assert(native.bx == 0x0005u && native.dx == 0x0005u);
    assert(native.ax == 0x7e40u && native.cx == 0x7e40u);
    assert(native.sp == 0x3d9au && native.flags == 0x7206u);

    synthetic.bx = 0x1234u;
    synthetic.dx = 0xbeefu;
    synthetic.flags = 0x7101u;
    pr_rec_res_08ae_0031_copy_dx_to_bx(&synthetic);
    assert(synthetic.bx == 0xbeefu && synthetic.dx == 0xbeefu);
    assert(synthetic.ip == 0x0033u && synthetic.flags == 0x7101u);
    assert(synthetic.cs == 0x10d2u && synthetic.ax == 0x7e40u);
    assert(synthetic.cx == 0x7e40u && synthetic.sp == 0x3d9au);
    return 0;
}
