#include "poolrad/recovered/overlay_u7f_roll_call_0fde.h"
#include "poolrad/recovered/overlay_u9e_entry8_roll.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static size_t physical(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & (PR_U7F_0FDE_RAM_SIZE - 1u);
}

static uint16_t word(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    size_t at = physical(seg, off);
    return (uint16_t)(ram[at] |
        ((uint16_t)ram[(at + 1u) & (PR_U7F_0FDE_RAM_SIZE - 1u)] << 8));
}

static void put(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t value)
{
    size_t at = physical(seg, off);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & (PR_U7F_0FDE_RAM_SIZE - 1u)] = (uint8_t)(value >> 8);
}

typedef struct {
    uint16_t source_cs;
    int mode;
    unsigned calls;
    uint16_t word;   /* the loader-relocated segment half of the LCALL at 0FDE (CS:0FE1) */
} context;

static int roll(void *opaque, uint8_t *ram, size_t size, pr_u7f_0fde_cpu *cpu)
{
    context *ctx = opaque;
    uint32_t seed;
    uint8_t al;
    assert(size == PR_U7F_0FDE_RAM_SIZE);
    assert(cpu->cs == ctx->word && cpu->ip == 0x0048u);   /* entered at the code word's segment, not a constant 00B0 */
    assert(word(ram, cpu->ss, cpu->sp) == 0x0FE3u);
    assert(word(ram, cpu->ss, (uint16_t)(cpu->sp + 2u)) == ctx->source_cs);
    assert(word(ram, cpu->ss, (uint16_t)(cpu->sp + 4u)) == 20u);
    assert(word(ram, cpu->ss, (uint16_t)(cpu->sp + 6u)) == 1u);
    ++ctx->calls;
    if (ctx->mode == 1) {
        cpu->bx = 0xDEADu;
        return 0;
    }
    seed = (uint32_t)word(ram, cpu->ds, 0x4428u) |
           ((uint32_t)word(ram, cpu->ds, 0x442Au) << 16);
    assert(pr_u9e_entry8_roll(1u, 20u, &seed, &al, NULL) == PR_U9E_ENTRY8_OK);
    put(ram, cpu->ds, 0x4428u, (uint16_t)seed);
    put(ram, cpu->ds, 0x442Au, (uint16_t)(seed >> 16));
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | al);
    cpu->bx = 0xBEEFu;
    cpu->flags = 0x0247u;
    cpu->ip = word(ram, cpu->ss, cpu->sp);
    cpu->cs = word(ram, cpu->ss, (uint16_t)(cpu->sp + 2u));
    cpu->sp = (uint16_t)(cpu->sp + 8u);
    if (ctx->mode == 2) cpu->ip = 0xCAFEu;
    return 1;
}

static pr_u7f_0fde_cpu frame(uint8_t *ram, uint16_t sp)
{
    pr_u7f_0fde_cpu cpu = {0xA514u,0x3456u,0x5678u,0x789Au,0x9ABCu,
        0xBCDEu,0x7777u,sp,0x3000u,0x0FDEu,0x1000u,0x1111u,0x2000u,0x7202u};
    memset(ram, 0xA5, PR_U7F_0FDE_RAM_SIZE);
    put(ram, cpu.cs, 0x0FE1u, 0x00B0u);
    put(ram, cpu.ss, sp, 20u);
    put(ram, cpu.ss, (uint16_t)(sp + 2u), 1u);
    put(ram, cpu.ds, 0x4428u, 0x5678u);
    put(ram, cpu.ds, 0x442Au, 0x1234u);
    return cpu;
}

int main(void)
{
    uint8_t *ram = malloc(PR_U7F_0FDE_RAM_SIZE);
    pr_u7f_0fde_cpu cpu, before;
    context ctx = {0x3000u,0,0,0x00B0u};
    assert(ram != NULL);
    cpu = frame(ram, 0x8000u);
    before = cpu;
    assert(pr_u7f_roll_call_0fde(NULL,PR_U7F_0FDE_RAM_SIZE,&cpu,roll,&ctx)==PR_U7F_0FDE_ARGUMENT);
    assert(pr_u7f_roll_call_0fde(ram,0u,&cpu,roll,&ctx)==PR_U7F_0FDE_ARGUMENT);
    assert(pr_u7f_roll_call_0fde(ram,PR_U7F_0FDE_RAM_SIZE,NULL,roll,&ctx)==PR_U7F_0FDE_ARGUMENT);
    assert(pr_u7f_roll_call_0fde(ram,PR_U7F_0FDE_RAM_SIZE,&cpu,NULL,&ctx)==PR_U7F_0FDE_ARGUMENT);
    cpu.ip = 0x0FDFu;
    assert(pr_u7f_roll_call_0fde(ram,PR_U7F_0FDE_RAM_SIZE,&cpu,roll,&ctx)==PR_U7F_0FDE_ARGUMENT);
    cpu = before;
    assert(ctx.calls==0u);

    assert(pr_u7f_roll_call_0fde(ram,PR_U7F_0FDE_RAM_SIZE,&cpu,roll,&ctx)==PR_U7F_0FDE_RETURNED);
    assert(ctx.calls==1u && cpu.cs==0x3000u && cpu.ip==0x0FE3u && cpu.sp==0x8004u);
    assert(cpu.bx==0xBEEFu && cpu.flags==0x0247u);
    assert(word(ram,cpu.ds,0x4428u)!=0x5678u || word(ram,cpu.ds,0x442Au)!=0x1234u);

    cpu = frame(ram, 0x0002u);
    {
        size_t linear_cs=(physical(cpu.ss,0xFFFEu)+2u)&(PR_U7F_0FDE_RAM_SIZE-1u);
        ram[linear_cs]=0xADu;
        ram[(linear_cs+1u)&(PR_U7F_0FDE_RAM_SIZE-1u)]=0xDEu;
    }
    assert(pr_u7f_roll_call_0fde(ram,PR_U7F_0FDE_RAM_SIZE,&cpu,roll,&ctx)==PR_U7F_0FDE_RETURNED);
    assert(cpu.cs==0x3000u && cpu.ip==0x0FE3u && cpu.sp==0x0006u);

    cpu = frame(ram, 0x8100u);
    ctx.mode = 1;
    assert(pr_u7f_roll_call_0fde(ram,PR_U7F_0FDE_RAM_SIZE,&cpu,roll,&ctx)==PR_U7F_0FDE_CALL_ABORT);
    assert(cpu.cs==ctx.word && cpu.ip==0x0048u && cpu.bx==0xDEADu);
    cpu = frame(ram, 0x8200u);
    ctx.mode = 2;
    assert(pr_u7f_roll_call_0fde(ram,PR_U7F_0FDE_RAM_SIZE,&cpu,roll,&ctx)==PR_U7F_0FDE_TRANSFER);
    assert(cpu.cs==0x3000u && cpu.ip==0xCAFEu && cpu.bx==0xBEEFu);
    /* other load bases: the relocated word names another segment and the roll is entered there */
    {
        static const uint16_t words[] = {0x1234u, 0x0001u, 0xFFB0u, 0x10B0u};
        for (unsigned k = 0; k < sizeof words / sizeof words[0]; ++k) {
            cpu = frame(ram, 0x8000u);
            put(ram, cpu.cs, 0x0FE1u, words[k]);
            ctx.mode = 0; ctx.word = words[k];
            assert(pr_u7f_roll_call_0fde(ram,PR_U7F_0FDE_RAM_SIZE,&cpu,roll,&ctx)==PR_U7F_0FDE_RETURNED);
            assert(cpu.cs==0x3000u && cpu.ip==0x0FE3u);
        }
    }
    free(ram);
    return 0;
}
