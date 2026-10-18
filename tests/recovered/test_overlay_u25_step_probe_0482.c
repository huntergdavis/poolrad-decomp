#include "poolrad/recovered/overlay_u25_step_probe_0482.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define DS 0x3000u
#define SS 0xE000u
#define SP 0x8000u
#define BP (SP - 2u)

typedef struct {
    unsigned calls;
    unsigned abort_at;
    uint8_t results[4];
    uint16_t seen[4][3]; /* x, y, dir words as pushed */
} fixture;

static size_t physical(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & (PR_U25_0482_RAM_SIZE - 1u);
}

static void put16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t value)
{
    size_t p = physical(seg, off);
    ram[p] = (uint8_t)value;
    ram[(p + 1u) & (PR_U25_0482_RAM_SIZE - 1u)] = (uint8_t)(value >> 8);
}

static uint16_t get16(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    size_t p = physical(seg, off);
    return (uint16_t)(ram[p] |
        ((uint16_t)ram[(p + 1u) & (PR_U25_0482_RAM_SIZE - 1u)] << 8));
}

static int external(void *context, uint8_t *ram, size_t ram_size,
                    pr_u25_0482_cpu *cpu, pr_u25_0482_call call)
{
    fixture *f = context;
    unsigned n = f->calls++;
    assert(ram_size == PR_U25_0482_RAM_SIZE && call == PR_U25_0482_U3BD_2);
    assert(cpu->ip == 0x04C9u && n < 4u);
    /* CMP [bp-2],0 just fell through: ZF and PF set. */
    assert((cpu->flags & 0x08D5u) == 0x0044u);
    f->seen[n][0] = get16(ram, cpu->ss, (uint16_t)(cpu->sp + 4u));
    f->seen[n][1] = get16(ram, cpu->ss, (uint16_t)(cpu->sp + 2u));
    f->seen[n][2] = get16(ram, cpu->ss, cpu->sp);
    if (f->calls == f->abort_at) return 0;
    cpu->ax = (uint16_t)(0x7E00u | f->results[n]);
    cpu->flags = 0x0A53u;
    return 1;
}

static uint8_t *prepare(pr_u25_0482_cpu *cpu, uint16_t e6, uint8_t dir,
                        uint8_t y, uint8_t x)
{
    uint8_t *ram = calloc(PR_U25_0482_RAM_SIZE, 1u);
    assert(ram != NULL);
    memset(cpu, 0, sizeof *cpu);
    cpu->ax = 0xA5CDu; cpu->bp = 0x7777u; cpu->sp = SP;
    cpu->cs = 0xF000u; cpu->ip = 0x0482u; cpu->ds = DS; cpu->ss = SS;
    cpu->flags = 0x0202u;
    put16(ram, SS, SP, 0x03FAu);
    put16(ram, SS, SP + 2u, 0xD000u);
    put16(ram, SS, SP + 4u, (uint16_t)(0x1100u | dir));
    put16(ram, SS, SP + 6u, (uint16_t)(0x2200u | y));
    put16(ram, SS, SP + 8u, (uint16_t)(0x3300u | x));
    put16(ram, DS, 0x49D2u, 0x0010u); put16(ram, DS, 0x49D4u, 0x5000u);
    put16(ram, DS, 0x49D6u, 0x0020u); put16(ram, DS, 0x49D8u, 0x5800u);
    put16(ram, 0x5000u, 0x01DCu, e6);
    put16(ram, 0x5800u, 0x05A2u, 0xCAFEu);
    return ram;
}

static void run_ok(uint8_t *ram, pr_u25_0482_cpu *cpu, fixture *f, uint8_t al)
{
    assert(pr_u25_step_probe_0482(ram, PR_U25_0482_RAM_SIZE, cpu, external, f)
           == PR_U25_0482_OK);
    assert((cpu->ax & 0xFFu) == al);
    assert(cpu->sp == SP + 10u && cpu->bp == 0x7777u);
    assert(cpu->ip == 0x03FAu && cpu->cs == 0xD000u);
}

static void test_zero_window(void)
{
    pr_u25_0482_cpu cpu;
    fixture f;
    uint8_t *ram = prepare(&cpu, 0u, 2u, 5u, 7u);
    memset(&f, 0, sizeof f);
    run_ok(ram, &cpu, &f, 2u);
    assert(f.calls == 0u && get16(ram, 0x5800u, 0x05A2u) == 2u);
    /* CMP word,0 fell through to the jump: ZF and PF set. */
    assert((cpu.flags & 0x08D5u) == 0x0044u);
    free(ram);
}

static void test_steps(void)
{
    static const struct { uint8_t dir, y1, x1, y2, x2; } steps[] = {
        {0u, 4u, 7u, 3u, 7u}, {2u, 5u, 8u, 5u, 9u},
        {4u, 6u, 7u, 7u, 7u}, {6u, 5u, 6u, 5u, 5u},
        {3u, 5u, 7u, 5u, 7u}, {0xFFu, 5u, 7u, 5u, 7u}
    };
    unsigned i;
    for (i = 0u; i < sizeof steps / sizeof steps[0]; ++i) {
        pr_u25_0482_cpu cpu;
        fixture f;
        uint8_t *ram = prepare(&cpu, 1u, steps[i].dir, 5u, 7u);
        memset(&f, 0, sizeof f);
        run_ok(ram, &cpu, &f, 2u);
        assert(f.calls == 2u);
        /* The second call sees the first step; AH carries through. */
        assert(f.seen[0][0] == 0xA507u && (f.seen[1][0] >> 8) == 0x7Eu);
        assert((f.seen[1][0] & 0xFFu) == steps[i].x1);
        assert((f.seen[1][1] & 0xFFu) == steps[i].y1);
        assert((f.seen[1][2] & 0xFFu) == steps[i].dir);
        assert(get16(ram, SS, SP + 6u) == (uint16_t)(0x2200u | steps[i].y2));
        assert(get16(ram, SS, SP + 8u) == (uint16_t)(0x3300u | steps[i].x2));
        /* CMP [bp-3],2 with count 2 ends the loop: ZF PF set. */
        assert((cpu.flags & 0x08D5u) == 0x0044u);
        free(ram);
    }
}

static void test_stop(void)
{
    pr_u25_0482_cpu cpu;
    fixture f;
    uint8_t *ram = prepare(&cpu, 0x8000u, 0u, 5u, 7u);
    memset(&f, 0, sizeof f);
    f.results[1] = 9u;
    run_ok(ram, &cpu, &f, 1u);
    assert(f.calls == 2u && get16(ram, SS, SP + 6u) == 0x2204u);
    assert(ram[physical(SS, (uint16_t)(BP - 2u))] == 1u);
    /* CMP [bp-2],0 with stop 1 ends the loop: all six flags clear. */
    assert((cpu.flags & 0x08D5u) == 0u);
    free(ram);

    ram = prepare(&cpu, 1u, 2u, 5u, 7u);
    memset(&f, 0, sizeof f);
    f.results[0] = 0x80u;
    run_ok(ram, &cpu, &f, 0u);
    assert(f.calls == 1u && get16(ram, SS, SP + 8u) == 0x3307u);
    free(ram);
}

static void test_abort_and_arguments(void)
{
    pr_u25_0482_cpu cpu;
    fixture f;
    uint8_t *ram = prepare(&cpu, 1u, 2u, 5u, 7u);
    memset(&f, 0, sizeof f);
    f.abort_at = 2u;
    assert(pr_u25_step_probe_0482(ram, PR_U25_0482_RAM_SIZE, &cpu, external, &f)
           == PR_U25_0482_CALL_ABORT);
    assert(cpu.ip == 0x04C9u && f.calls == 2u && get16(ram, SS, SP + 8u) == 0x3308u);
    free(ram);
    ram = prepare(&cpu, 1u, 2u, 5u, 7u);
    assert(pr_u25_step_probe_0482(NULL, PR_U25_0482_RAM_SIZE, &cpu, external, &f)
           == PR_U25_0482_ARGUMENT);
    assert(pr_u25_step_probe_0482(ram, PR_U25_0482_RAM_SIZE - 1u, &cpu, external, &f)
           == PR_U25_0482_ARGUMENT);
    assert(pr_u25_step_probe_0482(ram, PR_U25_0482_RAM_SIZE, NULL, external, &f)
           == PR_U25_0482_ARGUMENT);
    assert(pr_u25_step_probe_0482(ram, PR_U25_0482_RAM_SIZE, &cpu, NULL, &f)
           == PR_U25_0482_ARGUMENT);
    assert(cpu.sp == SP);
    free(ram);
}


static void loaded_lookup(uint8_t *ram)
{
    ram[physical(0xF000u, 0x04C9u)] = 0x9Au;
    put16(ram, 0xF000u, 0x04CAu, 0x002Au);
    put16(ram, 0xF000u, 0x04CCu, 0x03D5u);
    ram[physical(0x03D5u, 0x002Au)] = 0xEAu;
    put16(ram, 0x03D5u, 0x002Bu, 0x047Bu);
    put16(ram, 0x03D5u, 0x002Du, 0xC000u);
    put16(ram, DS, 0x6A5Cu, 0x0100u);
    put16(ram, DS, 0x6A5Eu, 0x6000u);
    ram[physical(DS, 0x84DCu)] = 5u;
}

static void test_composed(void)
{
    const uint8_t directions[] = {0,2,4,6,1,255};
    const uint8_t ys[] = {3,5,7,5,5,5}, xs[] = {7,9,7,5,7,7};
    pr_u25_0482_cpu cpu, before;
    uint8_t *ram, *copy = malloc(PR_U25_0482_RAM_SIZE);
    assert(copy);
    for (unsigned i = 0; i < sizeof directions; ++i) {
        ram = prepare(&cpu, 1u, directions[i], 5u, 7u);
        loaded_lookup(ram);
        assert(pr_u25_step_probe_0482_fullstate(ram, PR_U25_0482_RAM_SIZE, &cpu) == PR_U25_0482_OK);
        assert((uint8_t)cpu.ax == 2u && cpu.sp == SP+10u && cpu.bp == 0x7777u);
        assert(cpu.cs == 0xD000u && cpu.ip == 0x03FAu);
        assert(get16(ram,SS,SP+6u) == (uint16_t)(0x2200u | ys[i]));
        assert(get16(ram,SS,SP+8u) == (uint16_t)(0x3300u | xs[i]));
        free(ram);
    }
    for (unsigned second = 0; second < 2u; ++second) {
        ram = prepare(&cpu, 1u, 2u, 5u, 7u);
        loaded_lookup(ram);
        ram[physical(0x6000u, (uint16_t)(0x157u + second))] = 0x0Fu;
        assert(pr_u25_step_probe_0482_fullstate(ram, PR_U25_0482_RAM_SIZE, &cpu) == PR_U25_0482_OK);
        assert((uint8_t)cpu.ax == second);
        assert(get16(ram,SS,SP+8u) == 0x3307u + second);
        free(ram);
    }
    /* Zero-window path never needs loaded callee metadata. Its real word
     * store can replace the caller return IP without being suppressed. */
    ram = prepare(&cpu, 0u, 2u, 5u, 7u);
    put16(ram,DS,0x49D6u,SP-0x582u); put16(ram,DS,0x49D8u,SS);
    assert(pr_u25_step_probe_0482_fullstate(ram,PR_U25_0482_RAM_SIZE,&cpu) == PR_U25_0482_OK);
    assert(cpu.ip == 2u && cpu.cs == 0xD000u && (uint8_t)cpu.ax == 2u);
    free(ram);
    ram = prepare(&cpu,1u,2u,5u,7u); loaded_lookup(ram);
    ram[physical(0x03D5u,0x002Au)] = 0u;
    assert(pr_u25_step_probe_0482_fullstate(ram,PR_U25_0482_RAM_SIZE,&cpu) == PR_U25_0482_CODE);
    assert(cpu.ip == 0x04C9u && cpu.cs == 0xF000u && cpu.sp == SP-12u);
    free(ram);
    for (unsigned i = 0; i < 7u; ++i) {
        size_t size = PR_U25_0482_RAM_SIZE;
        ram = prepare(&cpu,1u,2u,5u,7u); loaded_lookup(ram);
        if (i == 0u) size--;
        if (i == 1u) size++;
        if (i == 2u) cpu.ip++;
        if (i == 3u) cpu.sp = 0x7Fu;
        if (i == 4u) cpu.sp = 0xFFFBu;
        if (i >= 5u) { cpu.cs = cpu.ss; cpu.sp = i == 5u ? 0x483u : 0x590u; }
        before = cpu; memcpy(copy,ram,PR_U25_0482_RAM_SIZE);
        assert(pr_u25_step_probe_0482_fullstate(ram,size,&cpu) ==
               (i >= 5u ? PR_U25_0482_CODE_ALIAS : PR_U25_0482_ARGUMENT));
        assert(memcmp(&cpu,&before,sizeof cpu) == 0 && memcmp(ram,copy,PR_U25_0482_RAM_SIZE) == 0);
        free(ram);
    }
    assert(pr_u25_step_probe_0482_fullstate(NULL,PR_U25_0482_RAM_SIZE,&cpu) == PR_U25_0482_ARGUMENT);
    assert(pr_u25_step_probe_0482_fullstate(copy,PR_U25_0482_RAM_SIZE,NULL) == PR_U25_0482_ARGUMENT);
    free(copy);
}

int main(void)
{
    test_composed();
    test_zero_window();
    test_steps();
    test_stop();
    test_abort_and_arguments();
    return 0;
}
