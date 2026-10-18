/* Focused synthetic test for raw-CPU U3B0 +00CB composing the SYSTEM string store, FillChar and set membership
 * (docs/re/U3B0_ITEM_SPLIT_00CB.md). No game bytes: only the three LCALL encodings and a random 32-byte set are
 * placed in the code segment. An independent reference of the copy, the clear and the item split predicts the
 * parent's pairs and count, the local frame, every register and the final flags. */
#include "poolrad/recovered/overlay_u3b0_item_split_00cb_cpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM PR_U3B0_00CB_RAM_SIZE
#define CHECK(x)                                                                  \
    do {                                                                          \
        if (!(x)) {                                                               \
            fprintf(stderr, "u3b0 00cb line %d: %s\n", __LINE__, #x);             \
            exit(1);                                                              \
        }                                                                         \
        ++checks;                                                                 \
    } while (0)

enum { CS = 0x8800, SYS = 0x0AF8, DS = 0x1800, SS = 0x2800, SP0 = 0x8000, BP1 = SP0 - 2, LINK = SP0 + 0x100, SSEG = 0x6000,
       RET_IP = 0x7000, RET_CS = 0x9001, BP0 = 0x7777, FLAGS0 = 0x0602 };

static uint8_t *ram, *want;
static unsigned checks;
static unsigned seed = 0x00CBu;

static size_t at(unsigned seg, unsigned off) { return (((size_t)seg << 4) + (off & 0xFFFFu)) & (RAM - 1u); }
static void w8(unsigned seg, unsigned off, unsigned v) { ram[at(seg, off)] = (uint8_t)v; }
static uint8_t r8(unsigned seg, unsigned off) { return ram[at(seg, off)]; }
static void w16(unsigned seg, unsigned off, unsigned v) { w8(seg, off, v & 0xFFu); w8(seg, off + 1u, v >> 8); }
static unsigned r16(unsigned seg, unsigned off) { return r8(seg, off) | (unsigned)r8(seg, off + 1u) << 8; }
static unsigned next(void) { seed = seed * 1103515245u + 12345u; return seed >> 16; }

static void lcall_at(unsigned cs, unsigned site, unsigned entry)
{
    w8(cs, site, 0x9Au); w16(cs, site + 1u, entry); w16(cs, site + 3u, SYS);
}

static void lcall(unsigned site, unsigned entry) { lcall_at(CS, site, entry); }

/* The world: code-segment LCALLs and set at `cs`, the source string at SSEG:soff, the parent link and the entry frame. */
static pr_u3b0_00cb_cpu world_at(unsigned cs, const uint8_t *set, const uint8_t *text, unsigned length, unsigned soff)
{
    pr_u3b0_00cb_cpu cpu = {0x1234u, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu, BP0, SP0, (uint16_t)cs, 0x00CBu, DS, 0x4444u, SS, FLAGS0};
    memset(ram, 0x5C, RAM);
    lcall_at(cs, 0x00DFu, 0x0B25u); lcall_at(cs, 0x00F4u, 0x16A8u); lcall_at(cs, 0x012Bu, 0x0DC0u);
    for (unsigned b = 0u; b < 32u; ++b) w8(cs, 0x00ABu + b, set[b]);
    w8(SSEG, soff, length);
    for (unsigned i = 0u; i < length; ++i) w8(SSEG, soff + 1u + i, text[i]);
    w16(SS, SP0, RET_IP); w16(SS, SP0 + 2u, RET_CS); w16(SS, SP0 + 4u, LINK); w16(SS, SP0 + 6u, soff); w16(SS, SP0 + 8u, SSEG);
    return cpu;
}

static pr_u3b0_00cb_cpu world(const uint8_t *set, const uint8_t *text, unsigned length, unsigned soff)
{
    return world_at(CS, set, text, length, soff);
}

static int member(const uint8_t *set, unsigned ch) { return ((unsigned)set[ch >> 3] >> (ch & 7u)) & 1u; }

static int parity_even(unsigned v)
{
    unsigned ones = 0u;
    for (unsigned b = 0u; b < 8u; ++b) ones += (v >> b) & 1u;
    return (ones & 1u) == 0u;
}

/* ADD of two words: the six arithmetic flags. */
static unsigned add_flags(unsigned a, unsigned b)
{
    unsigned r = (a + b) & 0xFFFFu, f = 0u;
    if (a + b > 0xFFFFu) f |= 0x0001u;
    if (parity_even(r & 0xFFu)) f |= 0x0004u;
    if ((a ^ b ^ r) & 0x10u) f |= 0x0010u;
    if (r == 0u) f |= 0x0040u;
    if (r & 0x8000u) f |= 0x0080u;
    if (!((a ^ b) & 0x8000u) && ((a ^ r) & 0x8000u)) f |= 0x0800u;
    return f;
}

/* The reference split over the stored copy; fills `want` and returns k. */
static unsigned reference(const uint8_t *set, const uint8_t *text, unsigned length)
{
    unsigned n = length < 40u ? length : 40u, k = 1u;
    uint8_t pairs[0x200];
    memset(pairs, 0, sizeof pairs);
    for (unsigned j = 0u; j < sizeof pairs; ++j) pairs[j] = want[at(SS, LINK - 0x8Fu + j)];
    for (unsigned j = 0u; j < 40u; ++j) pairs[2u + j] = 0u;                    /* the 40 bytes from L-8Dh */
    for (unsigned i = 1u; i <= n; ++i) {
        if (!member(set, text[i - 1u])) continue;
        if (pairs[2u * k] == 0u) { pairs[2u * k] = (uint8_t)i; continue; }
        pairs[2u * k + 1u] = (uint8_t)(i - 2u);
        k = (k + 1u) & 0xFFu;
        pairs[2u * k] = (uint8_t)i;
    }
    pairs[2u * k + 1u] = (uint8_t)n;
    pairs[1u] = (uint8_t)k;
    for (unsigned j = 0u; j < sizeof pairs; ++j) want[at(SS, LINK - 0x8Fu + j)] = pairs[j];
    return k;
}

static void run_case(const uint8_t *set, const uint8_t *text, unsigned length, unsigned soff)
{
    pr_u3b0_00cb_cpu cpu = world(set, text, length, soff);
    unsigned n = length < 40u ? length : 40u, k, last = n ? text[n - 1u] : 0u;
    int callee = 99;
    memcpy(want, ram, RAM);
    k = reference(set, text, length);
    CHECK(pr_u3b0_item_split_00cb_cpu(ram, RAM, &cpu, &callee) == PR_U3B0_00CB_OK && callee == 0);
    CHECK(r8(SS, BP1 - 0x29u) == n && memcmp(&ram[at(SS, BP1 - 0x28u)], text, n) == 0);
    CHECK(r8(SS, BP1 - 0x2Au) == k && r8(SS, BP1 - 0x2Cu) == n && r8(SS, BP1 - 0x2Eu) == n && r16(SS, BP1) == BP0);
    CHECK(n == 0u ? r8(SS, BP1 - 0x2Bu) == 0x5Cu : r8(SS, BP1 - 0x2Bu) == n);
    CHECK(cpu.ax == (((2u * k) & 0xFF00u) | k) && cpu.dx == ((DS & 0xFF00u) | n) && cpu.di == LINK);
    CHECK(cpu.bx == (n ? last >> 3 : BP1 - 0x3Au) && cpu.cx == (n ? (last & 7u) : 0u) && cpu.es == (n ? CS : SS));
    CHECK(cpu.si == ((soff + 1u + n) & 0xFFFFu) && cpu.ds == DS && cpu.ss == SS);
    CHECK(cpu.bp == BP0 && cpu.sp == SP0 + 10u && cpu.cs == RET_CS && cpu.ip == RET_IP);
    CHECK(cpu.flags == ((FLAGS0 & ~0x0400u & ~0x08D5u) | add_flags(LINK, 2u * k)));
    for (unsigned off = SP0 - 0x40u; off < SP0 + 10u; ++off) want[at(SS, off)] = ram[at(SS, off)];
    CHECK(memcmp(ram, want, RAM) == 0);
}

static void random_set(uint8_t *set, unsigned density)
{
    for (unsigned b = 0u; b < 32u; ++b) {
        set[b] = 0u;
        for (unsigned bit = 0u; bit < 8u; ++bit) if (next() % 100u < density) set[b] |= (uint8_t)(1u << bit);
    }
}

static void splits(void)
{
    uint8_t set[32], text[64];
    for (unsigned n = 0u; n < 2000u; ++n) {
        unsigned length = next() % 56u, soff = (next() & 0x7FFEu) + (n & 1u);
        random_set(set, next() % 100u);
        for (unsigned i = 0u; i < length; ++i) text[i] = (uint8_t)next();
        run_case(set, text, length, soff);
    }
}

static void shapes(void)
{
    uint8_t set[32], text[64];
    memset(set, 0, sizeof set);
    for (unsigned ch = 'A'; ch <= 'Z'; ++ch) set[ch >> 3] |= (uint8_t)(1u << (ch & 7u));
    run_case(set, text, 0u, 0x0100u);                                      /* empty: no scan, end[1] := 0, count 1 */
    memcpy(text, "no capitals here", 16u);
    run_case(set, text, 16u, 0x0100u);                                     /* no member: start[1] stays 0 */
    memcpy(text, "AB", 2u);
    run_case(set, text, 2u, 0x0100u);                                      /* members at 1 and 2: end[1] = 0 */
    memcpy(text, " One Two Three", 14u);
    run_case(set, text, 14u, 0x0100u);
    memset(text, 'Q', 64u);
    run_case(set, text, 64u, 0x0100u);                                     /* 40 members: pairs run past the cleared 40 bytes */
    run_case(set, text, 41u, 0xFFFAu);                                     /* the source wraps its segment */
}

static void stops(void)
{
    uint8_t set[32], text[8];
    pr_u3b0_00cb_cpu cpu;
    int callee = 99;
    static const unsigned site[3] = {0x00DFu, 0x00F4u, 0x012Bu};
    memset(set, 0xFF, sizeof set);
    memcpy(text, "ABCDE", 5u);
    for (unsigned s = 0u; s < 3u; ++s) {                                   /* a wrong entry word at each site: CODE before it */
        cpu = world(set, text, 5u, 0x0100u);
        w16(CS, site[s] + 1u, 0x1234u);
        CHECK(pr_u3b0_item_split_00cb_cpu(ram, RAM, &cpu, &callee) == PR_U3B0_00CB_CODE && cpu.ip == site[s]);
        CHECK(callee == (s == 0u ? -1 : 0) && cpu.cs == CS);
    }
    cpu = world(set, text, 5u, 0x0100u);                                   /* SUB SP,2Eh's flags at the first site */
    w8(CS, 0x00DFu, 0x90u);
    cpu.flags = 0x0AD7u;                                                   /* every arithmetic flag set on entry */
    CHECK(pr_u3b0_item_split_00cb_cpu(ram, RAM, &cpu, &callee) == PR_U3B0_00CB_CODE);
    CHECK(cpu.sp == BP1 - 0x2Eu - 10u && cpu.flags == 0x0202u);            /* 7FFE - 2E = 7FD0: none set, odd parity */
    cpu = world(set, text, 5u, 0x0100u);                                   /* XOR AH,AH's flags at the set site */
    w8(CS, 0x012Bu, 0x90u);
    CHECK(pr_u3b0_item_split_00cb_cpu(ram, RAM, &cpu, &callee) == PR_U3B0_00CB_CODE && (cpu.flags & 0x08D5u) == 0x0044u);
    CHECK(cpu.ax == 'A' && r16(SS, cpu.sp) == 0x00ABu && r16(SS, cpu.sp + 2u) == CS && r16(SS, cpu.sp + 4u) == 'A');
    cpu = world(set, text, 5u, 0x0100u);                                   /* FillChar clears its own return frame */
    w16(SS, SP0 + 4u, (SP0 - 0x3Cu + 0x8Du) & 0xFFFFu);
    CHECK(pr_u3b0_item_split_00cb_cpu(ram, RAM, &cpu, &callee) == PR_U3B0_00CB_TRANSFER && callee == 0);
    CHECK(cpu.ip == 0u && cpu.cs == 0u && (cpu.flags & 0x0400u) == 0u);
    cpu = world(set, text, 5u, 0x0100u);                                   /* entered at another offset: the sites move with it */
    memmove(&ram[at(CS, 0x10CBu)], &ram[at(CS, 0x00CBu)], 0x100u);
    lcall(0x10DFu, 0x0B25u); lcall(0x10F4u, 0x16A8u); lcall(0x112Bu, 0x0DC0u);
    cpu.ip = 0x10CBu;
    CHECK(pr_u3b0_item_split_00cb_cpu(ram, RAM, &cpu, &callee) == PR_U3B0_00CB_OK && r8(SS, LINK - 0x8Eu) == 5u);
}

/* Pairs that climb through the call frames onto the locals (L = BP + 27h, 40 members) still end the scan. */
static void aliasing(void)
{
    uint8_t set[32], text[40];
    pr_u3b0_00cb_cpu cpu;
    int callee = 99;
    memset(set, 0xFF, sizeof set);
    memset(text, 'Q', sizeof text);
    cpu = world(set, text, 40u, 0x0100u);                                  /* CS 8800h: start[25] is the pushed CS low byte 00h, so k sticks */
    w16(SS, SP0 + 4u, BP1 + 0x27u);
    CHECK(pr_u3b0_item_split_00cb_cpu(ram, RAM, &cpu, &callee) == PR_U3B0_00CB_OK && callee == 0);
    CHECK(r8(SS, BP1 - 0x2Bu) == 40u && r8(SS, BP1 - 0x2Au) == 25u && r8(SS, BP1 - 0x2Eu) == 40u && r8(SS, BP1 - 0x67u) == 25u);
    cpu = world_at(0x8811u, set, text, 40u, 0x0100u);                      /* CS 8811h: k reaches start[29], the bound, which becomes i */
    w16(SS, SP0 + 4u, BP1 + 0x27u);
    CHECK(pr_u3b0_item_split_00cb_cpu(ram, RAM, &cpu, &callee) == PR_U3B0_00CB_OK && callee == 0);
    CHECK(r8(SS, BP1 - 0x2Bu) == 29u && r8(SS, BP1 - 0x2Au) == 29u && r8(SS, BP1 - 0x2Eu) == 29u && r8(SS, BP1 - 0x67u) == 29u);
    CHECK(cpu.ip == RET_IP && cpu.cs == RET_CS && cpu.sp == SP0 + 10u);
}

static void arguments(void)
{
    uint8_t set[32] = {0}, text[4] = {'a', 'b', 'c', 'd'};
    pr_u3b0_00cb_cpu cpu, before;
    int callee = 99;
    cpu = world(set, text, 4u, 0x0100u);
    before = cpu;
    memcpy(want, ram, RAM);
    CHECK(pr_u3b0_item_split_00cb_cpu(NULL, RAM, &cpu, &callee) == PR_U3B0_00CB_ARGUMENT && callee == -1);
    CHECK(pr_u3b0_item_split_00cb_cpu(ram, RAM, NULL, NULL) == PR_U3B0_00CB_ARGUMENT);
    CHECK(pr_u3b0_item_split_00cb_cpu(ram, RAM - 1u, &cpu, NULL) == PR_U3B0_00CB_ARGUMENT);
    CHECK(pr_u3b0_item_split_00cb_cpu(ram, RAM + 1u, &cpu, NULL) == PR_U3B0_00CB_ARGUMENT);
    CHECK(memcmp(&cpu, &before, sizeof cpu) == 0 && memcmp(ram, want, RAM) == 0);
}

int main(void)
{
    ram = malloc(RAM);
    want = malloc(RAM);
    CHECK(ram != NULL && want != NULL);
    splits();
    shapes();
    stops();
    aliasing();
    arguments();
    free(ram);
    free(want);
    printf("u3b0 +00cb: %u checks passed\n", checks);
    return 0;
}
