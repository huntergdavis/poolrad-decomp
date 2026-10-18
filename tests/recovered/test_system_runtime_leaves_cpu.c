/* Focused synthetic test for the raw-CPU SYSTEM FillChar, Move, set IN and UpCase leaves
 * (docs/re/SYSTEM_RUNTIME_LEAVES.md). No game bytes: every case builds a frame and data in a 1 MiB buffer
 * and compares the whole buffer with an expected copy. */
#include "poolrad/recovered/system_runtime_leaves_cpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM PR_SYSTEM_LEAF_RAM_SIZE
#define CHECK(x)                                                                  \
    do {                                                                          \
        if (!(x)) {                                                               \
            fprintf(stderr, "system leaves line %d: %s\n", __LINE__, #x);        \
            exit(1);                                                              \
        }                                                                         \
        ++checks;                                                                 \
    } while (0)

enum { SS = 0x2800, SP0 = 0x8000, DATA = 0x5000, DST = 0x6000, DS0 = 0x1111, ES0 = 0x4444, RET_IP = 0x7A00, RET_CS = 0x7A01 };

static uint8_t *ram, *want;
static unsigned checks;

static size_t at(unsigned seg, unsigned off) { return (((size_t)seg << 4) + (off & 0xFFFFu)) & (RAM - 1u); }
static void w8(unsigned seg, unsigned off, unsigned v) { ram[at(seg, off)] = (uint8_t)v; }
static void w16(unsigned seg, unsigned off, unsigned v) { w8(seg, off, v & 0xFFu); w8(seg, off + 1u, v >> 8); }
static void e8(unsigned seg, unsigned off, unsigned v) { want[at(seg, off)] = (uint8_t)v; }

static unsigned parity_even(unsigned v)
{
    unsigned ones = 0u;
    for (unsigned bit = 0u; bit < 8u; ++bit) ones += (v >> bit) & 1u;
    return (ones & 1u) == 0u;
}

static uint16_t sub_flags(unsigned flags, unsigned left, unsigned right)
{
    unsigned result = (left - right) & 0xFFu;
    flags &= ~0x08D5u;
    if (left < right) flags |= 0x0001u;
    if (parity_even(result)) flags |= 0x0004u;
    if ((left ^ right ^ result) & 0x10u) flags |= 0x0010u;
    if (result == 0u) flags |= 0x0040u;
    if (result & 0x80u) flags |= 0x0080u;
    if ((left ^ right) & (left ^ result) & 0x80u) flags |= 0x0800u;
    return (uint16_t)flags;
}

/* SS:[SP] return IP, [SP+2] return CS, then the argument words in [SP+4] order. */
static pr_system_leaf_cpu frame(unsigned ip, const unsigned *words, unsigned count, unsigned flags)
{
    pr_system_leaf_cpu cpu = {0x1234u, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu, 0x7777u, SP0, 0x0AF8u, (uint16_t)ip,
                              DS0, ES0, SS, (uint16_t)flags};
    w16(SS, SP0, RET_IP); w16(SS, SP0 + 2u, RET_CS);
    for (unsigned i = 0u; i < count; ++i) w16(SS, SP0 + 4u + 2u * i, words[i]);
    return cpu;
}

static void returned(const pr_system_leaf_cpu *cpu, unsigned args)
{
    CHECK(cpu->cs == RET_CS && cpu->ip == RET_IP && cpu->sp == SP0 + 4u + args && cpu->ss == SS && cpu->bp == 0x7777u);
}

static void fillchar(void)
{
    static const unsigned counts[] = {0u, 1u, 2u, 255u, 256u, 0x1234u, 0xFFFFu};
    for (unsigned k = 0u; k < 7u; ++k) {
        unsigned count = counts[k], value = 0xA0u + k, offset = 0xFF00u - 3u * k, words[4];
        pr_system_leaf_cpu cpu;
        memset(ram, 0x5C, RAM);
        words[0] = value | 0x3300u; words[1] = count; words[2] = offset; words[3] = DST;
        cpu = frame(0x16A8u, words, 4u, 0x0ED7u);
        memcpy(want, ram, RAM);
        for (unsigned i = 0u; i < count; ++i) e8(DST, offset + i, value);   /* DI wraps inside the segment */
        CHECK(pr_system_fillchar_16a8(ram, RAM, &cpu) == PR_SYSTEM_LEAF_RETURNED);
        CHECK(memcmp(ram, want, RAM) == 0);
        CHECK(cpu.ax == (0x1200u | value) && cpu.bx == SP0 && cpu.cx == 0u && cpu.dx == 0x789Au && cpu.si == 0x9ABCu);
        CHECK(cpu.di == ((offset + count) & 0xFFFFu) && cpu.es == DST && cpu.ds == DS0);
        CHECK(cpu.flags == (0x0ED7u & ~0x0400u));                             /* only DF changes */
        returned(&cpu, 8u);
    }
    {   /* a fill over its own frame: RETF 8 pops the fill value */
        unsigned words[4] = {0x0B, 0x20u, SP0 - 8u, SS};
        pr_system_leaf_cpu cpu;
        memset(ram, 0x5C, RAM);
        cpu = frame(0x16A8u, words, 4u, 0x0002u);
        CHECK(pr_system_fillchar_16a8(ram, RAM, &cpu) == PR_SYSTEM_LEAF_RETURNED);
        CHECK(cpu.ip == 0x0B0Bu && cpu.cs == 0x0B0Bu && cpu.sp == SP0 + 12u && cpu.di == SP0 + 0x18u);
    }
}

static void move(void)
{
    static const int deltas[] = {1, 2, 7, -1, -2, -7, 0};
    for (unsigned k = 0u; k < 7u; ++k) {                 /* overlapping copies in one segment, forward byte order */
        unsigned src = 0x0400u, dst = (unsigned)((int)src + deltas[k]), count = 0x40u, words[5];
        uint8_t expect[0x100];
        pr_system_leaf_cpu cpu;
        memset(ram, 0x5C, RAM);
        for (unsigned i = 0u; i < 0x100u; ++i) w8(DATA, 0x0380u + i, 0x80u + i);
        words[0] = count; words[1] = dst; words[2] = DATA; words[3] = src; words[4] = DATA;
        cpu = frame(0x0A3Fu, words, 5u, 0x0602u);
        memcpy(want, ram, RAM);
        for (unsigned i = 0u; i < 0x100u; ++i) expect[i] = ram[at(DATA, 0x0380u + i)];
        for (unsigned i = 0u; i < count; ++i) expect[dst - 0x0380u + i] = expect[src - 0x0380u + i];
        for (unsigned i = 0u; i < 0x100u; ++i) e8(DATA, 0x0380u + i, expect[i]);
        CHECK(pr_system_move_0a3f(ram, RAM, &cpu) == PR_SYSTEM_LEAF_RETURNED);
        CHECK(memcmp(ram, want, RAM) == 0);
        CHECK(cpu.bx == SP0 && cpu.cx == 0u && cpu.dx == DS0 && cpu.ds == DS0 && cpu.es == DATA && cpu.ax == 0x1234u);
        CHECK(cpu.si == src + count && cpu.di == dst + count && cpu.flags == 0x0202u);
        returned(&cpu, 10u);
    }
    {   /* destination one above: the first byte propagates */
        unsigned words[5] = {4u, 0x0101u, DATA, 0x0100u, DATA};
        pr_system_leaf_cpu cpu;
        memset(ram, 0x5C, RAM);
        w8(DATA, 0x0100u, 0x11u); w8(DATA, 0x0101u, 0x22u); w8(DATA, 0x0102u, 0x33u); w8(DATA, 0x0103u, 0x44u);
        cpu = frame(0x0A3Fu, words, 5u, 0x0002u);
        CHECK(pr_system_move_0a3f(ram, RAM, &cpu) == PR_SYSTEM_LEAF_RETURNED);
        for (unsigned i = 0u; i <= 4u; ++i) CHECK(ram[at(DATA, 0x0100u + i)] == 0x11u);
    }
    {   /* SI and DI wrap inside their segments */
        unsigned words[5] = {3u, 0xFFFFu, DST, 0xFFFEu, DATA};
        pr_system_leaf_cpu cpu;
        memset(ram, 0x5C, RAM);
        w8(DATA, 0xFFFEu, 0xA1u); w8(DATA, 0xFFFFu, 0xA2u); w8(DATA, 0x0000u, 0xA3u);
        cpu = frame(0x0A3Fu, words, 5u, 0x0002u);
        memcpy(want, ram, RAM);
        e8(DST, 0xFFFFu, 0xA1u); e8(DST, 0x0000u, 0xA2u); e8(DST, 0x0001u, 0xA3u);
        CHECK(pr_system_move_0a3f(ram, RAM, &cpu) == PR_SYSTEM_LEAF_RETURNED);
        CHECK(memcmp(ram, want, RAM) == 0 && cpu.si == 0x0001u && cpu.di == 0x0002u);
    }
}

static void set_in(void)
{
    for (unsigned e = 0u; e < 256u; ++e) {
        unsigned words[3], set_off = (e & 1u) ? 0xFFF0u : 0x0200u, mask = 1u << (e & 7u), byte, flags_in = (e & 2u) ? 0x0ED7u : 0x0002u;
        pr_system_leaf_cpu cpu;
        memset(ram, 0x5C, RAM);
        for (unsigned i = 0u; i < 32u; ++i) w8(DATA, set_off + i, (i * 0x3Bu + 0x15u) & 0xFFu);
        words[0] = set_off; words[1] = DATA; words[2] = e | 0x9900u;
        cpu = frame(0x0DC0u, words, 3u, flags_in);
        memcpy(want, ram, RAM);
        byte = ram[at(DATA, set_off + (e >> 3))] & mask;                     /* [BX+DI] wraps at 16 bits */
        CHECK(pr_system_set_in_0dc0(ram, RAM, &cpu) == PR_SYSTEM_LEAF_RETURNED);
        CHECK(memcmp(ram, want, RAM) == 0);
        CHECK(cpu.ax == (0x1200u | mask) && cpu.bx == (e >> 3) && cpu.cx == (0x5600u | (e & 7u)) && cpu.di == set_off);
        CHECK(cpu.es == DATA && cpu.ds == DS0 && cpu.dx == 0x789Au && cpu.si == 0x9ABCu);
        CHECK(cpu.flags == ((flags_in & ~0x08D5u) | (parity_even(byte) ? 0x0004u : 0u) | (byte ? 0u : 0x0040u) | (byte & 0x80u)));
        returned(&cpu, 6u);
    }
}

static void upcase(void)
{
    for (unsigned c = 0u; c < 256u; ++c) {
        unsigned words[1] = {c | 0x7700u}, result = c, flags_in = (c & 1u) ? 0x0ED7u : 0x0002u, flags;
        pr_system_leaf_cpu cpu;
        memset(ram, 0x5C, RAM);
        cpu = frame(0x182Au, words, 1u, flags_in);
        memcpy(want, ram, RAM);
        flags = sub_flags(flags_in, c, 0x61u);
        if (c >= 0x61u) {
            flags = sub_flags(flags_in, c, 0x7Au);
            if (c <= 0x7Au) { flags = sub_flags(flags_in, c, 0x20u); result = c - 0x20u; }
        }
        CHECK(pr_system_upcase_182a(ram, RAM, &cpu) == PR_SYSTEM_LEAF_RETURNED);
        CHECK(memcmp(ram, want, RAM) == 0);
        CHECK(cpu.ax == (0x1200u | result) && cpu.bx == SP0 && cpu.flags == flags);
        CHECK((c >= 'a' && c <= 'z') ? result == c - 32u : result == c);
        returned(&cpu, 2u);
    }
}

static void arguments(void)
{
    pr_system_leaf_status (*const leaves[4])(uint8_t *, size_t, pr_system_leaf_cpu *) = {
        pr_system_fillchar_16a8, pr_system_move_0a3f, pr_system_set_in_0dc0, pr_system_upcase_182a};
    for (unsigned k = 0u; k < 4u; ++k) {
        unsigned words[5] = {1u, 1u, 0x0100u, DATA, 0x0100u};
        pr_system_leaf_cpu cpu, before;
        memset(ram, 0x5C, RAM);
        cpu = frame(0x16A8u, words, 5u, 0x0002u);
        before = cpu;
        memcpy(want, ram, RAM);
        CHECK(leaves[k](NULL, RAM, &cpu) == PR_SYSTEM_LEAF_ARGUMENT);
        CHECK(leaves[k](ram, RAM, NULL) == PR_SYSTEM_LEAF_ARGUMENT);
        CHECK(leaves[k](ram, RAM - 1u, &cpu) == PR_SYSTEM_LEAF_ARGUMENT);
        CHECK(leaves[k](ram, RAM + 1u, &cpu) == PR_SYSTEM_LEAF_ARGUMENT);
        CHECK(memcmp(&cpu, &before, sizeof cpu) == 0 && memcmp(ram, want, RAM) == 0);
    }
}

int main(void)
{
    ram = malloc(RAM);
    want = malloc(RAM);
    CHECK(ram != NULL && want != NULL);
    fillchar();
    move();
    set_in();
    upcase();
    arguments();
    free(ram);
    free(want);
    printf("system runtime leaves: %u checks passed\n", checks);
    return 0;
}
