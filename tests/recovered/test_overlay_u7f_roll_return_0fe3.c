#include "poolrad/recovered/overlay_u7f_roll_return_0fe3.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static size_t physical(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & (PR_U7F_0FE3_RAM_SIZE - 1u);
}

static void put16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t value)
{
    size_t at = physical(seg, off);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & (PR_U7F_0FE3_RAM_SIZE - 1u)] = (uint8_t)(value >> 8);
}

static void one(uint8_t *ram, uint16_t bp, uint16_t ss, uint8_t roll,
                uint8_t expected)
{
    pr_u7f_0fe3_cpu cpu = { (uint16_t)(0xA500u | roll), 0x3456u, 0x5678u,
        0x789Au, 0x9ABCu, 0xBCDEu, bp, 0x1111u, 0x3000u, 0x0FE3u,
        0x1000u, 0x2222u, ss, 0x7202u };
    memset(ram, 0xA5, PR_U7F_0FE3_RAM_SIZE);
    put16(ram, ss, bp, 0x7777u);
    put16(ram, ss, (uint16_t)(bp + 2u), 0x2468u);
    put16(ram, ss, (uint16_t)(bp + 4u), 0x1357u);
    assert(pr_u7f_roll_return_0fe3(ram, PR_U7F_0FE3_RAM_SIZE, &cpu)
           == PR_U7F_0FE3_RETURNED);
    assert(ram[physical(ss, (uint16_t)(bp - 2u))] == roll);
    assert(ram[physical(ss, (uint16_t)(bp - 1u))] == expected);
    assert(cpu.ax == (uint16_t)(0xA500u | expected));
    assert(cpu.bp == 0x7777u && cpu.sp == (uint16_t)(bp + 6u));
    assert(cpu.cs == 0x1357u && cpu.ip == 0x2468u);
    assert(cpu.bx == 0x3456u && cpu.cx == 0x5678u && cpu.dx == 0x789Au);
}

int main(void)
{
    uint8_t *ram = malloc(PR_U7F_0FE3_RAM_SIZE);
    pr_u7f_0fe3_cpu cpu = {0}, before = cpu;
    static const struct { uint8_t roll, tier; } cases[] = {
        {0u,3u}, {1u,1u}, {14u,1u}, {15u,2u}, {19u,2u},
        {20u,3u}, {255u,3u}
    };
    size_t i;
    assert(ram != NULL);
    assert(pr_u7f_roll_return_0fe3(NULL, PR_U7F_0FE3_RAM_SIZE, &cpu)
           == PR_U7F_0FE3_ARGUMENT);
    assert(pr_u7f_roll_return_0fe3(ram, 0u, &cpu) == PR_U7F_0FE3_ARGUMENT);
    assert(pr_u7f_roll_return_0fe3(ram, PR_U7F_0FE3_RAM_SIZE, NULL)
           == PR_U7F_0FE3_ARGUMENT);
    assert(memcmp(&cpu, &before, sizeof cpu) == 0);
    for (i = 0u; i < sizeof cases / sizeof cases[0]; ++i)
        one(ram, 0x8000u, 0x2000u, cases[i].roll, cases[i].tier);
    one(ram, 0x0000u, 0x2000u, 14u, 1u);
    one(ram, 0xFFFCu, 0x2000u, 1u, 1u);
    one(ram, 0xFFFDu, 0x2000u, 19u, 2u);
    one(ram, 0xFFFEu, 0x2000u, 15u, 2u);
    one(ram, 0xFFFFu, 0xFFFFu, 20u, 3u);
    free(ram);
    return 0;
}
