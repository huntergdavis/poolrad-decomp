/* Focused synthetic test for raw-CPU U3C2 +002E composing SYSTEM Move (docs/re/U3C2_RECORD_SORT_002E.md).
 * No game bytes: only the three five-byte far-call sites and synthetic 3-byte records. Every case compares
 * all RAM outside the routine's stack frame with a byte-level reference sort, plus the frame's counters. */
#include "poolrad/recovered/overlay_u3c2_record_sort_002e_cpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM PR_U3C2_002E_RAM_SIZE
#define CHECK(x)                                                                  \
    do {                                                                          \
        if (!(x)) {                                                               \
            fprintf(stderr, "u3c2 002e line %d: %s\n", __LINE__, #x);             \
            exit(1);                                                              \
        }                                                                         \
        ++checks;                                                                 \
    } while (0)

enum { CS = 0x3800, SYS = 0x0AF8, DS = 0x1800, SS = 0x2800, SP0 = 0x8000, BP1 = SP0 - 2, BP0 = 0x7777,
       RET_IP = 0x7A00, RET_CS = 0x7A01, CAP = 100000 };

static uint8_t *ram, *want;
static unsigned checks;

static size_t at(unsigned seg, unsigned off) { return (((size_t)seg << 4) + (off & 0xFFFFu)) & (RAM - 1u); }
static void w8(unsigned seg, unsigned off, unsigned v) { ram[at(seg, off)] = (uint8_t)v; }
static uint8_t r8(unsigned seg, unsigned off) { return ram[at(seg, off)]; }
static void w16(unsigned seg, unsigned off, unsigned v) { w8(seg, off, v & 0xFFu); w8(seg, off + 1u, v >> 8); }
static unsigned r16(unsigned seg, unsigned off) { return r8(seg, off) | (unsigned)r8(seg, off + 1u) << 8; }

static unsigned parity_even(unsigned v)
{
    unsigned ones = 0u;
    for (unsigned bit = 0u; bit < 8u; ++bit) ones += (v >> bit) & 1u;
    return (ones & 1u) == 0u;
}

static uint16_t cmp_flags(unsigned flags, unsigned left, unsigned right)
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

static pr_u3c2_002e_cpu world(unsigned base, const uint8_t *records, unsigned count, unsigned flags)
{
    static const unsigned sites[3] = {0x0131u, 0x0160u, 0x0181u};
    pr_u3c2_002e_cpu cpu = {0x1234u, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu, BP0, SP0, CS, (uint16_t)(base + 0x002Eu),
                            DS, 0x4444u, SS, (uint16_t)flags};
    memset(ram, 0x5C, RAM);
    for (unsigned s = 0u; s < 3u; ++s) {
        w8(CS, base + sites[s], 0x9Au); w16(CS, base + sites[s] + 1u, 0x0A3Fu); w16(CS, base + sites[s] + 3u, SYS);
    }
    for (unsigned i = 0u; i < count; ++i) w8(DS, 0x6718u + i, records[i]);
    w16(SS, SP0, RET_IP); w16(SS, SP0 + 2u, RET_CS);
    return cpu;
}

/* Byte-level reference of the sort over the records at DS:6718; returns the taken loop-backs. */
static unsigned reference(uint8_t *r, unsigned *swaps)
{
    unsigned n = r[2], loopbacks = 0u, last_i, i;
    *swaps = 0u;
    if (n <= 1u) return 0u;
    last_i = (n - 1u) & 0xFFu;
    for (i = 1u;; i = (i + 1u) & 0xFFu) {
        unsigned last_j = r[2], j = (i + 1u) & 0xFFu;
        if (j <= last_j) {
            for (;; j = (j + 1u) & 0xFFu) {
                unsigned ai = r[3u * i + 2u], aj = r[3u * j + 2u], bi = r[3u * i + 1u], bj = r[3u * j + 1u];
                if (bj < bi || (bj == bi && aj < ai && (aj % 2u) <= (ai % 2u))) {
                    uint8_t temp[3];
                    memcpy(temp, &r[3u * i], 3u);
                    memmove(&r[3u * i], &r[3u * j], 3u);
                    memcpy(&r[3u * j], temp, 3u);
                    ++*swaps;
                }
                if (j == last_j) break;
                ++loopbacks;
            }
        }
        if (i == last_i) break;
        ++loopbacks;
    }
    return loopbacks;
}

static void run_case(unsigned base, const uint8_t *records, unsigned count, unsigned flags)
{
    pr_u3c2_002e_cpu cpu = world(base, records, count, flags);
    uint8_t expect[0x400];
    unsigned swaps, loopbacks, n = records[2];
    int callee = 99;
    memcpy(expect, records, count);
    loopbacks = reference(expect, &swaps);
    memcpy(want, ram, RAM);
    for (unsigned i = 0u; i < count; ++i) want[at(DS, 0x6718u + i)] = expect[i];
    CHECK(pr_u3c2_record_sort_002e_cpu(ram, RAM, &cpu, CAP, &callee) == PR_U3C2_002E_OK);
    CHECK(callee == (swaps ? 0 : -1));
    for (unsigned off = SP0 - 0x40u; off < SP0 + 4u; ++off) want[at(SS, off)] = ram[at(SS, off)];   /* the frame is checked below */
    CHECK(memcmp(ram, want, RAM) == 0);
    CHECK(cpu.bp == BP0 && cpu.sp == SP0 + 4u && cpu.cs == RET_CS && cpu.ip == RET_IP && cpu.ss == SS && cpu.ds == DS);
    CHECK(r16(SS, BP1) == BP0);
    if (n <= 1u) {
        CHECK(cpu.flags == cmp_flags(flags, n, 1u));
    } else {
        CHECK(r8(SS, BP1 - 8u) == ((n - 1u) & 0xFFu) && r8(SS, BP1 - 1u) == ((n - 1u) & 0xFFu) && r8(SS, BP1 - 9u) == n);
        CHECK((cpu.flags & 0x08D5u) == (cmp_flags(flags, n - 1u, n - 1u) & 0x08D5u));
        CHECK(((cpu.flags & 0x0400u) == 0u) == (swaps != 0u || (flags & 0x0400u) == 0u));
        (void)loopbacks;
    }
}


static void sorts(void)
{
    uint8_t records[0x400];
    unsigned seed = 0x002Eu;
    for (unsigned n = 0u; n <= 12u; ++n)                                    /* every small count, several orders */
        for (unsigned k = 0u; k < 6u; ++k) {
            for (unsigned i = 0u; i < 3u * 14u; ++i) { seed = seed * 1103515245u + 12345u; records[i] = (uint8_t)(seed >> 16); }
            for (unsigned i = 1u; i <= n; ++i) {
                if (k == 1u) records[3u * i + 1u] = (uint8_t)(n - i);           /* reversed keys */
                if (k == 2u) records[3u * i + 1u] = 7u;                         /* all keys tied: the parity rule decides */
                if (k == 3u) records[3u * i + 1u] = (uint8_t)i;                 /* already sorted */
            }
            records[2] = (uint8_t)n;
            run_case(0u, records, 3u * 14u, (k & 1u) ? 0x0ED7u : 0x0202u);
        }
    for (unsigned t = 0u; t < 60u; ++t) {                                 /* larger random counts up to 255 */
        unsigned n = (t * 37u + 5u) & 0xFFu;
        for (unsigned i = 0u; i < 3u * 256u; ++i) { seed = seed * 1103515245u + 12345u; records[i] = (uint8_t)(seed >> 16); }
        if (t & 1u) for (unsigned i = 1u; i <= n; ++i) records[3u * i + 1u] &= 3u;   /* many ties */
        records[2] = (uint8_t)n;
        run_case((t & 4u) ? 0x0400u : 0u, records, 3u * 256u, 0x0202u);
    }
}

static void stops(void)
{
    uint8_t records[48];
    pr_u3c2_002e_cpu cpu;
    int callee = 99;
    for (unsigned i = 0u; i < sizeof records; ++i) records[i] = (uint8_t)(0x40u - i);
    records[2] = 6u;
    cpu = world(0u, records, sizeof records, 0x0202u);                    /* a cap of 2: the third loop-back stops */
    CHECK(pr_u3c2_record_sort_002e_cpu(ram, RAM, &cpu, 2u, &callee) == PR_U3C2_002E_LIMIT);
    CHECK(cpu.ip == 0x0072u && r8(SS, BP1 - 2u) == 4u && r8(SS, BP1 - 1u) == 1u);
    cpu = world(0u, records, sizeof records, 0x0202u);                    /* a rewritten second site */
    w8(CS, 0x0161u, 0x40u);
    CHECK(pr_u3c2_record_sort_002e_cpu(ram, RAM, &cpu, CAP, &callee) == PR_U3C2_002E_CODE && callee == 0);
    CHECK(cpu.ip == 0x0160u && cpu.ax == 3u && cpu.di == 0x6718u + 3u);
    cpu = world(0u, records, sizeof records, 0x0202u);                    /* a rewritten first site */
    w8(CS, 0x0131u, 0x90u);
    CHECK(pr_u3c2_record_sort_002e_cpu(ram, RAM, &cpu, CAP, &callee) == PR_U3C2_002E_CODE && callee == -1);
    CHECK(cpu.ip == 0x0131u && cpu.sp == ((BP1 - 0x0Au - 10u) & 0xFFFFu));
}

static void arguments(void)
{
    uint8_t records[12] = {0};
    pr_u3c2_002e_cpu cpu = world(0u, records, sizeof records, 0x0002u), before = cpu;
    int callee = 99;
    memcpy(want, ram, RAM);
    CHECK(pr_u3c2_record_sort_002e_cpu(NULL, RAM, &cpu, CAP, &callee) == PR_U3C2_002E_ARGUMENT && callee == -1);
    CHECK(pr_u3c2_record_sort_002e_cpu(ram, RAM, NULL, CAP, NULL) == PR_U3C2_002E_ARGUMENT);
    CHECK(pr_u3c2_record_sort_002e_cpu(ram, RAM - 1u, &cpu, CAP, NULL) == PR_U3C2_002E_ARGUMENT);
    CHECK(pr_u3c2_record_sort_002e_cpu(ram, RAM + 1u, &cpu, CAP, NULL) == PR_U3C2_002E_ARGUMENT);
    CHECK(memcmp(&cpu, &before, sizeof cpu) == 0 && memcmp(ram, want, RAM) == 0);
}

int main(void)
{
    ram = malloc(RAM);
    want = malloc(RAM);
    CHECK(ram != NULL && want != NULL);
    sorts();
    stops();
    arguments();
    free(ram);
    free(want);
    printf("u3c2 +002e: %u checks passed\n", checks);
    return 0;
}
