#include "poolrad/recovered/overlay_u1c_compare_and_0aed.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    uint16_t values[4];
    unsigned calls[3];
} fixture;

static size_t physical(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & (PR_U1C_0AED_RAM_SIZE - 1u);
}

static void put16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t value)
{
    size_t p = physical(seg, off);
    ram[p] = (uint8_t)value;
    ram[(p + 1u) & (PR_U1C_0AED_RAM_SIZE - 1u)] = (uint8_t)(value >> 8);
}

static uint16_t get16(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    size_t p = physical(seg, off);
    return (uint16_t)(ram[p] |
        ((uint16_t)ram[(p + 1u) & (PR_U1C_0AED_RAM_SIZE - 1u)] << 8));
}

static int external(void *context, uint8_t *ram, size_t ram_size,
                    pr_u1c_0aed_cpu *cpu, pr_u1c_0aed_call call)
{
    fixture *f = context;
    assert(ram_size == PR_U1C_0AED_RAM_SIZE);
    assert(call <= PR_U1C_0AED_VALUE);
    ++f->calls[call];
    if (call == PR_U1C_0AED_FILL) {
        uint16_t value = get16(ram, cpu->ss, cpu->sp);
        uint16_t size = get16(ram, cpu->ss, (uint16_t)(cpu->sp + 2u));
        uint16_t off = get16(ram, cpu->ss, (uint16_t)(cpu->sp + 4u));
        uint16_t seg = get16(ram, cpu->ss, (uint16_t)(cpu->sp + 6u));
        unsigned i;
        assert(value == 0u && size == 6u && off == 0x6F78u && seg == cpu->ds);
        for (i = 0u; i < size; ++i)
            ram[physical(seg, (uint16_t)(off + i))] = (uint8_t)value;
    } else if (call == PR_U1C_0AED_LOADER) {
        assert(get16(ram, cpu->ss, cpu->sp) == 4u);
    } else {
        unsigned n = f->calls[PR_U1C_0AED_VALUE] - 1u;
        assert(n < 4u);
        assert((get16(ram, cpu->ss, cpu->sp) & 0xFFu) == n + 1u);
        cpu->ax = f->values[n];
    }
    return 1;
}

static void run(const uint16_t values[4], uint16_t expected_flag)
{
    uint8_t *ram = calloc(PR_U1C_0AED_RAM_SIZE, 1u);
    pr_u1c_0aed_cpu cpu = {0};
    fixture f = {{0}, {0}};
    assert(ram != NULL);
    memcpy(f.values, values, sizeof(f.values));
    cpu.cs = 0xF000u; cpu.ds = 0x3000u; cpu.ss = 0xE000u;
    cpu.sp = 0x8000u; cpu.ip = 0x0AEDu; cpu.bp = 0x7777u;
    put16(ram, cpu.ss, cpu.sp, 0x0712u);
    put16(ram, cpu.ss, (uint16_t)(cpu.sp + 2u), 0xD000u);
    memset(ram + physical(cpu.ds, 0x6F78u), 0xA5, 6u);
    assert(pr_u1c_compare_and_0aed(ram, PR_U1C_0AED_RAM_SIZE,
                                   &cpu, external, &f) == PR_U1C_0AED_OK);
    assert(f.calls[0] == 1u && f.calls[1] == 1u && f.calls[2] == 4u);
    assert(ram[physical(cpu.ds, expected_flag)] == 1u);
    assert(ram[physical(cpu.ds, expected_flag == 0x6F78u ? 0x6F79u : 0x6F78u)] == 0u);
    assert(cpu.bp == 0x7777u && cpu.sp == 0x8004u);
    assert(cpu.cs == 0xD000u && cpu.ip == 0x0712u);
    free(ram);
}

int main(void)
{
    const uint16_t equal[4] = {0x1234u, 0x1234u, 0x5678u, 0x5678u};
    const uint16_t first_different[4] = {0u, 1u, 2u, 2u};
    const uint16_t second_different[4] = {0u, 0u, 2u, 3u};
    run(equal, 0x6F78u);
    run(first_different, 0x6F79u);
    run(second_different, 0x6F79u);
    return 0;
}
