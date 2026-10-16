#include "poolrad/recovered/overlay_u7d_carry_02f2.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); exit(1); } } while (0)
static void put(uint8_t *ram, size_t at, uint16_t value)
{
    ram[at] = (uint8_t)value;
    ram[at + 1u] = (uint8_t)(value >> 8u);
}
static uint16_t word(const uint8_t *ram, size_t at)
{
    return (uint16_t)(ram[at] | ((uint16_t)ram[at + 1u] << 8u));
}
static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & (PR_U7D_02F2_RAM_SIZE - 1u);
}
static void check_retf_wrap(uint8_t *ram, uint16_t sp)
{
    unsigned i;
    pr_u7d_02f2_cpu cpu = {0x1234,0x3456,0x5678,0x789A,0x9ABC,
        0xBCDE,0x7777,sp,0x3000,0x02F2,0x1000,0x1111,0x2000,0x7603};
    size_t linear_cs = (physical(cpu.ss, sp) + 2u) & (PR_U7D_02F2_RAM_SIZE - 1u);
    memset(ram, 0xA5, PR_U7D_02F2_RAM_SIZE);
    for (i = 0; i < 7u; ++i) {
        put(ram, physical(cpu.ds, (uint16_t)(0x363Au + 2u*i)), 0xFFFFu);
        put(ram, physical(0x6000u, (uint16_t)(0x0100u + 2u*i)), 0u);
    }
    put(ram, physical(cpu.ss, sp), 0x7000u);
    put(ram, physical(cpu.ss, (uint16_t)(sp + 2u)), 0x3000u);
    put(ram, physical(cpu.ss, (uint16_t)(sp + 4u)), 0x0100u);
    put(ram, physical(cpu.ss, (uint16_t)(sp + 6u)), 0x6000u);
    put(ram, linear_cs, 0xD00Du);
    CHECK(word(ram, physical(cpu.ss, (uint16_t)(sp + 2u))) == 0x3000u);
    CHECK(word(ram, linear_cs) == 0xD00Du);
    CHECK(pr_u7d_carry_02f2(ram, PR_U7D_02F2_RAM_SIZE, &cpu) == PR_U7D_02F2_OK);
    CHECK(cpu.ip == 0x7000u && cpu.cs == 0x3000u);
    CHECK(cpu.sp == (uint16_t)(sp + 8u) && cpu.bp == 0x7777u);
    CHECK(cpu.flags == 0x7646u);
}
int main(void)
{
    uint8_t *ram = malloc(PR_U7D_02F2_RAM_SIZE);
    unsigned mask, mixed, cases = 0;
    CHECK(ram != NULL);
    for (mixed = 0; mixed < 2u; ++mixed) for (mask = 0; mask < 128u; ++mask) {
        unsigned i, n = mask % 7u;
        uint16_t expected[7], before[6];
        pr_u7d_02f2_cpu cpu = {0x1234,0x3456,0x5678,0x789A,0x9ABC,
            0xBCDE,0x7777,0x8000,0x3000,0x02F2,0x1000,0x1111,0x2000,0x7603};
        memset(ram, 0xA5, PR_U7D_02F2_RAM_SIZE);
        for (i = 0; i < 7u; ++i) {
            expected[i] = (uint16_t)((mask & (1u << i)) ? i + 3u : 0u);
            put(ram, 0x1363A + 2u*i, (uint16_t)(i + 3u));
        }
        for (i = 0; i < 6u; ++i) {
            size_t at = 0x40100u + 0x200u*i + (mixed && (i % 2u) ? 0x10000u : 0u);
            before[i] = i == 0u ? 0xFFFFu : (uint16_t)(0x7FFCu + i);
            put(ram, at + 0x30u, before[i]);
            put(ram, at + 0x104u, i + 1u < n ? (uint16_t)(0x100u + 0x200u*(i+1u)) : 0u);
            put(ram, at + 0x106u, i + 1u < n ? (uint16_t)(0x4000u + (mixed && ((i+1u)%2u) ? 0x1000u : 0u)) : 0u);
        }
        put(ram, 0x15D96, n ? 0x100u : 0u); put(ram, 0x15D98, n ? 0x4000u : 0u);
        /* The numeric array and records are disjoint. */
        for (i = 0; i < 7u; ++i) put(ram, 0x60000u + 2u*i, expected[i]);
        put(ram, 0x28000, 0x7000); put(ram, 0x28002, 0x3000);
        put(ram, 0x28004, 0u); put(ram, 0x28006, 0x6000);
        for (i = 0; i < 6u; ++i) if (mask & (1u << i)) {
            expected[i] = (uint16_t)(expected[i] - (i + 3u));
            expected[i + 1u] = (uint16_t)(expected[i + 1u] + 1u);
        }
        CHECK(pr_u7d_carry_02f2(ram, PR_U7D_02F2_RAM_SIZE, &cpu) == PR_U7D_02F2_OK);
        for (i = 0; i < 7u; ++i) CHECK(word(ram, 0x60000u + 2u*i) == expected[i]);
        for (i = 0; i < 6u; ++i) {
            size_t at = 0x40100u + 0x200u*i + (mixed && (i % 2u) ? 0x10000u : 0u);
            CHECK(word(ram, at + 0x30u) == (uint16_t)(before[i] + ((i < n && (mask & 64u)) ? 1u : 0u)));
        }
        CHECK(word(ram, 0x15D96) == (n ? 0x100u : 0u));
        CHECK(cpu.bx == 0x3456 && cpu.cx == 0x5678 && cpu.si == 0x9ABC);
        CHECK(cpu.bp == 0x7777 && cpu.sp == 0x8008 && cpu.cs == 0x3000 && cpu.ip == 0x7000);
        CHECK(cpu.ds == 0x1000 && cpu.ss == 0x2000 && cpu.flags == 0x7646);
        ++cases;
    }
    {
        pr_u7d_02f2_cpu cpu = {0}, before = cpu;
        CHECK(pr_u7d_carry_02f2(NULL, PR_U7D_02F2_RAM_SIZE, &cpu) == PR_U7D_02F2_ARGUMENT);
        CHECK(pr_u7d_carry_02f2(ram, 0u, &cpu) == PR_U7D_02F2_ARGUMENT);
        CHECK(pr_u7d_carry_02f2(ram, PR_U7D_02F2_RAM_SIZE, NULL) == PR_U7D_02F2_ARGUMENT);
        CHECK(memcmp(&before, &cpu, sizeof cpu) == 0);
    }
    check_retf_wrap(ram, 0xFFFEu);
    check_retf_wrap(ram, 0xFFFFu);
    free(ram);
    printf("u7D +02F2: %u independent carry patterns and host guards passed\n", cases);
    return 0;
}
