/* Focused synthetic test for raw-CPU U3B0 +08D7 and +0923 composing the SYSTEM 0B25 string store
 * (docs/re/U3B0_SPACE_SCAN_CPU.md). No game bytes: only the two five-byte far-call sites and synthetic
 * strings. Every case compares the whole 1 MiB buffer with an expected copy built from a byte-level
 * reference of the scans. */
#include "poolrad/recovered/overlay_u3b0_space_scan_cpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM PR_U3B0S_RAM_SIZE
#define CHECK(x)                                                                  \
    do {                                                                          \
        if (!(x)) {                                                               \
            fprintf(stderr, "u3b0 space scan line %d: %s\n", __LINE__, #x);       \
            exit(1);                                                              \
        }                                                                         \
        ++checks;                                                                 \
    } while (0)

enum { CS = 0x3800, SYS = 0x0AF8, SS = 0x2800, SP0 = 0x8000, SRC = 0x5000, SOFF = 0x0100, DS0 = 0x1111,
       RET_IP = 0x7A00, RET_CS = 0x7A01, LINK = 0xC0DE, BP0 = 0x7777, CAP = 0x28, LEAD = 0, TRAIL = 1 };

static uint8_t *ram, *want;
static unsigned checks;

static size_t at(unsigned seg, unsigned off) { return (((size_t)seg << 4) + (off & 0xFFFFu)) & (RAM - 1u); }
static void w8(unsigned seg, unsigned off, unsigned v) { ram[at(seg, off)] = (uint8_t)v; }
static void w16(unsigned seg, unsigned off, unsigned v) { w8(seg, off, v & 0xFFu); w8(seg, off + 1u, v >> 8); }
static void e8(unsigned seg, unsigned off, unsigned v) { want[at(seg, off)] = (uint8_t)v; }
static void e16(unsigned seg, unsigned off, unsigned v) { e8(seg, off, v & 0xFFu); e8(seg, off + 1u, v >> 8); }

static unsigned entry_of(int which) { return which == LEAD ? 0x08D7u : 0x0923u; }
static unsigned site_of(int which) { return which == LEAD ? 0x08EBu : 0x0937u; }

static uint16_t cmp_flags(unsigned flags, unsigned left, unsigned right)
{
    unsigned result = (left - right) & 0xFFu, ones = 0u;
    for (unsigned bit = 0u; bit < 8u; ++bit) ones += (result >> bit) & 1u;
    flags &= ~0x08D5u;
    if (left < right) flags |= 0x0001u;
    if ((ones & 1u) == 0u) flags |= 0x0004u;
    if ((left ^ right ^ result) & 0x10u) flags |= 0x0010u;
    if (result == 0u) flags |= 0x0040u;
    if (result & 0x80u) flags |= 0x0080u;
    if ((left ^ right) & (left ^ result) & 0x80u) flags |= 0x0800u;
    return (uint16_t)flags;
}

/* Code: the far call at each site (9A 25 0B F8 0A) relative to `base`; the frame at SS:sp; S = sseg:soff. */
static pr_u3b0s_cpu world(unsigned base, unsigned sp, unsigned sseg, unsigned soff, unsigned n, const uint8_t *chars,
                          unsigned count, unsigned flags, int which)
{
    pr_u3b0s_cpu cpu = {0x1234u, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu, BP0, (uint16_t)sp, CS,
                        (uint16_t)(base + entry_of(which)), DS0, 0x4444u, SS, (uint16_t)flags};
    for (int w = LEAD; w <= TRAIL; ++w) {
        unsigned site = base + site_of(w);
        w8(CS, site, 0x9Au); w16(CS, site + 1u, 0x0B25u); w16(CS, site + 3u, SYS);
    }
    w8(sseg, soff, n);
    for (unsigned i = 0u; i < count; ++i) w8(sseg, soff + 1u + i, chars[i]);
    w16(SS, sp, RET_IP); w16(SS, sp + 2u, RET_CS); w16(SS, sp + 4u, LINK); w16(SS, sp + 6u, soff); w16(SS, sp + 8u, sseg);
    return cpu;
}

/* Byte-level reference: copy, scan, and every byte the routine writes. */
static void expect(const pr_u3b0s_cpu *cpu, unsigned base, unsigned sp, unsigned sseg, unsigned soff, int which,
                   const pr_u3b0s_cpu *entry)
{
    unsigned n, k, i, checked = 0u, left, right, bp = (sp - 2u) & 0xFFFFu, d = (bp - 0x2Au) & 0xFFFFu;
    uint8_t local[CAP + 1];
    memcpy(want, ram, RAM);
    n = ram[at(sseg, soff)];
    k = n < CAP ? n : CAP;
    local[0] = (uint8_t)k;
    for (i = 1u; i <= k; ++i) local[i] = ram[at(sseg, soff + i)];
    /* frame bytes in write order */
    e16(SS, sp - 2u, BP0);
    e16(SS, sp - 0x30u, sseg); e16(SS, sp - 0x32u, soff); e16(SS, sp - 0x34u, SS); e16(SS, sp - 0x36u, d);
    e16(SS, sp - 0x38u, 0x28u); e16(SS, sp - 0x3Au, CS); e16(SS, sp - 0x3Cu, base + site_of(which) + 5u);
    for (i = 0u; i <= k; ++i) e8(SS, d + i, local[i]);
    if (which == LEAD) {
        e8(SS, bp - 0x2Cu, k);
        for (i = 1u;; ++i) {
            if (i >= k) { left = i; right = k; break; }
            checked = i;
            if (local[i] != 0x20u) { left = local[i]; right = 0x20u; break; }
        }
    } else {
        for (i = k;; --i) {
            if (i <= 1u) { left = i; right = 1u; break; }
            checked = i;
            if (local[i] != 0x20u) { left = local[i]; right = 0x20u; break; }
        }
    }
    e8(SS, bp - 0x2Bu, i);
    e8(SS, bp - 1u, i);
    CHECK(cpu->ax == i);
    CHECK(cpu->bx == ((sp - 0x3Cu) & 0xFFFFu) && cpu->cx == 0u && cpu->dx == DS0);
    CHECK(cpu->si == ((soff + 1u + k) & 0xFFFFu));
    CHECK(cpu->di == (checked ? checked : ((d + 1u + k) & 0xFFFFu)));
    CHECK(cpu->bp == BP0 && cpu->sp == ((sp + 10u) & 0xFFFFu));
    CHECK(cpu->cs == RET_CS && cpu->ip == RET_IP);
    CHECK(cpu->ds == DS0 && cpu->es == SS && cpu->ss == SS);
    CHECK(cpu->flags == cmp_flags(entry->flags & ~0x0400u, left, right));
}

static unsigned run_case(int which, unsigned base, unsigned sp, unsigned sseg, unsigned soff, unsigned n, const uint8_t *chars,
                         unsigned count, unsigned flags)
{
    pr_u3b0s_cpu cpu, entry;
    int callee = 99;
    memset(ram, 0x5C, RAM);
    cpu = world(base, sp, sseg, soff, n, chars, count, flags, which);
    entry = cpu;
    CHECK((which == LEAD ? pr_u3b0_lead_spaces_08d7_cpu : pr_u3b0_trail_spaces_0923_cpu)(ram, RAM, &cpu, &callee) == PR_U3B0S_OK);
    CHECK(callee == 0);
    {
        pr_u3b0s_cpu check_cpu = cpu;
        uint8_t *after = malloc(RAM);
        CHECK(after != NULL);
        memcpy(after, ram, RAM);
        /* rebuild the entry memory to derive the expectation, then compare with what the model left */
        memset(ram, 0x5C, RAM);
        (void)world(base, sp, sseg, soff, n, chars, count, flags, which);
        expect(&check_cpu, base, sp, sseg, soff, which, &entry);
        CHECK(memcmp(after, want, RAM) == 0);
        free(after);
    }
    return cpu.ax & 0xFFu;
}

static unsigned spaced(uint8_t *chars, unsigned k, unsigned lead, unsigned trail)
{
    for (unsigned i = 0u; i < k; ++i)
        chars[i] = (uint8_t)(i < lead || i >= k - trail ? 0x20u : 0x41u + (i % 26u));
    return k;
}

static void directed(void)
{
    uint8_t chars[256];
    memcpy(chars, "   AB  ", 7);
    CHECK(run_case(LEAD, 0u, SP0, SRC, SOFF, 7u, chars, 7u, 0x0002u) == 4u);   /* first non-space */
    CHECK(run_case(TRAIL, 0u, SP0, SRC, SOFF, 7u, chars, 7u, 0x0002u) == 5u);  /* last non-space */
    CHECK(run_case(LEAD, 0u, SP0, SRC, SOFF, 0u, chars, 0u, 0x0ED7u) == 1u);   /* empty: i starts at 1 */
    CHECK(run_case(TRAIL, 0u, SP0, SRC, SOFF, 0u, chars, 0u, 0x0ED7u) == 0u);  /* empty: i starts at n */
    chars[0] = 0x20u;
    CHECK(run_case(LEAD, 0u, SP0, SRC, SOFF, 1u, chars, 1u, 0x0002u) == 1u);
    CHECK(run_case(TRAIL, 0u, SP0, SRC, SOFF, 1u, chars, 1u, 0x0002u) == 1u);
    memset(chars, 0x20, sizeof chars);
    CHECK(run_case(LEAD, 0u, SP0, SRC, SOFF, CAP, chars, CAP, 0x0002u) == CAP);  /* all spaces stop at n */
    CHECK(run_case(TRAIL, 0u, SP0, SRC, SOFF, CAP, chars, CAP, 0x0002u) == 1u);  /* and at 1 */
    /* caller length above the 28h local: only the first 40 characters are copied and scanned */
    CHECK(run_case(LEAD, 0u, SP0, SRC, SOFF, 0x50u, chars, 0x50u, 0x0002u) == CAP);
    spaced(chars, 0x50u, 0u, 0x28u);                                        /* spaces only past the copied 40 */
    CHECK(run_case(TRAIL, 0u, SP0, SRC, SOFF, 0x50u, chars, 0x50u, 0x0002u) == CAP);
    CHECK(run_case(LEAD, 0u, SP0, SRC, SOFF, 0xFFu, chars, 0xFFu, 0x0002u) == 1u);
}

/* Every copied length with every leading or trailing space count, and the other end randomized. */
static void sweep(void)
{
    static const unsigned flags[] = {0x0002u, 0x0ED7u, 0x0602u, 0x08D7u};
    uint8_t chars[CAP];
    unsigned seed = 1u;
    for (unsigned k = 0u; k <= CAP; ++k)
        for (unsigned count = 0u; count <= k; ++count)
            for (int which = LEAD; which <= TRAIL; ++which) {
                unsigned other, result;
                seed = seed * 1103515245u + 12345u;
                other = (seed >> 16) % (k - count + 1u);
                spaced(chars, k, which == LEAD ? count : other, which == LEAD ? other : count);
                result = run_case(which, 0u, SP0, SRC, SOFF, k, chars, k, flags[(k + count) & 3u]);
                if (count + other == k)                                       /* all spaces */
                    CHECK(result == (which == LEAD ? (k > 1u ? k : 1u) : (k < 1u ? k : 1u)));
                else
                    CHECK(result == (which == LEAD ? count + 1u : k - count));
            }
}

/* The entry IP may sit at another offset: the site and return IP follow it. Stacks wrap at 16 bits. */
static void placement(void)
{
    uint8_t chars[CAP];
    spaced(chars, 12u, 3u, 4u);
    CHECK(run_case(LEAD, 0x1000u, SP0, SRC, SOFF, 12u, chars, 12u, 0x0002u) == 4u);
    CHECK(run_case(TRAIL, 0x1000u, SP0, SRC, SOFF, 12u, chars, 12u, 0x0002u) == 8u);
    CHECK(run_case(LEAD, 0u, 0x0020u, SRC, SOFF, 12u, chars, 12u, 0x0002u) == 4u);   /* BP-2Ah wraps below 0 */
    CHECK(run_case(TRAIL, 0u, 0x0020u, SRC, SOFF, 12u, chars, 12u, 0x0002u) == 8u);
    CHECK(run_case(LEAD, 0u, SP0, SRC, 0xFFFAu, 12u, chars, 12u, 0x0002u) == 4u);    /* S wraps in its segment */
    CHECK(run_case(TRAIL, 0u, SP0, SS, (SP0 - 0x2Cu) & 0xFFFFu, 12u, chars, 12u, 0x0002u) == 8u);  /* S is the local */
}

static void code_stop(void)
{
    for (int which = LEAD; which <= TRAIL; ++which)
        for (unsigned patch = 0u; patch < 2u; ++patch) {
            pr_u3b0s_cpu cpu;
            int callee = 99;
            unsigned site = site_of(which), sp = (SP0 - 0x38u) & 0xFFFFu;
            memset(ram, 0x5C, RAM);
            cpu = world(0u, SP0, SRC, SOFF, 3u, (const uint8_t *)"ABC", 3u, 0x0ED7u, which);
            w8(CS, site + patch, patch ? 0x26u : 0x90u);
            memcpy(want, ram, RAM);
            e16(SS, SP0 - 2u, BP0);
            e16(SS, SP0 - 0x30u, SRC); e16(SS, SP0 - 0x32u, SOFF); e16(SS, SP0 - 0x34u, SS);
            e16(SS, SP0 - 0x36u, SP0 - 0x2Cu); e16(SS, SP0 - 0x38u, 0x28u);
            CHECK((which == LEAD ? pr_u3b0_lead_spaces_08d7_cpu : pr_u3b0_trail_spaces_0923_cpu)(ram, RAM, &cpu, &callee) == PR_U3B0S_CODE);
            CHECK(callee == -1 && cpu.ip == site && cpu.cs == CS && cpu.sp == sp && cpu.bp == ((SP0 - 2u) & 0xFFFFu));
            CHECK(cpu.ax == 0x28u && cpu.di == SP0 - 0x2Cu && cpu.es == SRC && cpu.ds == DS0);
            CHECK(cpu.flags == ((0x0ED7u & ~0x08D5u) | 0x0004u));          /* SUB 7FFEh,2Ch = 7FD2h: only PF */
            CHECK(memcmp(ram, want, RAM) == 0);
        }
}

static void arguments(void)
{
    pr_u3b0s_cpu cpu, before;
    int callee = 99;
    memset(ram, 0x5C, RAM);
    cpu = world(0u, SP0, SRC, SOFF, 3u, (const uint8_t *)"ABC", 3u, 0x0002u, LEAD);
    before = cpu;
    memcpy(want, ram, RAM);
    CHECK(pr_u3b0_lead_spaces_08d7_cpu(NULL, RAM, &cpu, &callee) == PR_U3B0S_ARGUMENT && callee == -1);
    callee = 99;
    CHECK(pr_u3b0_trail_spaces_0923_cpu(ram, RAM, NULL, &callee) == PR_U3B0S_ARGUMENT && callee == -1);
    CHECK(pr_u3b0_lead_spaces_08d7_cpu(ram, RAM - 1u, &cpu, NULL) == PR_U3B0S_ARGUMENT);
    CHECK(pr_u3b0_trail_spaces_0923_cpu(ram, RAM + 1u, &cpu, NULL) == PR_U3B0S_ARGUMENT);
    CHECK(memcmp(&cpu, &before, sizeof cpu) == 0 && memcmp(ram, want, RAM) == 0);
    CHECK(pr_u3b0_lead_spaces_08d7_cpu(ram, RAM, &cpu, NULL) == PR_U3B0S_OK && (cpu.ax & 0xFFu) == 1u);  /* NULL status is fine */
}

int main(void)
{
    ram = malloc(RAM);
    want = malloc(RAM);
    CHECK(ram != NULL && want != NULL);
    directed();
    sweep();
    placement();
    code_stop();
    arguments();
    free(ram);
    free(want);
    printf("u3b0 space scans: %u checks passed\n", checks);
    return 0;
}
