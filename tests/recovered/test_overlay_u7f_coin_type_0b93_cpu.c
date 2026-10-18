/* Focused synthetic test for raw-CPU U7F +0B93 composing the SYSTEM string store and UpCase
 * (docs/re/U7F_COIN_TYPE_0B93.md). No game bytes: only the nine five-byte far-call sites and synthetic
 * code-segment literals ("lit5" and so on) in place of the game's strings. Every case compares all RAM
 * outside the stack frame with an expected copy, plus the frame's index and result bytes. */
#include "poolrad/recovered/overlay_u7f_coin_type_0b93_cpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM PR_U7F_0B93_RAM_SIZE
#define CHECK(x)                                                                  \
    do {                                                                          \
        if (!(x)) {                                                               \
            fprintf(stderr, "u7f 0b93 line %d: %s\n", __LINE__, #x);              \
            exit(1);                                                              \
        }                                                                         \
        ++checks;                                                                 \
    } while (0)

enum { CS = 0x8800, SYS = 0x0AF8, SS = 0x2800, SP0 = 0x8000, BP1 = SP0 - 2, BP0 = 0x7777, SRC = 0x4000, SOFF = 0x0100,
       DST = 0x6000, DOFF = 0x0200, RET_IP = 0x7000, RET_CS = 0x9001, CAP = 600 };

static const unsigned store_sites[8] = {0x0BA9u, 0x0C01u, 0x0C16u, 0x0C34u, 0x0C51u, 0x0C6Eu, 0x0C8Bu, 0x0CA8u};
static const struct { unsigned letter, literal, result; } table[7] = {
    {'G', 0x0B5Au, 5u}, {'g', 0x0B60u, 3u}, {'P', 0x0B66u, 4u}, {'E', 0x0B70u, 2u}, {'S', 0x0B7Au, 1u}, {'C', 0x0B82u, 0u}, {'J', 0x0B8Au, 6u}
};

static uint8_t *ram, *want;
static unsigned checks;

static size_t at(unsigned seg, unsigned off) { return (((size_t)seg << 4) + (off & 0xFFFFu)) & (RAM - 1u); }
static void w8(unsigned seg, unsigned off, unsigned v) { ram[at(seg, off)] = (uint8_t)v; }
static uint8_t r8(unsigned seg, unsigned off) { return ram[at(seg, off)]; }
static void w16(unsigned seg, unsigned off, unsigned v) { w8(seg, off, v & 0xFFu); w8(seg, off + 1u, v >> 8); }

/* Synthetic literal k at CS:offset: "litN" with N the result digit. */
static void literal(unsigned offset, unsigned digit)
{
    w8(CS, offset, 4u); w8(CS, offset + 1u, 'l'); w8(CS, offset + 2u, 'i'); w8(CS, offset + 3u, 't'); w8(CS, offset + 4u, '0' + digit);
}

static pr_u7f_0b93_cpu world(const char *text, unsigned stale, unsigned flags)
{
    pr_u7f_0b93_cpu cpu = {0x1234u, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu, BP0, SP0, CS, 0x0B93u, 0x1111u, 0x4444u, SS, (uint16_t)flags};
    size_t n = strlen(text);
    memset(ram, 0x5C, RAM);
    for (unsigned s = 0u; s < 8u; ++s) { w8(CS, store_sites[s], 0x9Au); w16(CS, store_sites[s] + 1u, 0x0B25u); w16(CS, store_sites[s] + 3u, SYS); }
    w8(CS, 0x0BE6u, 0x9Au); w16(CS, 0x0BE7u, 0x182Au); w16(CS, 0x0BE9u, SYS);
    for (unsigned k = 0u; k < 7u; ++k) literal(table[k].literal, table[k].result);
    for (unsigned i = 0u; i < 0x110u; ++i) w8(SS, BP1 - 0x108u + i, stale);
    w8(SRC, SOFF, (unsigned)n);
    for (size_t i = 0u; i < n; ++i) w8(SRC, SOFF + 1u + (unsigned)i, (unsigned char)text[i]);
    w16(SS, SP0, RET_IP); w16(SS, SP0 + 2u, RET_CS); w16(SS, SP0 + 4u, DOFF); w16(SS, SP0 + 6u, DST);
    w16(SS, SP0 + 8u, SOFF); w16(SS, SP0 + 0x0Au, SRC);
    return cpu;
}

/* Expect: D receives literal `offset` (when matched), the frame window is free, and the result and index bytes. */
static void run_case(const char *text, unsigned stale, int literal_offset, unsigned result, unsigned index, unsigned flags)
{
    pr_u7f_0b93_cpu cpu = world(text, stale, flags);
    int callee = 99;
    memcpy(want, ram, RAM);
    if (literal_offset >= 0)
        for (unsigned i = 0u; i < 5u; ++i) want[at(DST, DOFF + i)] = r8(CS, (unsigned)literal_offset + i);
    CHECK(pr_u7f_coin_type_0b93_cpu(ram, RAM, &cpu, CAP, &callee) == PR_U7F_0B93_OK && callee == 0);
    CHECK((cpu.ax & 0xFFu) == result && r8(SS, BP1 - 1u) == result && r8(SS, BP1 - 0x102u) == index);
    for (unsigned off = BP1 - 0x120u; off < SP0 + 0x0Cu; ++off) want[at(SS, off)] = ram[at(SS, off)];
    CHECK(memcmp(ram, want, RAM) == 0);
    CHECK(cpu.bp == BP0 && cpu.sp == SP0 + 12u && cpu.cs == RET_CS && cpu.ip == RET_IP && cpu.ss == SS && cpu.ds == 0x1111u);
    CHECK((cpu.flags & 0x0400u) == 0u);
    if (literal_offset >= 0)
        CHECK((cpu.flags & 0x08D5u) == 0x0044u && cpu.cx == 0u && cpu.es == DST);   /* the last store's XOR CH,CH */
}

static void letters(void)
{
    run_case("Gems", 0x5Cu, 0x0B5A, 5u, 1u, 0x0002u);                     /* 'G' then UpCase('e') = 'E' */
    run_case("GEMS", 0x5Cu, 0x0B5A, 5u, 1u, 0x0ED7u);
    run_case("Gold", 0x5Cu, 0x0B60, 3u, 1u, 0x0002u);
    run_case("  Platinum", 0x5Cu, 0x0B66, 4u, 3u, 0x0002u);
    run_case(" Electrum", 0x5Cu, 0x0B70, 2u, 2u, 0x0002u);
    run_case("Silver", 0x5Cu, 0x0B7A, 1u, 1u, 0x0002u);
    run_case("Copper", 0x5Cu, 0x0B82, 0u, 1u, 0x0002u);
    run_case("Jewels", 0x5Cu, 0x0B8A, 6u, 1u, 0x0002u);
    run_case("gold", 0x37u, -1, 0x37u, 1u, 0x0002u);                      /* lower case: unmatched, the stale [BP-1] */
    run_case("", 0x41u, -1, 0x41u, 1u, 0x0002u);                          /* empty: L[1] is the stale byte 41h */
    run_case("G", 0x45u, 0x0B5A, 5u, 1u, 0x0002u);                        /* L[2] past the copy is the stale 'E' */
}

static void stops(void)
{
    pr_u7f_0b93_cpu cpu;
    int callee = 99;
    cpu = world("", 0x20u, 0x0002u);                                      /* an all-space local: LIMIT */
    w8(SRC, SOFF, 0x20u);
    for (unsigned i = 1u; i <= 0x20u; ++i) w8(SRC, SOFF + i, 0x20u);
    CHECK(pr_u7f_coin_type_0b93_cpu(ram, RAM, &cpu, 3u, &callee) == PR_U7F_0B93_LIMIT && callee == 0);
    CHECK(cpu.ip == 0x0BB3u && r8(SS, BP1 - 0x102u) == 5u && (cpu.flags & 0x0041u) == 0u);   /* INC 4->5: ZF, CF clear */
    cpu = world("Gold", 0x5Cu, 0x0202u);                                  /* the UpCase site rewritten */
    w8(CS, 0x0BE7u, 0x2Bu);
    CHECK(pr_u7f_coin_type_0b93_cpu(ram, RAM, &cpu, CAP, &callee) == PR_U7F_0B93_CODE && callee == 0);
    CHECK(cpu.ip == 0x0BE6u && cpu.ax == 'o' && cpu.di == 2u && (cpu.flags & 0x08D5u) == 0u);   /* INC AX 1->2: PF odd, ZF clear */
    cpu = world("Silver", 0x5Cu, 0x0202u);                                /* the first store site rewritten */
    w8(CS, 0x0BA9u, 0x90u);
    CHECK(pr_u7f_coin_type_0b93_cpu(ram, RAM, &cpu, CAP, &callee) == PR_U7F_0B93_CODE && callee == -1);
    CHECK(cpu.ip == 0x0BA9u && cpu.ax == 0x00FFu && cpu.sp == ((BP1 - 0x102u - 10u) & 0xFFFFu));
    cpu = world("Silver", 0x5Cu, 0x0202u);                                /* D over the store's own return frame */
    w16(SS, SP0 + 4u, BP1 - 0x110u); w16(SS, SP0 + 6u, SS);
    CHECK(pr_u7f_coin_type_0b93_cpu(ram, RAM, &cpu, CAP, &callee) == PR_U7F_0B93_TRANSFER && callee == 0);
    CHECK(cpu.ip == (4u | ((unsigned)'l' << 8)));                         /* the literal's length and first letter */
}

static void arguments(void)
{
    pr_u7f_0b93_cpu cpu = world("Gold", 0x5Cu, 0x0002u), before = cpu;
    int callee = 99;
    memcpy(want, ram, RAM);
    CHECK(pr_u7f_coin_type_0b93_cpu(NULL, RAM, &cpu, CAP, &callee) == PR_U7F_0B93_ARGUMENT && callee == -1);
    CHECK(pr_u7f_coin_type_0b93_cpu(ram, RAM, NULL, CAP, NULL) == PR_U7F_0B93_ARGUMENT);
    CHECK(pr_u7f_coin_type_0b93_cpu(ram, RAM - 1u, &cpu, CAP, NULL) == PR_U7F_0B93_ARGUMENT);
    CHECK(pr_u7f_coin_type_0b93_cpu(ram, RAM + 1u, &cpu, CAP, NULL) == PR_U7F_0B93_ARGUMENT);
    CHECK(memcmp(&cpu, &before, sizeof cpu) == 0 && memcmp(ram, want, RAM) == 0);
}

int main(void)
{
    ram = malloc(RAM);
    want = malloc(RAM);
    CHECK(ram != NULL && want != NULL);
    letters();
    stops();
    arguments();
    free(ram);
    free(want);
    printf("u7f +0b93: %u checks passed\n", checks);
    return 0;
}
