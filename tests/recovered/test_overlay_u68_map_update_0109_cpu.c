/* Focused synthetic test for the raw-CPU U68 +0109 map update (docs/re/U68_MAP_UPDATE_0109_CPU.md). No game bytes: a
 * synthetic map and frames, every result predicted by hand. */
#include "poolrad/recovered/overlay_u68_map_update_0109_cpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM PR_U68_0109_RAM_SIZE
#define CHECK(x)                                                                  \
    do {                                                                          \
        if (!(x)) {                                                               \
            fprintf(stderr, "u68 0109 line %d: %s\n", __LINE__, #x);              \
            exit(1);                                                              \
        }                                                                         \
        ++checks;                                                                 \
    } while (0)

enum { CS0 = 0x8800, DS0 = 0x1800, SS0 = 0x2800, SP0 = 0x4000, BP0 = 0x7777, RET_IP = 0x7000, RET_CS = 0x9001, MAP = 0x4000 };

static uint8_t *ram;
static unsigned checks;

static size_t at(unsigned seg, unsigned off) { return (((size_t)seg << 4) + (off & 0xFFFFu)) & (RAM - 1u); }
static void put16(unsigned seg, unsigned off, unsigned value)
{
    ram[at(seg, off)] = (uint8_t)value;
    ram[at(seg, off + 1u)] = (uint8_t)(value >> 8);
}

/* The map pointer DS:6A5C = MAP:base, the cell byte `cell` at base + 16y + x + 300h, and the frame x, y, s. */
static pr_u68_0109_cpu frame(unsigned base, unsigned x, unsigned y, unsigned s, uint8_t cell)
{
    pr_u68_0109_cpu cpu;
    memset(ram, 0xA5, RAM);
    put16(DS0, 0x6A5C, base); put16(DS0, 0x6A5E, MAP);
    ram[at(MAP, base + 16u * (y & 0x0Fu) + (x & 0x0Fu) + 0x300u)] = cell;
    put16(SS0, SP0, RET_IP); put16(SS0, SP0 + 2u, RET_CS);
    put16(SS0, SP0 + 4u, 0x9900u | s); put16(SS0, SP0 + 6u, 0x9900u | y); put16(SS0, SP0 + 8u, 0x9900u | x);
    memset(&cpu, 0, sizeof cpu);
    cpu.ax = 0x1111; cpu.bx = 0x2222; cpu.cx = 0x3333; cpu.dx = 0x4444; cpu.si = 0x5555; cpu.di = 0x6666; cpu.bp = BP0;
    cpu.sp = SP0; cpu.cs = CS0; cpu.ip = 0x0109; cpu.ds = DS0; cpu.es = 0x9999; cpu.ss = SS0; cpu.flags = 0x0602;
    return cpu;
}

static void returned(const pr_u68_0109_cpu *cpu)
{
    CHECK(cpu->cs == RET_CS && cpu->ip == RET_IP && cpu->sp == SP0 + 10u && cpu->bp == BP0 && cpu->ds == DS0 && cpu->si == 0x5555);
}

int main(void)
{
    static const unsigned selector[4] = {6, 4, 2, 0};
    static const uint8_t from[4] = {0xFF, 0x00, 0x00, 0xFF}, to[4] = {0x7F, 0x10, 0x04, 0xFD};
    pr_u68_0109_cpu cpu;
    unsigned k;
    ram = malloc(RAM);
    if (ram == NULL)
        return 1;

    /* (3, 4): DI = 0100h + 40h + 3; the four selectors' AND/OR; AX = 40h, DX = 3, CL = 4, BL = the new byte; ADD 140h,3 leaves
     * no arithmetic flag. */
    for (k = 0; k < 4u; ++k) {
        cpu = frame(0x0100, 3, 4, selector[k], from[k]);
        CHECK(pr_u68_map_update_0109_cpu(ram, RAM, &cpu) == PR_U68_0109_OK);
        CHECK(ram[at(MAP, 0x0443)] == to[k]);
        returned(&cpu);
        CHECK(cpu.ax == 0x0040 && cpu.dx == 3 && cpu.cx == 0x3304 && cpu.bx == (0x2200u | to[k]) && cpu.di == 0x0143 && cpu.es == MAP);
        CHECK(cpu.flags == 0x0602);
    }
    /* Selector 1: no store; AL = 1 and CMP AL,0 (1, odd parity) leaves no arithmetic flag. */
    cpu = frame(0x0100, 3, 4, 1, 0x5A);
    CHECK(pr_u68_map_update_0109_cpu(ram, RAM, &cpu) == PR_U68_0109_OK);
    CHECK(ram[at(MAP, 0x0443)] == 0x5A && cpu.ax == 0x1101 && cpu.flags == 0x0602);
    returned(&cpu);
    /* x = 16: CMP 16,15 sets only AF; AX untouched. */
    cpu = frame(0x0100, 16, 4, 6, 0x5A);
    CHECK(pr_u68_map_update_0109_cpu(ram, RAM, &cpu) == PR_U68_0109_OK);
    CHECK(cpu.ax == 0x1111 && cpu.flags == 0x0612 && cpu.es == 0x9999);
    /* y = -1: CMP FFh,0 sets SF and PF and JL leaves. */
    cpu = frame(0x0100, 3, 0xFF, 6, 0x5A);
    CHECK(pr_u68_map_update_0109_cpu(ram, RAM, &cpu) == PR_U68_0109_OK);
    CHECK(cpu.flags == 0x0686 && ram[at(MAP, 0x0100 + 0xF3 + 0x300)] == 0x5A);
    /* (15, 15) from base FD00h: DI = FDFFh and DI+300h wraps to 00FFh; ADD FDF0h,0Fh sets SF and PF. */
    cpu = frame(0xFD00, 15, 15, 2, 0x00);
    CHECK(pr_u68_map_update_0109_cpu(ram, RAM, &cpu) == PR_U68_0109_OK);
    CHECK(ram[at(MAP, 0x00FF)] == 0x04 && cpu.di == 0xFDFF && cpu.flags == 0x0686);

    /* Guards. */
    cpu = frame(0x0100, 3, 4, 6, 0);
    cpu.ip = 0x010A;
    CHECK(pr_u68_map_update_0109_cpu(ram, RAM, &cpu) == PR_U68_0109_ARGUMENT && cpu.sp == SP0);
    CHECK(pr_u68_map_update_0109_cpu(NULL, RAM, &cpu) == PR_U68_0109_ARGUMENT);
    CHECK(pr_u68_map_update_0109_cpu(ram, RAM - 1u, &cpu) == PR_U68_0109_ARGUMENT);

    free(ram);
    printf("u68 +0109: %u checks passed\n", checks);
    return 0;
}
