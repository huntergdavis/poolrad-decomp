#include "poolrad/recovered/overlay_u3c2_signum_0000.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static void put(uint8_t *ram, size_t at, uint16_t value)
{
    ram[at] = (uint8_t)value;
    ram[at + 1u] = (uint8_t)(value >> 8u);
}
static uint16_t get(const uint8_t *ram, size_t at)
{
    return (uint16_t)(ram[at] | ((uint16_t)ram[at + 1u] << 8u));
}
static pr_u3c2_0000_cpu run(uint8_t *ram, uint16_t value)
{
    pr_u3c2_0000_cpu cpu = {0xABCD, 0x3456, 0x5678, 0x789A,
        0x9ABC, 0xBCDE, 0x7777, 0x8000, 0xF000, 0,
        0x1000, 0x1111, 0x2000, 0x0ED7};
    put(ram, 0x28000, 0x0712);
    put(ram, 0x28002, 0xF000);
    put(ram, 0x28004, value);
    assert(pr_u3c2_signum_0000(ram, PR_U3C2_0000_RAM_SIZE, &cpu) == PR_U3C2_0000_OK);
    assert(cpu.ax == (value == 0u ? 0u : value < 0x8000u ? 1u : 0xFFFFu));
    assert(cpu.bx == 0x3456 && cpu.cx == 0x5678 && cpu.dx == 0x789A);
    assert(cpu.si == 0x9ABC && cpu.di == 0xBCDE && cpu.bp == 0x7777);
    assert(cpu.sp == 0x8006 && cpu.cs == 0xF000 && cpu.ip == 0x0712);
    assert(cpu.ds == 0x1000 && cpu.es == 0x1111 && cpu.ss == 0x2000);
    assert(get(ram, 0x27FFC) == cpu.ax && get(ram, 0x27FFE) == 0x7777);
    assert(get(ram, 0x28004) == value);
    assert(ram[0x27FFB] == 0xA5 && ram[0x28006] == 0xA5);
    return cpu;
}
int main(void)
{
    static const uint16_t values[] = {0, 1, 3, 0x7FFF, 0x8000, 0x8001, 0xFFFF, 0xFFFE};
    static const uint16_t expected_flags[] = {0x0646, 0x0602, 0x0606, 0x0606,
        0x0686, 0x0682, 0x0686, 0x0682};
    uint8_t *ram = malloc(PR_U3C2_0000_RAM_SIZE);
    unsigned value;
    size_t i;
    pr_u3c2_0000_cpu cpu;
    assert(ram);
    memset(ram, 0xA5, PR_U3C2_0000_RAM_SIZE);
    for (value = 0; value < 65536u; ++value) (void)run(ram, (uint16_t)value);
    for (i = 0; i < sizeof(values) / sizeof(values[0]); ++i) {
        cpu = run(ram, values[i]);
        assert(cpu.flags == expected_flags[i]); /* CMP input flags, not AX result. */
    }
    /* Distinct wrapped CS words and linear-shadow bytes catch the
     * RETF2 SP=FFFE/FFFF regression independently of the emulator. */
    for (i = 0; i < 2; ++i) {
        uint16_t sp = (uint16_t)(0xFFFEu + i);
        cpu = run(ram, 6);
        memset(ram, 0xA5, PR_U3C2_0000_RAM_SIZE);
        cpu.ax = 0xABCD;cpu.ip = 0;cpu.sp = sp;
        put(ram, 0x20000u + sp, 0x0712);
        put(ram, 0x20000u + (uint16_t)(sp + 2u), 0xF000);
        put(ram, 0x20000u + (uint16_t)(sp + 4u), 6);
        assert(pr_u3c2_signum_0000(ram, PR_U3C2_0000_RAM_SIZE, &cpu) == PR_U3C2_0000_OK);
        assert(cpu.cs == 0xF000 && cpu.ip == 0x0712 && cpu.sp == (uint16_t)(sp + 6u));
        assert(cpu.ax == 1 && cpu.bp == 0x7777);
    }
    assert(pr_u3c2_signum_0000(NULL, PR_U3C2_0000_RAM_SIZE, &cpu) == PR_U3C2_0000_ARGUMENT);
    assert(pr_u3c2_signum_0000(ram, PR_U3C2_0000_RAM_SIZE, NULL) == PR_U3C2_0000_ARGUMENT);
    assert(pr_u3c2_signum_0000(ram, PR_U3C2_0000_RAM_SIZE - 1u, &cpu) == PR_U3C2_0000_ARGUMENT);
    free(ram);
    return 0;
}
