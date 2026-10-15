#include "poolrad/recovered/resident_time_caller_08ae_00c4.h"

#include <assert.h>

int main(void)
{
    pr_rec_res_08ae_001f_cpu cpu = {
        0x10d2u, 0x00c4u, 0x14a0u, 0x1d16u, 0x1d16u, 0x3dacu,
        0x3db4u, 0x000au, 0x0000u, 0x000au, 0x0000u, 0x0001u,
        0x0000u, 0x7246u
    };
    uint8_t locals[8] = {0x94u, 0x8du, 5u, 0u, 0x9eu, 0x8du, 5u, 0u};

    pr_rec_res_08ae_00c4_branch(&cpu, locals);
    assert(cpu.ip == 0x0089u && cpu.ax == 0x8d94u && cpu.dx == 5u);
    assert(cpu.bx == 0u && cpu.cx == 10u && cpu.flags == 0x7297u);
    assert(cpu.cs == 0x10d2u && cpu.sp == 0x3dacu && cpu.bp == 0x3db4u);

    locals[2] = 6u; /* Signed high WORD greater: JG. */
    cpu.ip = 0x00c4u;
    pr_rec_res_08ae_00c4_branch(&cpu, locals);
    assert(cpu.ip == 0x00d6u && cpu.dx == 6u && cpu.flags == 0x7202u);

    locals[2] = 4u; /* Signed high WORD less: JL. */
    cpu.ip = 0x00c4u;
    pr_rec_res_08ae_00c4_branch(&cpu, locals);
    assert(cpu.ip == 0x0089u && cpu.dx == 4u && cpu.flags == 0x7297u);

    locals[2] = 5u;
    locals[4] = 0x94u; /* Equal low WORD: JB is not taken. */
    locals[5] = 0x8du;
    cpu.ip = 0x00c4u;
    pr_rec_res_08ae_00c4_branch(&cpu, locals);
    assert(cpu.ip == 0x00d6u && cpu.flags == 0x7246u);

    locals[0] = 0x95u; /* Greater low WORD: JB is not taken. */
    cpu.ip = 0x00c4u;
    pr_rec_res_08ae_00c4_branch(&cpu, locals);
    assert(cpu.ip == 0x00d6u && cpu.flags == 0x7202u);

    locals[2] = 0x00u; /* Signed overflow in high-WORD CMP: JL. */
    locals[3] = 0x80u;
    locals[6] = 1u;
    locals[7] = 0u;
    cpu.ip = 0x00c4u;
    pr_rec_res_08ae_00c4_branch(&cpu, locals);
    assert(cpu.ip == 0x0089u && cpu.dx == 0x8000u);
    assert(cpu.flags == 0x7a16u);
    return 0;
}
