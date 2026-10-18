/* Focused synthetic test for GAME.OVR unit 0x3B4 +00C7, +00FB and +012F (docs/re/U3B4_CASE_MAPS.md). */
#include "poolrad/recovered/overlay_u3b4_case_maps.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

_Static_assert(sizeof(pr_u3b4_case_cpu) == 28, "all fourteen CPU words");

#define RAM PR_U3B4_CASE_RAM_SIZE
#define CHECK(x)                                                                  \
    do {                                                                          \
        if (!(x)) {                                                               \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #x); \
            exit(1);                                                              \
        }                                                                         \
        ++checks;                                                                 \
    } while (0)

typedef pr_u3b4_case_status (*routine)(uint8_t *, size_t, pr_u3b4_case_cpu *);

typedef struct {
    routine run;
    unsigned entry, compares;
    uint8_t value[4], result[4], otherwise;
} spec;

static const spec specs[3] = {
    {pr_u3b4_case_map_00c7, 0x00C7, 3, {0x08, 0x41, 0xE8}, {0x00, 0x00, 0x0F}, 0x0F},
    {pr_u3b4_case_map_00fb, 0x00FB, 3, {0x08, 0x41, 0xE8}, {0x08, 0x07, 0x07}, 0x07},
    {pr_u3b4_case_map_012f, 0x012F, 4, {0x0B, 0x09, 0xDB, 0x06}, {0x08, 0x06, 0x06, 0x01}, 0x06},
};

enum { SS = 0x2000, CODE = 0x3000, RET_IP = 0x0987, RET_CS = 0x4321, LINK = 0x6E6E };

static uint8_t ram[RAM], want[RAM];
static unsigned checks;

static size_t at(unsigned segment, unsigned offset)
{
    return (((size_t)segment << 4) + (offset & 0xFFFFu)) & (RAM - 1u);
}

static void put16(uint8_t *memory, unsigned segment, unsigned offset, unsigned value)
{
    memory[at(segment, offset)] = (uint8_t)value;
    memory[at(segment, offset + 1u)] = (uint8_t)(value >> 8);
}

/* Independent CMP r/m8 flag rule (CF PF AF ZF SF OF; other bits kept). */
static uint16_t cmp_flags(unsigned flags, unsigned left, unsigned right)
{
    unsigned result = (left - right) & 0xFFu, ones = 0u;
    for (unsigned bit = 0u; bit < 8u; ++bit)
        ones += (result >> bit) & 1u;
    flags &= ~0x08D5u;
    if (left < right) flags |= 0x0001u;
    if ((ones & 1u) == 0u) flags |= 0x0004u;
    if ((left ^ right ^ result) & 0x10u) flags |= 0x0010u;
    if (result == 0u) flags |= 0x0040u;
    if (result & 0x80u) flags |= 0x0080u;
    if ((left ^ right) & (left ^ result) & 0x80u) flags |= 0x0800u;
    return (uint16_t)flags;
}

/* One call with argument byte b at entry SP sp; checks every CPU word and the whole RAM. */
static void run_case(const spec *s, unsigned b, unsigned sp, unsigned flags, unsigned seed)
{
    pr_u3b4_case_cpu cpu = {0, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777, 0, CODE, 0, 0x1000, 0x1111, SS, 0};
    unsigned j = 0u, result = s->otherwise, last;
    for (size_t i = 0; i < RAM; i += 4096u)
        memset(ram + i, (int)((i >> 12) * 7u + seed) & 0xFF, 4096u);
    put16(ram, SS, sp, RET_IP);
    put16(ram, SS, sp + 2u, RET_CS);
    put16(ram, SS, sp + 4u, LINK);
    put16(ram, SS, sp + 6u, 0xC300u | b);
    memcpy(want, ram, RAM);
    while (j < s->compares && s->value[j] != b)
        ++j;
    last = s->value[j < s->compares ? j : s->compares - 1u];
    if (j < s->compares)
        result = s->result[j];
    put16(want, SS, sp - 2u, 0x7777u);
    want[at(SS, sp - 3u)] = (uint8_t)result;
    cpu.ax = (uint16_t)(0x5A00u | (b ^ 0xFFu));
    cpu.sp = (uint16_t)sp;
    cpu.ip = (uint16_t)s->entry;
    cpu.flags = (uint16_t)flags;
    CHECK(s->run(ram, RAM, &cpu) == PR_U3B4_CASE_OK);
    CHECK(cpu.ax == (0x5A00u | result));
    CHECK(cpu.flags == cmp_flags(flags, b, last));
    CHECK(cpu.bx == 0x3456 && cpu.cx == 0x5678 && cpu.dx == 0x789A && cpu.si == 0x9ABC && cpu.di == 0xBCDE);
    CHECK(cpu.bp == 0x7777 && cpu.sp == (uint16_t)(sp + 8u) && cpu.ip == RET_IP && cpu.cs == RET_CS);
    CHECK(cpu.ds == 0x1000 && cpu.es == 0x1111 && cpu.ss == SS);
    CHECK(memcmp(ram, want, RAM) == 0);
}

int main(void)
{
    unsigned hits = 0u;
    for (unsigned n = 0u; n < 3u; ++n) {
        const spec *s = &specs[n];
        for (unsigned b = 0u; b < 256u; ++b) {
            run_case(s, b, 0x8000u, 0x0002u, n);
            run_case(s, b, 0x8000u, 0x0FD7u, n + 3u);   /* DF, IF, TF, IOPL bits kept; arithmetic flags replaced */
            for (unsigned j = 0u; j < s->compares; ++j)
                hits += s->value[j] == b;
        }
        /* Entry SP 0002h: BP = 0000h, so [BP-1] is SS:FFFF and [BP+8] is SS:0008. */
        run_case(s, s->value[0], 0x0002u, 0x0202u, 9u);
        /* Entry SP FFF6h: the argument word sits at SS:FFFC, the last bytes of the segment. */
        run_case(s, s->value[s->compares - 1u], 0xFFF6u, 0x0202u, 10u);
    }
    CHECK(hits == 10u);

    {
        pr_u3b4_case_cpu cpu = {1, 2, 3, 4, 5, 6, 7, 0x8000, CODE, 0x00C7, 8, 9, SS, 0x0202}, saved = cpu;
        memset(ram, 0x5C, RAM);
        CHECK(pr_u3b4_case_map_00c7(NULL, RAM, &cpu) == PR_U3B4_CASE_ARGUMENT);
        CHECK(pr_u3b4_case_map_00fb(ram, RAM, NULL) == PR_U3B4_CASE_ARGUMENT);
        CHECK(pr_u3b4_case_map_012f(ram, RAM - 1u, &cpu) == PR_U3B4_CASE_ARGUMENT);
        CHECK(pr_u3b4_case_map_012f(ram, (size_t)RAM + 1u, &cpu) == PR_U3B4_CASE_ARGUMENT);
        CHECK(pr_u3b4_case_map_00c7(ram, 0u, &cpu) == PR_U3B4_CASE_ARGUMENT);
        CHECK(memcmp(&cpu, &saved, sizeof cpu) == 0);
        for (size_t i = 0; i < RAM; ++i)
            if (ram[i] != 0x5C) {
                CHECK(0);
            }
    }

    printf("PASS: u3B4 +00C7/+00FB/+012F, %u checks (all 256 argument bytes twice each, segment-end stacks, arguments)\n",
           checks);
    return 0;
}
