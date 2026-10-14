#include "poolrad/recovered/overlay_u33_byte_flags_08a4.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static size_t address(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & (PR_U33_08A4_RAM_SIZE - 1u);
}

static void put16(uint8_t *ram, uint16_t segment, uint16_t offset, uint16_t value)
{
    size_t at = address(segment, offset);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & (PR_U33_08A4_RAM_SIZE - 1u)] = (uint8_t)(value >> 8);
}

static pr_u33_08a4_cpu prepare(uint8_t *ram, uint8_t input, uint8_t old,
                              uint16_t window)
{
    pr_u33_08a4_cpu cpu = {
        0xABCDu, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu,
        0x7777u, 0x8000u, 0xF000u, 0x08A4u, 0x1000u, 0x1111u,
        0x2000u, 0x0202u
    };
    memset(ram, 0, PR_U33_08A4_RAM_SIZE);
    ram[address(cpu.ds, 0x465Au)] = input;
    ram[address(cpu.ss, 0x7FFCu)] = old;
    put16(ram, cpu.ds, 0x49D2u, 0x1000u);
    put16(ram, cpu.ds, 0x49D4u, 0x4000u);
    put16(ram, 0x4000u, 0x1366u, window);
    put16(ram, cpu.ss, 0x8000u, 0x0712u);
    put16(ram, cpu.ss, 0x8002u, cpu.cs);
    return cpu;
}

int main(void)
{
    uint8_t *ram = malloc(PR_U33_08A4_RAM_SIZE);
    pr_u33_08a4_cpu cpu;
    assert(ram);
    cpu = prepare(ram, 1, 0xA5u, 0xFEu);
    assert(pr_u33_byte_flags_08a4(NULL, PR_U33_08A4_RAM_SIZE, &cpu) == PR_U33_08A4_ARGUMENT);
    assert(pr_u33_byte_flags_08a4(ram, PR_U33_08A4_RAM_SIZE, NULL) == PR_U33_08A4_ARGUMENT);
    assert(pr_u33_byte_flags_08a4(ram, 1, &cpu) == PR_U33_08A4_ARGUMENT);
    assert(pr_u33_byte_flags_08a4(ram, PR_U33_08A4_RAM_SIZE, &cpu) == PR_U33_08A4_OK);
    assert(cpu.ax == 0xAB01u && cpu.di == 0xBCDEu && cpu.es == 0x1111u);
    assert(cpu.sp == 0x8004u && cpu.bp == 0x7777u && cpu.cs == 0xF000u && cpu.ip == 0x0712u);
    assert(cpu.flags == 0x0246u);
    assert(ram[address(cpu.ss, 0x7FFCu)] == 1 && ram[address(cpu.ss, 0x7FFDu)] == 1);

    cpu = prepare(ram, 0x7Cu, 0, 0xFFu); /* earlier range wins over later ==7C */
    assert(pr_u33_byte_flags_08a4(ram, PR_U33_08A4_RAM_SIZE, &cpu) == PR_U33_08A4_OK);
    assert(cpu.ax == 0xAB08u);
    cpu = prepare(ram, 0xF5u, 0, 0xFFu); /* earlier 20 wins over F4..F6 ->80 */
    assert(pr_u33_byte_flags_08a4(ram, PR_U33_08A4_RAM_SIZE, &cpu) == PR_U33_08A4_OK);
    assert(cpu.ax == 0xAB20u && cpu.es == 0x1111u);

    cpu = prepare(ram, 0, 0xA5u, 0xFEu); /* unmatched: actual old local */
    assert(pr_u33_byte_flags_08a4(ram, PR_U33_08A4_RAM_SIZE, &cpu) == PR_U33_08A4_OK);
    assert(cpu.ax == 0xABA5u && cpu.es == 0x4000u && cpu.di == 0x1000u);
    assert(cpu.flags == 0x0297u); /* defined CMP AF, CF, SF and PF */
    cpu = prepare(ram, 0, 0xA5u, 0xFFu);
    assert(pr_u33_byte_flags_08a4(ram, PR_U33_08A4_RAM_SIZE, &cpu) == PR_U33_08A4_OK);
    assert(cpu.ax == 0xAB25u);

    cpu = prepare(ram, 0x57u, 0, 0xFFu);
    cpu.ss = cpu.ds;
    cpu.sp = 0x49D6u;
    put16(ram, cpu.ss, cpu.sp, 0x0712u);
    put16(ram, cpu.ss, 0x49D8u, 0xF000u);
    put16(ram, 0x7777u, 0x13EEu, 0xFFu);
    assert(pr_u33_byte_flags_08a4(ram, PR_U33_08A4_RAM_SIZE, &cpu) == PR_U33_08A4_OK);
    assert(cpu.es == 0x7777u && cpu.di == 0x1088u); /* PUSH BP + local write */
    assert(cpu.ax == 0xAB09u && cpu.bp == 0x7777u);
    assert(ram[address(cpu.ds, 0x49D2u)] == 9 && ram[address(cpu.ds, 0x49D3u)] == 9);
    free(ram);
    return 0;
}
