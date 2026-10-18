/* Focused synthetic test for the raw-CPU SYSTEM string leaves 0AF8:0B0B (load) and 0AF8:0B98 (concatenation)
 * (docs/re/SYSTEM_STRING_LEAVES.md). No game bytes: synthetic Pascal strings, every result predicted by hand. */
#include "poolrad/recovered/system_string_leaves_cpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM PR_SYSTEM_LEAF_RAM_SIZE
#define CHECK(x)                                                                  \
    do {                                                                          \
        if (!(x)) {                                                               \
            fprintf(stderr, "strings line %d: %s\n", __LINE__, #x);               \
            exit(1);                                                              \
        }                                                                         \
        ++checks;                                                                 \
    } while (0)

enum { SYS = 0x0AF8, SS = 0x2800, SP0 = 0x8000, SEG = 0x4000, RET_IP = 0x7000, RET_CS = 0x9001, DS0 = 0x1357, FLAGS0 = 0x0ED7 };

static uint8_t *ram;
static unsigned checks;

static size_t at(unsigned seg, unsigned off) { return (((size_t)seg << 4) + (off & 0xFFFFu)) & (RAM - 1u); }
static void w8(unsigned seg, unsigned off, unsigned v) { ram[at(seg, off)] = (uint8_t)v; }
static uint8_t r8(unsigned seg, unsigned off) { return ram[at(seg, off)]; }
static void w16(unsigned seg, unsigned off, unsigned v) { w8(seg, off, v & 0xFFu); w8(seg, off + 1u, v >> 8); }

static void pstring(unsigned off, const char *text)
{
    size_t n = strlen(text), i;
    w8(SEG, off, (unsigned)n);
    for (i = 0u; i < n; ++i) w8(SEG, off + 1u + (unsigned)i, (unsigned char)text[i]);
}

/* A frame with S = SEG:s and D = SEG:d; entry at `ip`. */
static pr_system_leaf_cpu frame(unsigned ip, unsigned s, unsigned d)
{
    pr_system_leaf_cpu cpu = {0x1234u, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu, 0x7777u, SP0, SYS, (uint16_t)ip, DS0, 0x4444u, SS, FLAGS0};
    w16(SS, SP0, RET_IP); w16(SS, SP0 + 2u, RET_CS); w16(SS, SP0 + 4u, s); w16(SS, SP0 + 6u, SEG); w16(SS, SP0 + 8u, d); w16(SS, SP0 + 10u, SEG);
    return cpu;
}

static void common(const pr_system_leaf_cpu *cpu)
{
    CHECK(cpu->cs == RET_CS && cpu->ip == RET_IP && cpu->sp == SP0 + 8u && cpu->bx == SP0 && cpu->dx == DS0 && cpu->ds == DS0);
    CHECK(cpu->cx == 0u && cpu->es == SEG && (cpu->flags & 0x0400u) == 0u && (cpu->flags & 0x0200u) == (FLAGS0 & 0x0200u));
}

static void load(void)
{
    pr_system_leaf_cpu cpu;
    memset(ram, 0xA5, RAM);
    pstring(0x0100u, "Phlan");
    cpu = frame(0x0B0Bu, 0x0100u, 0x0200u);
    CHECK(pr_system_strload_0b0b(ram, RAM, &cpu) == PR_SYSTEM_LEAF_RETURNED);
    common(&cpu);
    CHECK(memcmp(ram + at(SEG, 0x0200u), "\x05Phlan", 6) == 0 && r8(SEG, 0x0206u) == 0xA5u);
    CHECK((cpu.ax & 0xFFu) == 5u && (cpu.ax >> 8) == 0x12u && cpu.si == 0x0106u && cpu.di == 0x0206u);
    CHECK((cpu.flags & 0x08D5u) == 0x0044u);                              /* XOR CH,CH */

    memset(ram, 0xA5, RAM);                                               /* D two bytes past S: the copy replicates */
    pstring(0x0100u, "abcdef");
    cpu = frame(0x0B0Bu, 0x0100u, 0x0102u);
    CHECK(pr_system_strload_0b0b(ram, RAM, &cpu) == PR_SYSTEM_LEAF_RETURNED);
    {
        static const uint8_t replicated[9] = {6u, 'a', 6u, 'a', 6u, 'a', 6u, 'a', 6u};
        CHECK(memcmp(ram + at(SEG, 0x0100u), replicated, sizeof replicated) == 0);
    }

    memset(ram, 0xA5, RAM);                                               /* S and D wrap past SEG:FFFF */
    pstring(0xFFFDu, "xyz");
    cpu = frame(0x0B0Bu, 0xFFFDu, 0xFFFEu);
    CHECK(pr_system_strload_0b0b(ram, RAM, &cpu) == PR_SYSTEM_LEAF_RETURNED && cpu.si == 0x0001u && cpu.di == 0x0002u);
}

static void concat(void)
{
    pr_system_leaf_cpu cpu;
    memset(ram, 0xA5, RAM);
    pstring(0x0100u, "Pool");
    pstring(0x0200u, " of Radiance");
    cpu = frame(0x0B98u, 0x0200u, 0x0100u);
    CHECK(pr_system_strcat_0b98(ram, RAM, &cpu) == PR_SYSTEM_LEAF_RETURNED);
    common(&cpu);
    CHECK(memcmp(ram + at(SEG, 0x0100u), "\x10Pool of Radiance", 17) == 0 && (cpu.ax & 0xFFu) == 12u);
    CHECK(cpu.si == 0x020Du && cpu.di == 0x0111u && (cpu.flags & 0x08D5u) == 0x0004u); /* INC DI giving 0105h: PF only */
}

static void clamp(void)
{
    pr_system_leaf_cpu cpu;
    unsigned i;
    memset(ram, 0xA5, RAM);                                               /* 200 + 56 > 255: only 55 bytes fit */
    w8(SEG, 0x0100u, 200u);
    for (i = 1u; i <= 200u; ++i) w8(SEG, 0x0100u + i, 'a');
    w8(SEG, 0x0400u, 56u);
    for (i = 1u; i <= 56u; ++i) w8(SEG, 0x0400u + i, 'b');
    cpu = frame(0x0B98u, 0x0400u, 0x0100u);
    CHECK(pr_system_strcat_0b98(ram, RAM, &cpu) == PR_SYSTEM_LEAF_RETURNED);
    CHECK(r8(SEG, 0x0100u) == 255u && (cpu.ax & 0xFFu) == 55u && r8(SEG, 0x0100u + 255u) == 'b' && r8(SEG, 0x0100u + 256u) == 0xA5u);
    CHECK(cpu.si == 0x0400u + 1u + 55u && cpu.di == 0x0100u + 256u);

    memset(ram, 0xA5, RAM);                                               /* 200 + 55 = 255 exactly: no clamp */
    w8(SEG, 0x0100u, 200u);
    w8(SEG, 0x0400u, 55u);
    cpu = frame(0x0B98u, 0x0400u, 0x0100u);
    CHECK(pr_system_strcat_0b98(ram, RAM, &cpu) == PR_SYSTEM_LEAF_RETURNED && r8(SEG, 0x0100u) == 255u && (cpu.ax & 0xFFu) == 55u);

    memset(ram, 0xA5, RAM);                                               /* ADD DI,CX carries: CF stays through INC */
    w8(SEG, 0xFFF0u, 40u);
    w8(SEG, 0x0400u, 0u);
    cpu = frame(0x0B98u, 0x0400u, 0xFFF0u);
    CHECK(pr_system_strcat_0b98(ram, RAM, &cpu) == PR_SYSTEM_LEAF_RETURNED && cpu.di == 0x0019u && (cpu.flags & 0x0001u) != 0u);
}

static void arguments(void)
{
    pr_system_leaf_cpu cpu = frame(0x0B0Bu, 0x0100u, 0x0200u), before = cpu;
    CHECK(pr_system_strload_0b0b(NULL, RAM, &cpu) == PR_SYSTEM_LEAF_ARGUMENT);
    CHECK(pr_system_strcat_0b98(ram, RAM - 1u, &cpu) == PR_SYSTEM_LEAF_ARGUMENT);
    CHECK(pr_system_strcat_0b98(ram, RAM, NULL) == PR_SYSTEM_LEAF_ARGUMENT);
    CHECK(memcmp(&cpu, &before, sizeof cpu) == 0);
}

int main(void)
{
    ram = malloc(RAM);
    CHECK(ram != NULL);
    load();
    concat();
    clamp();
    arguments();
    free(ram);
    printf("string leaves: %u checks passed\n", checks);
    return 0;
}
