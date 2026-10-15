#include "poolrad/recovered/resident_time_caller_08ae_0036.h"

#include <assert.h>

int main(void)
{
    pr_rec_res_08ae_001f_cpu native = {
        0x10d2u, 0x0036u, 0x14a0u, 0x1d16u, 0x1d16u, 0x3d9au,
        0x3da6u, 0x1770u, 0x0005u, 0x7e40u, 0x0005u, 0x0001u,
        0x0000u, 0x7206u
    };
    pr_rec_res_08ae_001f_cpu synthetic = native;
    const uint8_t zero[2] = {0u, 0u};
    const uint8_t two[2] = {2u, 0u};
    const uint8_t three[2] = {3u, 0u};

    pr_rec_res_08ae_0036_mul_local(&native, zero);
    assert(native.cs == 0x10d2u && native.ip == 0x0039u);
    assert(native.ax == 0u && native.dx == 0u);
    assert(native.bx == 5u && native.cx == 0x7e40u);
    assert(native.sp == 0x3d9au && native.flags == 0x7246u);

    synthetic.ax = 0x8000u;
    synthetic.flags = 0x7096u; /* Preserve incoming AF and SF. */
    pr_rec_res_08ae_0036_mul_local(&synthetic, two);
    assert(synthetic.ax == 0u && synthetic.dx == 1u);
    assert(synthetic.flags == 0x78d3u && synthetic.ip == 0x0039u);

    synthetic.ax = 0x0101u;
    synthetic.flags = 0x7096u;
    synthetic.ip = 0x0036u;
    pr_rec_res_08ae_0036_mul_local(&synthetic, three);
    assert(synthetic.ax == 0x0303u && synthetic.dx == 0u);
    assert(synthetic.flags == 0x7096u && synthetic.ip == 0x0039u);
    return 0;
}
