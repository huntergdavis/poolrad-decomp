/* Focused synthetic test for the raw-CPU SYSTEM leaves 0AF8:04A2 (IOResult) and 0AF8:13E2 (Assign)
 * (docs/re/SYSTEM_IORESULT_ASSIGN.md). No game bytes: synthetic names and records, every result predicted by hand. */
#include "poolrad/recovered/system_ioresult_assign_cpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM PR_SYSTEM_LEAF_RAM_SIZE
#define CHECK(x)                                                                  \
    do {                                                                          \
        if (!(x)) {                                                               \
            fprintf(stderr, "ioresult/assign line %d: %s\n", __LINE__, #x);       \
            exit(1);                                                              \
        }                                                                         \
        ++checks;                                                                 \
    } while (0)

enum { SYS = 0x0AF8, SS = 0x2800, SP0 = 0x4000, RET_IP = 0x7000, RET_CS = 0x9001, DS0 = 0x1357, FSEG = 0x5000, SSEG = 0x4000 };

static uint8_t *ram;
static unsigned checks;

static size_t at(unsigned seg, unsigned off) { return (((size_t)seg << 4) + (off & 0xFFFFu)) & (RAM - 1u); }
static unsigned get16(unsigned seg, unsigned off) { return ram[at(seg, off)] | ((unsigned)ram[at(seg, off + 1u)] << 8); }
static void put16(unsigned seg, unsigned off, unsigned value)
{
    ram[at(seg, off)] = (uint8_t)value;
    ram[at(seg, off + 1u)] = (uint8_t)(value >> 8);
}
static int zeros(unsigned seg, unsigned off, unsigned count)
{
    unsigned i;
    for (i = 0; i < count; ++i)
        if (ram[at(seg, off + i)] != 0)
            return 0;
    return 1;
}

static pr_system_leaf_cpu frame(uint16_t ip, const unsigned *words, size_t count, uint16_t flags)
{
    pr_system_leaf_cpu cpu;
    size_t i;
    memset(ram, 0xA5, RAM);
    put16(SS, SP0, RET_IP); put16(SS, SP0 + 2u, RET_CS);
    for (i = 0; i < count; ++i)
        put16(SS, SP0 + 4u + 2u * (unsigned)i, words[i]);
    memset(&cpu, 0, sizeof cpu);
    cpu.ax = 0x1111; cpu.bx = 0x2222; cpu.cx = 0x3333; cpu.dx = 0x4444; cpu.si = 0x5555; cpu.di = 0x6666; cpu.bp = 0x7777;
    cpu.sp = SP0; cpu.cs = SYS; cpu.ip = ip; cpu.ds = DS0; cpu.es = 0x9999; cpu.ss = SS; cpu.flags = flags;
    return cpu;
}

/* Assign(F at 5000:0100, S at seg:off) after `setup` placed the name; checks the common results. */
static pr_system_leaf_cpu assign(unsigned sseg, unsigned soff, const char *pascal, size_t length)
{
    const unsigned words[4] = {soff, sseg, 0x0100, FSEG};
    pr_system_leaf_cpu cpu = frame(0x13E2, words, 4, 0x0602);
    memcpy(ram + at(sseg, soff), pascal, length);
    CHECK(pr_system_assign_13e2(ram, RAM, &cpu) == PR_SYSTEM_LEAF_RETURNED);
    CHECK(cpu.cs == RET_CS && cpu.ip == RET_IP && cpu.sp == SP0 + 12u && cpu.bx == SP0 && cpu.ds == DS0 && cpu.es == FSEG);
    CHECK(cpu.ax == 0 && cpu.cx == 0 && cpu.dx == 0x4444 && cpu.flags == 0x0246 && get16(SS, SP0 - 2u) == DS0);
    CHECK(get16(FSEG, 0x0100) == 0 && get16(FSEG, 0x0102) == 0xD7B0 && zeros(FSEG, 0x0104, 44));
    return cpu;
}

int main(void)
{
    pr_system_leaf_cpu cpu;
    char long_name[1 + 100];
    ram = malloc(RAM);
    if (ram == NULL)
        return 1;

    /* IOResult: AX := InOutRes (103), which becomes 0; XOR AX,AX's flags over 0ED7h. */
    cpu = frame(0x04A2, NULL, 0, 0x0ED7);
    cpu.ds = 0x5000;
    put16(0x5000, 0x4426, 103);
    CHECK(pr_system_ioresult_04a2(ram, RAM, &cpu) == PR_SYSTEM_LEAF_RETURNED);
    CHECK(cpu.ax == 103 && get16(0x5000, 0x4426) == 0 && cpu.flags == 0x0646 && cpu.sp == SP0 + 4u && cpu.cs == RET_CS && cpu.ip == RET_IP);
    CHECK(cpu.bx == 0x2222 && cpu.cx == 0x3333 && cpu.dx == 0x4444 && cpu.ds == 0x5000 && cpu.es == 0x9999);

    /* Assign "PHLAN": the name at F+48 and a NUL; DI and SI just past them. */
    cpu = assign(SSEG, 0x0200, "\x05PHLAN", 6);
    CHECK(memcmp(ram + at(FSEG, 0x0130), "PHLAN\0", 6) == 0 && cpu.di == 0x0136 && cpu.si == 0x0206);

    /* A 100-character name is cut to 79: the NUL lands at F+127. */
    long_name[0] = 100;
    memset(long_name + 1, 'N', 100);
    cpu = assign(SSEG, 0x0200, long_name, sizeof long_name);
    CHECK(ram[at(FSEG, 0x0130)] == 'N' && ram[at(FSEG, 0x017E)] == 'N' && ram[at(FSEG, 0x017F)] == 0 && cpu.di == 0x0180 && cpu.si == 0x0250);

    /* The name inside the record at F+10: the zeroing empties it before LODSB, so only the NUL is written. */
    cpu = assign(FSEG, 0x010A, "\x03XYZ", 4);
    CHECK(ram[at(FSEG, 0x0130)] == 0 && cpu.di == 0x0131 && cpu.si == 0x010B);

    /* Guards. */
    CHECK(pr_system_ioresult_04a2(NULL, RAM, &cpu) == PR_SYSTEM_LEAF_ARGUMENT);
    CHECK(pr_system_assign_13e2(ram, RAM - 1u, &cpu) == PR_SYSTEM_LEAF_ARGUMENT && pr_system_assign_13e2(ram, RAM, NULL) == PR_SYSTEM_LEAF_ARGUMENT);

    free(ram);
    printf("ioresult/assign: %u checks passed\n", checks);
    return 0;
}
