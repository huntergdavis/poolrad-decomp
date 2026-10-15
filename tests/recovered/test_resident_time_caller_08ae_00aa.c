#include "poolrad/recovered/resident_time_caller_08ae_00aa.h"

#include <assert.h>

int main(void)
{
    pr_rec_res_08ae_001f_cpu cpu = {
        0x10d2u, 0x00aau, 0x14a0u, 0x1d16u, 0x1d16u, 0x3dacu,
        0x3db4u, 0x000au, 0x0000u, 0x000au, 0x0000u, 0x0001u,
        0x0000u, 0x7246u
    };

    pr_rec_res_08ae_00aa_branch(&cpu);
    assert(cpu.ip == 0x00c4u && cpu.flags == 0x7246u);
    assert(cpu.ax == 10u && cpu.cx == 10u && cpu.dx == 0u && cpu.bx == 0u);
    assert(cpu.cs == 0x10d2u && cpu.sp == 0x3dacu && cpu.bp == 0x3db4u);

    cpu.ip = 0x00aau;
    cpu.flags = 0x7202u; /* Signed high WORD greater: JG. */
    pr_rec_res_08ae_00aa_branch(&cpu);
    assert(cpu.ip == 0x00b2u && cpu.flags == 0x7202u);

    cpu.ip = 0x00aau;
    cpu.flags = 0x7282u; /* Signed high WORD less: JL. */
    pr_rec_res_08ae_00aa_branch(&cpu);
    assert(cpu.ip == 0x00c4u && cpu.flags == 0x7282u);

    cpu.ip = 0x00aau;
    cpu.flags = 0x7246u;
    cpu.ax = 11u;
    pr_rec_res_08ae_00aa_branch(&cpu);
    assert(cpu.ip == 0x00b2u && cpu.flags == 0x7202u);

    cpu.ip = 0x00aau;
    cpu.flags = 0x7246u;
    cpu.ax = 9u;
    pr_rec_res_08ae_00aa_branch(&cpu);
    assert(cpu.ip == 0x00c4u && cpu.flags == 0x7297u);

    cpu.ip = 0x00aau;
    cpu.flags = 0x7246u;
    cpu.ax = 0x8000u;
    cpu.cx = 1u;
    pr_rec_res_08ae_00aa_branch(&cpu);
    assert(cpu.ip == 0x00b2u && cpu.flags == 0x7a16u);
    return 0;
}
