#include "poolrad/recovered/resident_time_caller_08ae_0039.h"

#include <assert.h>

int main(void)
{
    pr_rec_res_08ae_001f_cpu cpu = {
        0x10d2u, 0x0039u, 0x14a0u, 0x1d16u, 0x1d16u, 0x3d9au,
        0x3da6u, 0x0000u, 0x0005u, 0x7e40u, 0x0000u, 0x0001u,
        0x0000u, 0x7246u
    };

    pr_rec_res_08ae_0039_accumulate(&cpu);
    assert(cpu.ip == 0x003fu && cpu.ax == 0x7e40u && cpu.dx == 5u);
    assert(cpu.bx == 5u && cpu.cx == 0x7e40u && cpu.flags == 0x7206u);
    assert(cpu.cs == 0x10d2u && cpu.sp == 0x3d9au && cpu.bp == 0x3da6u);

    /* Low-WORD carry must feed ADC. */
    cpu.ip = 0x0039u;
    cpu.ax = 0xffffu;
    cpu.cx = 1u;
    cpu.bx = 0u;
    cpu.dx = 0xaaaau;
    cpu.flags = 0x7203u;
    pr_rec_res_08ae_0039_accumulate(&cpu);
    assert(cpu.ax == 0u && cpu.dx == 1u && cpu.flags == 0x7202u);

    /* ADC high-WORD overflow and auxiliary carry are independently visible. */
    cpu.ip = 0x0039u;
    cpu.ax = 0xffffu;
    cpu.cx = 1u;
    cpu.bx = 0x7fffu;
    pr_rec_res_08ae_0039_accumulate(&cpu);
    assert(cpu.ax == 0u && cpu.dx == 0x8000u && cpu.flags == 0x7a96u);

    cpu.ip = 0x0039u;
    cpu.ax = 0u;
    cpu.cx = 0u;
    cpu.bx = 0xffffu;
    pr_rec_res_08ae_0039_accumulate(&cpu);
    assert(cpu.ax == 0u && cpu.dx == 0xffffu && cpu.flags == 0x7286u);
    return 0;
}
