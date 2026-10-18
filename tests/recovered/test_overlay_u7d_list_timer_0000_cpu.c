/* Focused synthetic test for raw-CPU U7D +0000 up to its U9E +0026 call (docs/re/U7D_LIST_TIMER_0000_CPU.md). No game
 * bytes beyond the two LCALL operand sets: a synthetic party, flags and node list, every result predicted by hand. */
#include "poolrad/recovered/overlay_u7d_list_timer_0000_cpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM PR_U7D_0000_RAM_SIZE
#define CHECK(x)                                                                  \
    do {                                                                          \
        if (!(x)) {                                                               \
            fprintf(stderr, "u7d 0000 line %d: %s\n", __LINE__, #x);              \
            exit(1);                                                              \
        }                                                                         \
        ++checks;                                                                 \
    } while (0)

enum { CS0 = 0x8800, DS0 = 0x1800, SS0 = 0x2800, SP0 = 0x4000, BP0 = 0x7777, RET_IP = 0x7000, RET_CS = 0x9001,
       MSEG = 0x3000, MOFF = 0x0100, NSEG = 0x5000 };

static uint8_t *ram;
static unsigned checks;

static size_t at(unsigned seg, unsigned off) { return (((size_t)seg << 4) + (off & 0xFFFFu)) & (RAM - 1u); }
static unsigned get16(unsigned seg, unsigned off) { return ram[at(seg, off)] | ((unsigned)ram[at(seg, off + 1u)] << 8); }
static void put16(unsigned seg, unsigned off, unsigned value)
{
    ram[at(seg, off)] = (uint8_t)value;
    ram[at(seg, off + 1u)] = (uint8_t)(value >> 8);
}

/* Mode byte, member 1 flagged, one member (link `next`) whose list is one node of type 7 and `duration`; u, n. */
static pr_u7d_0000_cpu world(unsigned mode, unsigned duration, int cyclic, unsigned u, unsigned n)
{
    static const uint8_t fill[5] = {0x9A, 0xA8, 0x16, 0xF8, 0x0A}, u9e[5] = {0x9A, 0x2A, 0x00, 0xB0, 0x00};
    pr_u7d_0000_cpu cpu;
    unsigned i;
    memset(ram, 0xA5, RAM);
    memcpy(ram + at(CS0, 0x0019), fill, 5);
    memcpy(ram + at(CS0, 0x01FE), u9e, 5);
    ram[at(DS0, 0x49F3)] = (uint8_t)mode;
    for (i = 0; i < 0x60u; ++i)
        ram[at(DS0, 0x471D + i)] = 0;
    ram[at(DS0, 0x471E)] = 1;
    put16(DS0, 0x49D6, 0x0000); put16(DS0, 0x49D8, 0x6000); put16(0x6000, 0x067C, 1);
    for (i = 0; i < 7u; ++i)
        put16(DS0, 0x363A + 2u * i, 1);
    put16(DS0, 0x5D96, MOFF); put16(DS0, 0x5D98, MSEG);
    put16(MSEG, MOFF + 0x104u, cyclic ? MOFF : 0u); put16(MSEG, MOFF + 0x106u, cyclic ? MSEG : 0u);
    put16(MSEG, MOFF + 0x7Fu, 0x0000); put16(MSEG, MOFF + 0x81u, NSEG);
    ram[at(NSEG, 0)] = 7; put16(NSEG, 1, duration); put16(NSEG, 5, 0); put16(NSEG, 7, 0);
    put16(SS0, SP0, RET_IP); put16(SS0, SP0 + 2u, RET_CS); put16(SS0, SP0 + 4u, 0x9900u | u); put16(SS0, SP0 + 6u, 0x9900u | n);
    memset(&cpu, 0, sizeof cpu);
    cpu.ax = 0x1111; cpu.bx = 0x2222; cpu.cx = 0x3333; cpu.dx = 0x4444; cpu.si = 0x5555; cpu.di = 0x6666; cpu.bp = BP0;
    cpu.sp = SP0; cpu.cs = CS0; cpu.ip = 0x0000; cpu.ds = DS0; cpu.es = 0x9999; cpu.ss = SS0; cpu.flags = 0x0602;
    return cpu;
}

int main(void)
{
    pr_u7d_0000_cpu cpu;
    unsigned i, filled = 1;
    ram = malloc(RAM);
    if (ram == NULL)
        return 1;

    /* n = 25 in steps 10, 10, 5: the node goes 100 -> 75 and keeps the flag; amount 0 returns with CMP's ZF/PF. */
    cpu = world(2, 100, 0, 1, 25);
    CHECK(pr_u7d_list_timer_0000_cpu(ram, RAM, &cpu, 1000) == PR_U7D_0000_OK);
    CHECK(get16(NSEG, 1) == 75 && ram[at(DS0, 0x471E)] == 1 && get16(SS0, SP0 - 7u) == 0);
    CHECK(cpu.cs == RET_CS && cpu.ip == RET_IP && cpu.sp == SP0 + 8u && cpu.bp == BP0 && cpu.ax == 0 && cpu.flags == 0x0646);

    /* A node at 7 with a step of 10 reaches +01FE: member, type (AH from the link word), node pushed; CMP 10,7 leaves PF. */
    cpu = world(2, 7, 0, 1, 10);
    CHECK(pr_u7d_list_timer_0000_cpu(ram, RAM, &cpu, 1000) == PR_U7D_0000_PENDING);
    CHECK(cpu.ip == 0x01FE && cpu.cs == CS0 && cpu.sp == SP0 - 0x28u && cpu.bp == SP0 - 2u && cpu.flags == 0x0606);
    CHECK(get16(SS0, cpu.sp) == 0x0000 && get16(SS0, cpu.sp + 2u) == NSEG && get16(SS0, cpu.sp + 4u) == 0x0007);
    CHECK(get16(SS0, cpu.sp + 6u) == MOFF && get16(SS0, cpu.sp + 8u) == MSEG && get16(NSEG, 1) == 7 && ram[at(DS0, 0x471E)] == 0);

    /* No flag set: the scan ends at i = 2 > count 1 and returns with CMP BYTE [BP-0Bh],0's flags. */
    cpu = world(2, 100, 0, 1, 25);
    ram[at(DS0, 0x471E)] = 0;
    CHECK(pr_u7d_list_timer_0000_cpu(ram, RAM, &cpu, 1000) == PR_U7D_0000_OK);
    CHECK(get16(NSEG, 1) == 100 && cpu.flags == 0x0646 && cpu.sp == SP0 + 8u);

    /* A member linked to itself: after the node pass, three JMP 00BA loopbacks (cap 3) stop at the fourth with i = 4;
     * INC's flags over CMP's CF = 0. */
    cpu = world(2, 100, 1, 1, 25);
    CHECK(pr_u7d_list_timer_0000_cpu(ram, RAM, &cpu, 3) == PR_U7D_0000_LIMIT);
    CHECK(cpu.ip == 0x00BA && ram[at(SS0, SP0 - 4u)] == 4 && get16(NSEG, 1) == 90 && cpu.flags == 0x0602);

    /* CODE at the FillChar LCALL (mode 0): four words pushed; CMP 0,2 leaves CF, AF and SF. */
    cpu = world(0, 100, 0, 1, 25);
    ram[at(CS0, 0x001A)] = 0;
    CHECK(pr_u7d_list_timer_0000_cpu(ram, RAM, &cpu, 1000) == PR_U7D_0000_CODE);
    CHECK(cpu.ip == 0x0019 && cpu.sp == SP0 - 0x26u && cpu.flags == 0x0693 && get16(SS0, cpu.sp) == 1 && get16(SS0, cpu.sp + 2u) == 0x48);

    /* Mode 0 runs FillChar(DS:471E, 48h, 1) and then the same three passes. */
    cpu = world(0, 100, 0, 1, 25);
    CHECK(pr_u7d_list_timer_0000_cpu(ram, RAM, &cpu, 1000) == PR_U7D_0000_OK);
    for (i = 0; i < 0x48u; ++i)
        filled &= ram[at(DS0, 0x471E + i)] == 1;
    CHECK(filled && ram[at(DS0, 0x4766)] == 0 && get16(NSEG, 1) == 75 && cpu.ip == RET_IP);

    /* Guards. */
    cpu = world(2, 100, 0, 1, 25);
    cpu.ip = 1;
    CHECK(pr_u7d_list_timer_0000_cpu(ram, RAM, &cpu, 1000) == PR_U7D_0000_ARGUMENT && cpu.sp == SP0);
    CHECK(pr_u7d_list_timer_0000_cpu(NULL, RAM, &cpu, 1000) == PR_U7D_0000_ARGUMENT);
    CHECK(pr_u7d_list_timer_0000_cpu(ram, RAM - 1u, &cpu, 1000) == PR_U7D_0000_ARGUMENT);

    free(ram);
    printf("u7d +0000: %u checks passed\n", checks);
    return 0;
}
