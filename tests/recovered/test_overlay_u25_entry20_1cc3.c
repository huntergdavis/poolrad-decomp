#include "poolrad/recovered/overlay_u25_entry20_1cc3.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define SS 0xE000u
#define SP 0x8000u
#define BP (SP - 2u)
#define REC_SEG 0x4000u
#define REC_OFF 0x0100u

typedef struct {
    unsigned calls;
    unsigned no_return_at;
    uint16_t sites[21];
} fixture;

static size_t physical(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & (PR_U25_1CC3_RAM_SIZE - 1u);
}

static void put16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t value)
{
    size_t p = physical(seg, off);
    ram[p] = (uint8_t)value;
    ram[(p + 1u) & (PR_U25_1CC3_RAM_SIZE - 1u)] = (uint8_t)(value >> 8);
}

static uint16_t get16(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    size_t p = physical(seg, off);
    return (uint16_t)(ram[p] |
        ((uint16_t)ram[(p + 1u) & (PR_U25_1CC3_RAM_SIZE - 1u)] << 8));
}

/* Synthetic register-ABI helpers (not Real48): "real" = BX, factor = CX.
 * Each helper clobbers CX/SI/DI/ES to prove the caller reloads them. */
static int external(void *context, uint8_t *ram, size_t ram_size,
                    pr_u25_1cc3_cpu *cpu, pr_u25_1cc3_call call)
{
    fixture *f = context;
    (void)ram; /* the helpers are register-only */
    assert(ram_size == PR_U25_1CC3_RAM_SIZE && f->calls < 21u);
    assert(call == (pr_u25_1cc3_call)(f->calls % 3u));
    f->sites[f->calls++] = cpu->ip;
    if (f->calls == f->no_return_at) {
        /* RunError: AX = code, never returns. */
        cpu->ax = call == PR_U25_1CC3_REAL_MUL ? 0x00CDu : 0x00CFu;
        return 0;
    }
    switch (call) {
    case PR_U25_1CC3_LONG_TO_REAL:
        assert(cpu->dx == 0u && (cpu->flags & 0x08D5u) == 0x0044u);
        cpu->bx = cpu->ax;
        cpu->ax = 0x1111u;
        break;
    case PR_U25_1CC3_REAL_MUL:
        assert(cpu->cx == 3u && cpu->si == 0x5555u && cpu->di == 0x6666u);
        cpu->bx = (uint16_t)(cpu->bx * cpu->cx);
        break;
    case PR_U25_1CC3_REAL_TRUNC:
        cpu->ax = cpu->bx;
        cpu->dx = 0u;
        cpu->flags = 0x0A13u;
        break;
    }
    cpu->cx = 0xDEADu; cpu->si = 0xBEEFu; cpu->di = 0xF00Du; cpu->es = 0x1234u;
    return 1;
}

static uint8_t *prepare(pr_u25_1cc3_cpu *cpu)
{
    uint8_t *ram = calloc(PR_U25_1CC3_RAM_SIZE, 1u);
    unsigned k;
    assert(ram != NULL);
    memset(cpu, 0, sizeof *cpu);
    cpu->ax = 0xA5CDu; cpu->bp = 0x7777u; cpu->sp = SP;
    cpu->cs = 0xF000u; cpu->ip = 0x1CC3u; cpu->ds = 0x3000u; cpu->ss = SS;
    cpu->flags = 0x0202u;
    put16(ram, SS, SP, 0x1E24u);
    put16(ram, SS, SP + 2u, 0xD000u);
    put16(ram, SS, SP + 4u, REC_OFF);
    put16(ram, SS, SP + 6u, REC_SEG);
    put16(ram, SS, SP + 8u, 3u);       /* factor: CX */
    put16(ram, SS, SP + 10u, 0x5555u); /* SI */
    put16(ram, SS, SP + 12u, 0x6666u); /* DI */
    put16(ram, REC_SEG, (uint16_t)(REC_OFF + 0x86u), 0xAAAAu);
    put16(ram, REC_SEG, (uint16_t)(REC_OFF + 0x96u), 0xBBBBu);
    for (k = 0u; k < 7u; ++k)
        put16(ram, REC_SEG, (uint16_t)(REC_OFF + 0x88u + 2u * k), (uint16_t)(10u + k));
    return ram;
}

static void test_scale(void)
{
    pr_u25_1cc3_cpu cpu;
    fixture f;
    uint8_t *ram = prepare(&cpu);
    unsigned k;
    memset(&f, 0, sizeof f);
    assert(pr_u25_entry20_1cc3(ram, PR_U25_1CC3_RAM_SIZE, &cpu, external, &f)
           == PR_U25_1CC3_OK);
    assert(f.calls == 21u);
    for (k = 0u; k < 7u; ++k) {
        assert(f.sites[3u * k] == 0x1CD0u + 0x2Au * k);
        assert(f.sites[3u * k + 1u] == 0x1CDEu + 0x2Au * k);
        assert(f.sites[3u * k + 2u] == 0x1CE3u + 0x2Au * k);
        assert(get16(ram, REC_SEG, (uint16_t)(REC_OFF + 0x88u + 2u * k)) == 3u * (10u + k));
    }
    /* Neighbouring words are untouched. */
    assert(get16(ram, REC_SEG, (uint16_t)(REC_OFF + 0x86u)) == 0xAAAAu);
    assert(get16(ram, REC_SEG, (uint16_t)(REC_OFF + 0x96u)) == 0xBBBBu);
    /* Return: RETF 0xA; flags and DX from the last trunc; ES:DI from LES. */
    assert(cpu.sp == SP + 4u + 0x0Au && cpu.bp == 0x7777u);
    assert(cpu.ip == 0x1E24u && cpu.cs == 0xD000u);
    assert(cpu.ax == 3u * 16u && cpu.flags == 0x0A13u);
    assert(cpu.es == REC_SEG && cpu.di == REC_OFF);
    free(ram);
}

static void test_no_return(void)
{
    unsigned at;
    for (at = 1u; at <= 21u; ++at) {
        pr_u25_1cc3_cpu cpu;
        fixture f;
        uint8_t *ram = prepare(&cpu);
        unsigned k, done = (at - 1u) / 3u;
        memset(&f, 0, sizeof f);
        f.no_return_at = at;
        assert(pr_u25_entry20_1cc3(ram, PR_U25_1CC3_RAM_SIZE, &cpu, external, &f)
               == PR_U25_1CC3_NO_RETURN);
        assert(f.calls == at && cpu.ip == f.sites[at - 1u]);
        assert(cpu.sp == BP && get16(ram, SS, BP) == 0x7777u);
        /* Completed blocks stored their word; the failing block did not. */
        for (k = 0u; k < 7u; ++k)
            assert(get16(ram, REC_SEG, (uint16_t)(REC_OFF + 0x88u + 2u * k)) ==
                   (k < done ? 3u * (10u + k) : 10u + k));
        free(ram);
    }
}

static void test_arguments(void)
{
    pr_u25_1cc3_cpu cpu;
    fixture f;
    uint8_t *ram = prepare(&cpu);
    memset(&f, 0, sizeof f);
    assert(pr_u25_entry20_1cc3(NULL, PR_U25_1CC3_RAM_SIZE, &cpu, external, &f)
           == PR_U25_1CC3_ARGUMENT);
    assert(pr_u25_entry20_1cc3(ram, PR_U25_1CC3_RAM_SIZE - 1u, &cpu, external, &f)
           == PR_U25_1CC3_ARGUMENT);
    assert(pr_u25_entry20_1cc3(ram, PR_U25_1CC3_RAM_SIZE, NULL, external, &f)
           == PR_U25_1CC3_ARGUMENT);
    assert(pr_u25_entry20_1cc3(ram, PR_U25_1CC3_RAM_SIZE, &cpu, NULL, &f)
           == PR_U25_1CC3_ARGUMENT);
    assert(cpu.sp == SP && f.calls == 0u);
    free(ram);
}

int main(void)
{
    test_scale();
    test_no_return();
    test_arguments();
    return 0;
}
