#include "poolrad/recovered/overlay_u3c6_in_range_06e9.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & (PR_U3C6_06E9_RAM_SIZE - 1u);
}
static void put16(uint8_t *ram, uint16_t segment, uint16_t offset, uint16_t value)
{
    size_t at = physical(segment, offset);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & (PR_U3C6_06E9_RAM_SIZE - 1u)] = (uint8_t)(value >> 8u);
}
static pr_u3c6_06e9_cpu initial(void)
{
    pr_u3c6_06e9_cpu cpu = {
        0xABCDu,0x3456u,0x5678u,0x789Au,0x9ABCu,0xBCDEu,
        0x7777u,0x8000u,0xF000u,0x06E9u,0x1000u,0x1111u,0x2000u,0x0ED7u
    };
    return cpu;
}
static void arguments(uint8_t *ram, unsigned a, unsigned b)
{
    put16(ram, 0x2000u, 0x8000u, 0x0712u);
    put16(ram, 0x2000u, 0x8002u, 0xF000u);
    put16(ram, 0x2000u, 0x8004u, (uint16_t)(0xA500u | a));
    put16(ram, 0x2000u, 0x8006u, (uint16_t)(0x5A00u | b));
}
int main(void)
{
    uint8_t *ram = malloc(PR_U3C6_06E9_RAM_SIZE);
    pr_u3c6_06e9_cpu cpu = initial();
    unsigned a,b,i;
    const uint8_t aa[] = {0,6,7,128,255,6,5,127};
    const uint8_t bb[] = {0,6,6,6,0,128,6,7};
    const uint16_t expected_flags[] = {0x0697u,0x0646u,0x0602u,0x0682u,0x0686u,0x0682u,0x0697u,0x0602u};
    assert(ram);
    memset(ram, 0xA5, PR_U3C6_06E9_RAM_SIZE);
    assert(pr_u3c6_in_range_06e9(NULL, PR_U3C6_06E9_RAM_SIZE, &cpu) == PR_U3C6_06E9_ARGUMENT);
    assert(pr_u3c6_in_range_06e9(ram, 1, &cpu) == PR_U3C6_06E9_ARGUMENT);
    assert(pr_u3c6_in_range_06e9(ram, PR_U3C6_06E9_RAM_SIZE, NULL) == PR_U3C6_06E9_ARGUMENT);
    assert(cpu.sp == 0x8000u && ram[physical(cpu.ss, 0x7FFEu)] == 0xA5u);
    for (a = 0; a < 256; ++a) for (b = 0; b < 256; ++b) {
        int signed_a = a < 128 ? (int)a : (int)a - 256;
        int signed_b = b < 128 ? (int)b : (int)b - 256;
        unsigned expected = signed_a >= 0 && signed_a <= 6 && signed_b >= 0 && signed_b <= 6;
        uint16_t ah = (uint16_t)((a ^ b) << 8u);
        arguments(ram, a, b);
        cpu = initial();
        cpu.ax = (uint16_t)(ah | 0xCDu);
        assert(pr_u3c6_in_range_06e9(ram, PR_U3C6_06E9_RAM_SIZE, &cpu) == PR_U3C6_06E9_OK);
        assert(cpu.ax == (uint16_t)(ah | expected) && ram[physical(cpu.ss, 0x7FFDu)] == expected);
        assert(ram[0x27FFC] == 0xA5u && ram[0x28008] == 0xA5u);
        assert(ram[0x27FFE] == 0x77u && ram[0x27FFF] == 0x77u);
        assert(ram[0x28005] == 0xA5u && ram[0x28007] == 0x5Au);
        assert(cpu.sp == 0x8008u && cpu.bp == 0x7777u && cpu.cs == 0xF000u && cpu.ip == 0x0712u);
        assert(cpu.bx == 0x3456u && cpu.cx == 0x5678u && cpu.dx == 0x789Au && cpu.si == 0x9ABCu);
        assert(cpu.di == 0xBCDEu && cpu.es == 0x1111u && cpu.ds == 0x1000u && cpu.ss == 0x2000u);
    }
    for (i = 0; i < 8; ++i) {
        arguments(ram, aa[i], bb[i]);cpu = initial();
        assert(pr_u3c6_in_range_06e9(ram, PR_U3C6_06E9_RAM_SIZE, &cpu) == PR_U3C6_06E9_OK);
        assert(cpu.flags == expected_flags[i]);
    }
    /* RETF performs two word pops, each with a wrapped 16-bit SP.
     * Keep SS:0000/0001 different from the linear bytes above FFFF. */
    for (i = 0; i < 2; ++i) {
        uint16_t sp = (uint16_t)(0xFFFEu + i);
        memset(ram, 0xA5, PR_U3C6_06E9_RAM_SIZE);
        cpu = initial();cpu.sp = sp;
        put16(ram, cpu.ss, sp, 0x0712u);
        put16(ram, cpu.ss, (uint16_t)(sp + 2u), 0xF000u);
        put16(ram, cpu.ss, (uint16_t)(sp + 4u), 6u);
        put16(ram, cpu.ss, (uint16_t)(sp + 6u), 6u);
        assert(pr_u3c6_in_range_06e9(ram, PR_U3C6_06E9_RAM_SIZE, &cpu) == PR_U3C6_06E9_OK);
        assert(cpu.cs == 0xF000u && cpu.ip == 0x0712u && cpu.sp == (uint16_t)(sp + 8u));
        assert(cpu.ax == 0xAB01u && cpu.bp == 0x7777u);
    }
    free(ram);
    return 0;
}
