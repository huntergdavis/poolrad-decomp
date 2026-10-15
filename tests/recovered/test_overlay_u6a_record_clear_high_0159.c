#include "poolrad/recovered/overlay_u6a_record_clear_high_0159.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & (PR_U6A_0159_RAM_SIZE - 1u);
}

static void put16(uint8_t *ram, uint16_t segment, uint16_t offset, uint16_t value)
{
    size_t at = physical(segment, offset);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & (PR_U6A_0159_RAM_SIZE - 1u)] = (uint8_t)(value >> 8u);
}

static pr_u6a_0159_cpu prepare(uint8_t *ram, uint16_t segment, uint16_t offset)
{
    pr_u6a_0159_cpu cpu = {
        0xABCDu, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu,
        0x7777u, 0x8000u, 0xF000u, 0x0159u, 0x1000u, 0x1111u,
        0x2000u, 0x0ED7u
    };
    memset(ram, 0xA5, PR_U6A_0159_RAM_SIZE);
    put16(ram, cpu.ss, 0x8000u, 0x0712u);
    put16(ram, cpu.ss, 0x8002u, 0xF000u);
    put16(ram, cpu.ss, 0x8004u, offset);
    put16(ram, cpu.ss, 0x8006u, segment);
    return cpu;
}

int main(void)
{
    uint8_t *ram = malloc(PR_U6A_0159_RAM_SIZE);
    static const uint8_t values[] = {0, 1, 0x7E, 0x7F, 0x80, 0x81, 0xFF};
    pr_u6a_0159_cpu cpu;
    unsigned i;
    assert(ram);
    cpu = prepare(ram, 0x4000u, 0x1000u);
    assert(pr_u6a_record_clear_high_0159(NULL, PR_U6A_0159_RAM_SIZE, &cpu) == PR_U6A_0159_ARGUMENT);
    assert(pr_u6a_record_clear_high_0159(ram, PR_U6A_0159_RAM_SIZE, NULL) == PR_U6A_0159_ARGUMENT);
    assert(pr_u6a_record_clear_high_0159(ram, 1, &cpu) == PR_U6A_0159_ARGUMENT);
    assert(cpu.sp == 0x8000u && ram[physical(cpu.ss, 0x7FFDu)] == 0xA5u);
    for (i = 0; i < 21u; ++i)
        ram[physical(0x4000u, (uint16_t)(0x1017u + i))] = values[i % 7u];
    assert(pr_u6a_record_clear_high_0159(ram, PR_U6A_0159_RAM_SIZE, &cpu) == PR_U6A_0159_OK);
    for (i = 0; i < 21u; ++i)
        assert(ram[physical(0x4000u, (uint16_t)(0x1017u + i))] == (values[i % 7u] > 0x7Fu ? 0 : values[i % 7u]));
    assert(ram[physical(0x4000u, 0x102Cu)] == 0);
    assert(ram[physical(0x4000u, 0x1016u)] == 0xA5u && ram[physical(0x4000u, 0x102Du)] == 0xA5u);
    assert(ram[physical(cpu.ss, 0x7FFCu)] == 0xA5u && ram[physical(cpu.ss, 0x7FFDu)] == 20u);
    assert(cpu.ax == 20u && cpu.di == 0x1000u && cpu.es == 0x4000u && cpu.flags == 0x0646u);
    assert(cpu.bp == 0x7777u && cpu.sp == 0x8008u && cpu.cs == 0xF000u && cpu.ip == 0x0712u);
    assert(cpu.bx == 0x3456u && cpu.cx == 0x5678u && cpu.dx == 0x789Au && cpu.si == 0x9ABCu);
    assert(cpu.ds == 0x1000u && cpu.ss == 0x2000u);

    /* Clearing the argument's low offset byte redirects every later LES. */
    cpu = prepare(ram, 0x2000u, 0x7FEDu);
    assert(pr_u6a_record_clear_high_0159(ram, PR_U6A_0159_RAM_SIZE, &cpu) == PR_U6A_0159_OK);
    assert(ram[physical(cpu.ss, 0x8004u)] == 0 && cpu.di == 0x7F00u && cpu.es == 0x2000u);
    assert(ram[physical(cpu.ss, 0x7F17u)] == 0xA5u && ram[physical(cpu.ss, 0x7F18u)] == 0);
    assert(ram[physical(cpu.ss, 0x7F2Cu)] == 0 && cpu.flags == 0x0646u);

    /* The final +2C store can replace the low saved-BP byte. */
    cpu = prepare(ram, 0x2000u, 0x7FD2u);
    assert(pr_u6a_record_clear_high_0159(ram, PR_U6A_0159_RAM_SIZE, &cpu) == PR_U6A_0159_OK);
    assert(cpu.bp == 0x7700u && cpu.sp == 0x8008u);
    free(ram);
    return 0;
}
