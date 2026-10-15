#include "poolrad/recovered/resident_time_caller_composed_08ae_001f.h"

#include <assert.h>
#include <stdint.h>

typedef struct helper_context {
    uint8_t *return_frame;
    uint16_t expected_target_cs;
    unsigned calls;
} helper_context;

static void synthetic_helper_return(pr_rec_res_08ae_001f_cpu *cpu, void *ctx)
{
    helper_context *helper = ctx;
    assert(cpu->cs == helper->expected_target_cs && cpu->ip == 0x0a5bu);
    assert(cpu->sp == 0x3d96u && cpu->ax == 0x1234u);
    assert(cpu->bx == 5u && cpu->cx == 0x7e40u && cpu->dx == 0u);
    assert(helper->return_frame[0] == 0x2fu);
    assert(helper->return_frame[1] == 0u);
    assert(helper->return_frame[2] == 0xd2u);
    assert(helper->return_frame[3] == 0x10u);
    helper->calls++;
    cpu->cs = 0x10d2u;
    cpu->ip = 0x002fu;
    cpu->sp = 0x3d9au;
    cpu->ax = 0xbeefu;
    cpu->bx = 0x1111u;
    cpu->cx = 0x2222u;
    cpu->dx = 0xa55au;
    cpu->flags = 0x7206u;
}

int main(void)
{
    pr_rec_res_08ae_001f_cpu cpu = {
        0x10d2u, 0x001fu, 0x14a0u, 0x1d16u, 0x1d16u, 0x3d9au,
        0x3da6u, 0x7777u, 0x8888u, 0x9999u, 0xaaaau, 0x0001u,
        0x3da2u, 0x7246u
    };
    const uint8_t local_word[2] = {0x34u, 0x12u};
    uint8_t return_frame[4] = {0u, 0u, 0u, 0u};
    helper_context helper = {return_frame, 0x131cu, 0u};

    pr_rec_res_08ae_001f_to_0033(&cpu, local_word, return_frame,
                                  0x131cu, synthetic_helper_return, &helper);
    assert(helper.calls == 1u);
    assert(cpu.cs == 0x10d2u && cpu.ip == 0x0033u);
    assert(cpu.ax == 0xbeefu && cpu.cx == 0xbeefu);
    assert(cpu.dx == 0xa55au && cpu.bx == 0xa55au);
    assert(cpu.ds == 0x14a0u && cpu.es == 0x1d16u);
    assert(cpu.ss == 0x1d16u && cpu.sp == 0x3d9au);
    assert(cpu.bp == 0x3da6u && cpu.si == 1u);
    assert(cpu.di == 0x3da2u && cpu.flags == 0x7206u);
    return 0;
}
