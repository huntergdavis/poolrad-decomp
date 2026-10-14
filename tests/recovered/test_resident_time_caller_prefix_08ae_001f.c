#include "poolrad/recovered/resident_time_caller_prefix_08ae_001f.h"

#include <assert.h>
#include <stdint.h>

int main(void)
{
    pr_rec_res_08ae_001f_cpu observed = {
        0x10d2u, 0x001fu, 0x14a0u, 0x1d16u, 0x1d16u, 0x3d9au,
        0x3da6u, 0x0001u, 0x0000u, 0x0100u, 0x2718u, 0x0001u,
        0x3da2u, 0x7246u
    };
    pr_rec_res_08ae_001f_cpu synthetic = observed;
    const uint8_t observed_word[2] = {0x01u, 0x00u};
    const uint8_t synthetic_word[2] = {0x34u, 0x12u};

    pr_rec_res_08ae_001f_prepare_multiply(&observed, observed_word);
    assert(observed.cs == 0x10d2u && observed.ip == 0x002au);
    assert(observed.ds == 0x14a0u && observed.es == 0x1d16u);
    assert(observed.ss == 0x1d16u && observed.sp == 0x3d9au);
    assert(observed.bp == 0x3da6u && observed.ax == 0x0001u);
    assert(observed.bx == 0x0005u && observed.cx == 0x7e40u);
    assert(observed.dx == 0u && observed.si == 1u);
    assert(observed.di == 0x3da2u && observed.flags == 0x7246u);

    synthetic.ax = 0xa55au;
    synthetic.bx = 0xa55au;
    synthetic.cx = 0xa55au;
    synthetic.dx = 0xa55au;
    synthetic.flags = 0x7616u;
    pr_rec_res_08ae_001f_prepare_multiply(&synthetic, synthetic_word);
    assert(synthetic.ax == 0x1234u && synthetic.bx == 5u);
    assert(synthetic.cx == 0x7e40u && synthetic.dx == 0u);
    assert(synthetic.flags == 0x7646u);
    assert(synthetic.cs == 0x10d2u && synthetic.ip == 0x002au);
    assert(synthetic.sp == 0x3d9au && synthetic.bp == 0x3da6u);
    return 0;
}
