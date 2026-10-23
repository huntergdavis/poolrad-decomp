#include "poolrad/recovered/overlay_u5e_half_0e52.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { \
    fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); \
} } while (0)

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & (PR_U5E_0E52_RAM_SIZE - 1u);
}

static void put_word(uint8_t *ram, uint16_t segment, uint16_t offset,
                     uint16_t value)
{
    size_t at = physical(segment, offset);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & (PR_U5E_0E52_RAM_SIZE - 1u)] = (uint8_t)(value >> 8);
}

static pr_u5e_0e52_cpu initial(uint8_t *ram, uint16_t ss, uint16_t sp)
{
    pr_u5e_0e52_cpu cpu = {
        0xBEEF, 0x1234, 0x2345, 0x3456, 0x4567, 0x5678,
        0x6789, sp, 0x3000, 0x0E52, 0x1000, 0x789A, ss, 0x0ED7
    };
    put_word(ram, ss, sp, 0x4321);
    put_word(ram, ss, (uint16_t)(sp + 2u), 0xABCD);
    put_word(ram, ss, (uint16_t)(sp + 4u), 0x5A00);
    return cpu;
}

int main(void)
{
    uint8_t *ram = malloc(PR_U5E_0E52_RAM_SIZE);
    unsigned input, flag, i;
    CHECK(ram != NULL);
    memset(ram, 0xA5, PR_U5E_0E52_RAM_SIZE);
    for (input = 0u; input < 256u; ++input) {
        for (flag = 0u; flag < 256u; ++flag) {
            unsigned adjusted = (input + (flag % 2u)) % 256u;
            unsigned result = adjusted / 2u;
            pr_u5e_0e52_cpu cpu = initial(ram, 0x2000, 0x8000);
            ram[physical(cpu.ss, 0x8004)] = (uint8_t)input;
            ram[physical(cpu.ds, 0x6DE3)] = (uint8_t)flag;
            CHECK(pr_u5e_half_0e52(ram, PR_U5E_0E52_RAM_SIZE, &cpu) ==
                  PR_U5E_0E52_OK);
            CHECK(cpu.ax == result && cpu.bp == 0x6789 && cpu.sp == 0x8006);
            CHECK(cpu.cs == 0xABCD && cpu.ip == 0x4321);
            CHECK(cpu.bx == 0x1234 && cpu.cx == 0x2345 && cpu.dx == 0x3456);
            CHECK(cpu.si == 0x4567 && cpu.di == 0x5678 && cpu.es == 0x789A);
            CHECK(cpu.ds == 0x1000 && cpu.ss == 0x2000);
            CHECK((cpu.flags & 1u) == (adjusted % 2u));
            CHECK(((cpu.flags & 0x40u) != 0u) == (result == 0u));
            CHECK((cpu.flags & 0x0880u) == 0u);   /* OF, SF clear: AH was zeroed before the SHR */
            CHECK((cpu.flags & 0x0010u) != 0u);   /* pinned DOSBox-X: AF set after a non-zero SHR */
            CHECK((cpu.flags & (uint16_t)~0x08D5u) ==
                  (0x0ED7u & (uint16_t)~0x08D5u));
            CHECK(ram[physical(cpu.ss, 0x7FFC)] == adjusted);
            CHECK(ram[physical(cpu.ss, 0x7FFD)] == result);
            CHECK(ram[physical(cpu.ss, 0x7FFB)] == 0xA5);
            CHECK(ram[physical(cpu.ss, 0x8006)] == 0xA5);
        }
    }
    /* Both the stack offset and the physical A20 boundary can wrap. */
    for (i = 0u; i < 5u; ++i) {
        static const uint16_t stack[] = {0u, 1u, 2u, 0xFFFFu, 0x0011u};
        pr_u5e_0e52_cpu cpu;
        memset(ram, 0xA5, PR_U5E_0E52_RAM_SIZE);
        cpu = initial(ram, i == 4u ? 0xFFFFu : 0x2000u, stack[i]);
        ram[physical(cpu.ss, (uint16_t)(cpu.sp + 4u))] = 255u;
        ram[physical(cpu.ds, 0x6DE3)] = 1u;
        CHECK(pr_u5e_half_0e52(ram, PR_U5E_0E52_RAM_SIZE, &cpu) == PR_U5E_0E52_OK);
        CHECK(cpu.ax == 0u && cpu.bp == 0x6789);
        /* RETF reads IP/CS as a contiguous four-byte frame, even though
         * separate caller pushes can wrap the 16-bit stack offset. */
        CHECK(cpu.cs == (stack[i] >= 0xFFFEu ? 0xA5A5u : 0xABCDu));
        CHECK(cpu.ip == 0x4321 && cpu.sp == (uint16_t)(stack[i] + 6u));
    }
    {
        pr_u5e_0e52_cpu cpu = initial(ram, 0x2000, 0x8000), before = cpu;
        CHECK(pr_u5e_half_0e52(NULL, PR_U5E_0E52_RAM_SIZE, &cpu) == PR_U5E_0E52_ARGUMENT);
        CHECK(pr_u5e_half_0e52(ram, PR_U5E_0E52_RAM_SIZE - 1u, &cpu) == PR_U5E_0E52_ARGUMENT);
        CHECK(pr_u5e_half_0e52(ram, PR_U5E_0E52_RAM_SIZE, NULL) == PR_U5E_0E52_ARGUMENT);
        CHECK(memcmp(&cpu, &before, sizeof cpu) == 0);
    }
    free(ram);
    puts("u5E +0E52: 65,536 input/flag cases, wrap cases, and host guards passed");
    return 0;
}
