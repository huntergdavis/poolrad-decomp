#include "poolrad/recovered/overlay_u25_entry7_0511.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define DS 0x3000u
#define SS 0xE000u
#define SP 0x8000u
#define BP (SP - 2u)

typedef struct {
    unsigned calls, abort_at;
    uint16_t entry_flags[2];
    uint16_t args[2][3];
    int clobber_bp_slot;
} fixture;

static size_t physical(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & (PR_U25_0511_RAM_SIZE - 1u);
}

static void put16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t value)
{
    size_t p = physical(seg, off);
    ram[p] = (uint8_t)value;
    ram[(p + 1u) & (PR_U25_0511_RAM_SIZE - 1u)] = (uint8_t)(value >> 8);
}

static uint16_t get16(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    size_t p = physical(seg, off);
    return (uint16_t)(ram[p] |
        ((uint16_t)ram[(p + 1u) & (PR_U25_0511_RAM_SIZE - 1u)] << 8));
}

static int external(void *context, uint8_t *ram, size_t ram_size,
                    pr_u25_0511_cpu *cpu, pr_u25_0511_call call)
{
    fixture *f = context;
    unsigned n = f->calls++, i, words = call == PR_U25_0511_U3B9_6 ? 2u : 3u;
    assert(ram_size == PR_U25_0511_RAM_SIZE && n < 2u && call == (pr_u25_0511_call)n);
    assert(cpu->ip == (n == 0u ? 0x052Du : 0x053Bu));
    f->entry_flags[n] = cpu->flags;
    for (i = 0u; i < words; ++i)
        f->args[n][i] = get16(ram, cpu->ss, (uint16_t)(cpu->sp + 2u * i));
    if (f->calls == f->abort_at) return 0;
    cpu->ax = (uint16_t)(0x7100u + n);
    cpu->flags = n == 0u ? 0x0A53u : 0x0457u;
    if (n == 1u && f->clobber_bp_slot)
        put16(ram, cpu->ss, (uint16_t)(cpu->sp + 6u), 0x3344u); /* saved BP */
    return 1;
}

static uint8_t *prepare(pr_u25_0511_cpu *cpu)
{
    uint8_t *ram = calloc(PR_U25_0511_RAM_SIZE, 1u);
    assert(ram != NULL);
    memset(cpu, 0, sizeof *cpu);
    cpu->ax = 0xA5CDu; cpu->bp = 0x7777u; cpu->sp = SP;
    cpu->cs = 0xF000u; cpu->ip = 0x0511u; cpu->ds = DS; cpu->ss = SS;
    cpu->flags = 0x0287u;
    put16(ram, SS, SP, 0x0911u);
    put16(ram, SS, SP + 2u, 0xD000u);
    put16(ram, SS, SP + 4u, 0x1122u); /* [bp+6] */
    put16(ram, SS, SP + 6u, 0x3344u); /* [bp+8] */
    ram[physical(DS, 0x84E1u)] = 0x55u;
    return ram;
}

static void test_ok(void)
{
    pr_u25_0511_cpu cpu;
    fixture f;
    uint8_t *ram = prepare(&cpu);
    memset(&f, 0, sizeof f);
    assert(pr_u25_entry7_0511(ram, PR_U25_0511_RAM_SIZE, &cpu, external, &f)
           == PR_U25_0511_OK);
    assert(ram[physical(DS, 0x84E1u)] == 0u);
    assert(ram[physical(DS, 0x49FDu)] == 0x44u && ram[physical(DS, 0x49FEu)] == 0x22u);
    /* Entry 6 gets ([bp+6], [bp+8]) with AH carried; entry 5 gets (1, 3, 3). */
    assert(f.args[0][0] == 0xA522u && f.args[0][1] == 0xA544u);
    assert((f.args[1][0] & 0xFFu) == 1u && (f.args[1][1] & 0xFFu) == 3u &&
           (f.args[1][2] & 0xFFu) == 3u && (f.args[1][0] >> 8) == 0x71u);
    /* No flag writer in the body. */
    assert(f.entry_flags[0] == 0x0287u && f.entry_flags[1] == 0x0A53u);
    assert(cpu.flags == 0x0457u && cpu.ax == 0x7101u);
    assert(cpu.sp == SP + 8u && cpu.bp == 0x7777u);
    assert(cpu.ip == 0x0911u && cpu.cs == 0xD000u);
    free(ram);

    /* POP BP reads the live saved-BP word. */
    ram = prepare(&cpu);
    memset(&f, 0, sizeof f); f.clobber_bp_slot = 1;
    assert(pr_u25_entry7_0511(ram, PR_U25_0511_RAM_SIZE, &cpu, external, &f)
           == PR_U25_0511_OK);
    assert(cpu.bp == 0x3344u);
    free(ram);
}

static void test_abort_and_arguments(void)
{
    pr_u25_0511_cpu cpu;
    fixture f;
    uint8_t *ram;
    unsigned i;
    for (i = 1u; i <= 2u; ++i) {
        ram = prepare(&cpu);
        memset(&f, 0, sizeof f); f.abort_at = i;
        assert(pr_u25_entry7_0511(ram, PR_U25_0511_RAM_SIZE, &cpu, external, &f)
               == PR_U25_0511_CALL_ABORT);
        assert(cpu.ip == (i == 1u ? 0x052Du : 0x053Bu));
        free(ram);
    }
    ram = prepare(&cpu);
    memset(&f, 0, sizeof f);
    assert(pr_u25_entry7_0511(NULL, PR_U25_0511_RAM_SIZE, &cpu, external, &f)
           == PR_U25_0511_ARGUMENT);
    assert(pr_u25_entry7_0511(ram, PR_U25_0511_RAM_SIZE - 1u, &cpu, external, &f)
           == PR_U25_0511_ARGUMENT);
    assert(pr_u25_entry7_0511(ram, PR_U25_0511_RAM_SIZE, NULL, external, &f)
           == PR_U25_0511_ARGUMENT);
    assert(pr_u25_entry7_0511(ram, PR_U25_0511_RAM_SIZE, &cpu, NULL, &f)
           == PR_U25_0511_ARGUMENT);
    assert(cpu.sp == SP && ram[physical(DS, 0x84E1u)] == 0x55u);
    free(ram);
}

int main(void)
{
    test_ok();
    test_abort_and_arguments();
    return 0;
}
