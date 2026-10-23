/* Focused synthetic test for raw-CPU U77 +28BE composing the SYSTEM LongInt multiply and divide
 * (docs/re/U77_WEIGHTED_28BE_CPU.md). No game bytes: only the two LCALL encodings are placed in the code segment,
 * with synthetic records and weights. An independent reference of the 32-bit weighted sum, the truncating division
 * and the divide's flags predicts every register, the frame and every byte outside the stack window. */
#include "poolrad/recovered/overlay_u77_weighted_28be_cpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM PR_U77_28BE_CPU_RAM_SIZE
#define CHECK(x)                                                                  \
    do {                                                                          \
        if (!(x)) {                                                               \
            fprintf(stderr, "u77 28be line %d: %s\n", __LINE__, #x);              \
            exit(1);                                                              \
        }                                                                         \
        ++checks;                                                                 \
    } while (0)

enum { CS = 0x8800, SYS = 0x0AF8, DS = 0x1800, SS = 0x2800, SP0 = 0x8000, BP1 = SP0 - 2, PSEG = 0x6000, POFF = 0x0120,
       RET_IP = 0x7000, RET_CS = 0x9001, BP0 = 0x7777, FLAGS0 = 0x0602 };

static uint8_t *ram, *want;
static unsigned checks;
static uint32_t seed = 0x28BEu;

static size_t at(unsigned seg, unsigned off) { return (((size_t)seg << 4) + (off & 0xFFFFu)) & (RAM - 1u); }
static void w8(unsigned seg, unsigned off, unsigned v) { ram[at(seg, off)] = (uint8_t)v; }
static uint8_t r8(unsigned seg, unsigned off) { return ram[at(seg, off)]; }
static void w16(unsigned seg, unsigned off, unsigned v) { w8(seg, off, v & 0xFFu); w8(seg, off + 1u, v >> 8); }
static unsigned r16(unsigned seg, unsigned off) { return r8(seg, off) | (unsigned)r8(seg, off + 1u) << 8; }
static void w32(unsigned seg, unsigned off, uint32_t v) { w16(seg, off, v & 0xFFFFu); w16(seg, off + 2u, v >> 16); }
static uint32_t r32(unsigned seg, unsigned off) { return r16(seg, off) | (uint32_t)r16(seg, off + 2u) << 16; }
static uint32_t next(void) { seed = seed * 1103515245u + 12345u; return seed; }

static void lcall(unsigned site, unsigned entry) { w8(CS, site, 0x9Au); w16(CS, site + 1u, entry); w16(CS, site + 3u, SYS); }

static pr_u77_28be_cpu world(const unsigned *v, const uint32_t *w, uint32_t divisor, unsigned sp)
{
    pr_u77_28be_cpu cpu = {0x1234u, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu, BP0, (uint16_t)sp, CS, 0x28BEu, DS, 0x4444u, SS, FLAGS0};
    memset(ram, 0x5C, RAM);
    lcall(0x28FDu, 0x0A5Bu); lcall(0x2922u, 0x0A76u);
    for (unsigned k = 0u; k < 5u; ++k) w16(PSEG, POFF + 0x88u + 2u * k, v[k]);
    for (unsigned k = 0u; k < 5u; ++k) w32(DS, 0x0D24u + 4u * k, w[k]);
    w32(DS, 0x0D30u, divisor);                                             /* w[3] is the divisor word pair too */
    w16(SS, sp, RET_IP); w16(SS, sp + 2u, RET_CS); w16(SS, sp + 4u, POFF); w16(SS, sp + 6u, PSEG);
    return cpu;
}

static unsigned parity_even(unsigned v)
{
    unsigned ones = 0u;
    for (unsigned bit = 0u; bit < 8u; ++bit) ones += (v >> bit) & 1u;
    return (ones & 1u) == 0u;
}

static unsigned logic(unsigned flags, unsigned value)
{
    flags &= ~0x08D5u;
    if (parity_even(value & 0xFFu)) flags |= 0x0004u;
    if ((value & 0xFFFFu) == 0u) flags |= 0x0040u;
    if (value & 0x8000u) flags |= 0x0080u;
    return flags;
}

static unsigned canonical(unsigned flags) { return (flags & 0x7FD7u) | 0x0002u; }

/* The divide's final flags from its entry flags (only the non-arithmetic bits matter). */
static unsigned divide_flags(unsigned entry, int32_t a, int32_t b, uint32_t q)
{
    unsigned f1 = canonical(logic(entry, (uint32_t)b >> 16));
    if ((a < 0) != (b < 0)) {
        uint32_t uq = 0u - q, low = ~uq & 0xFFFFu, high = (~uq >> 16) & 0xFFFFu, carry = low == 0xFFFFu, result = (high + carry) & 0xFFFFu;
        unsigned final = f1 & ~0x08D5u;
        if (carry && high == 0xFFFFu) final |= 0x0001u;
        if (parity_even(result & 0xFFu)) final |= 0x0004u;
        if ((high ^ result) & 0x10u) final |= 0x0010u;
        if (result == 0u) final |= 0x0040u;
        if (result & 0x8000u) final |= 0x0080u;
        if (~high & result & 0x8000u) final |= 0x0800u;
        return final;
    }
    return f1;
}

static void run_case(const unsigned *v, const uint32_t *w, uint32_t divisor)
{
    pr_u77_28be_cpu cpu;
    uint32_t sum = 0u, dividend, q, r, magnitude, wv[5];
    int32_t a, b;
    int callee = 99;
    memcpy(wv, w, sizeof wv);
    wv[3] = divisor;
    cpu = world(v, wv, divisor, SP0);
    for (unsigned j = 0u; j < 5u; ++j) sum += wv[j] * v[j];
    dividend = sum + 100u;
    a = (int32_t)dividend; b = (int32_t)divisor;
    q = (a == INT32_MIN && b == -1) ? (uint32_t)a : (uint32_t)(a / b);
    r = dividend - q * divisor;
    magnitude = b < 0 ? 0u - divisor : divisor;
    memcpy(want, ram, RAM);
    CHECK(pr_u77_weighted_28be_cpu(ram, RAM, &cpu, &callee) == PR_U77_28BE_CPU_OK && callee == 0);
    CHECK(r32(SS, BP1 - 8u) == sum && r8(SS, BP1 - 9u) == 4u && r32(SS, BP1 - 4u) == q && r16(SS, BP1) == BP0);
    CHECK(cpu.ax == (q & 0xFFFFu) && cpu.dx == (q >> 16) && cpu.cx == (r & 0xFFFFu) && cpu.bx == (r >> 16));
    CHECK(cpu.si == (magnitude & 0xFFFFu) && cpu.di == (magnitude >> 16) && cpu.es == PSEG && cpu.ds == DS && cpu.ss == SS);
    CHECK(cpu.bp == BP0 && cpu.sp == SP0 + 8u && cpu.cs == RET_CS && cpu.ip == RET_IP && cpu.flags == divide_flags(FLAGS0, a, b, q));
    for (unsigned off = SP0 - 0x30u; off < SP0 + 8u; ++off) want[at(SS, off)] = ram[at(SS, off)];
    CHECK(memcmp(ram, want, RAM) == 0);
}

static uint32_t value(void)
{
    uint32_t x = next();
    switch (x & 3u) {
    case 0u: return x >> 20;
    case 1u: return 0u - (x >> 22);
    case 2u: return x >> (x & 15u);
    default: return x;
    }
}

static void sums(void)
{
    unsigned v[5];
    uint32_t w[5], divisor;
    for (unsigned n = 0u; n < 3000u; ++n) {
        for (unsigned k = 0u; k < 5u; ++k) v[k] = (unsigned)(next() >> ((n & 1u) ? 16u : 24u)) & 0xFFFFu;
        for (unsigned k = 0u; k < 5u; ++k) w[k] = value();
        do divisor = value(); while (divisor == 0u);
        run_case(v, w, divisor);
    }
    for (unsigned k = 0u; k < 5u; ++k) { v[k] = 0xFFFFu; w[k] = 0x7FFFFFFFu; }
    run_case(v, w, 100u);                                                  /* products and the sum wrap */
    for (unsigned k = 0u; k < 5u; ++k) { v[k] = 0u; w[k] = 0u; }
    run_case(v, w, 0xFFFFFFFFu);                                           /* (0 + 100) div -1 */
}

static void stops(void)
{
    unsigned v[5] = {1u, 2u, 3u, 4u, 5u};
    uint32_t w[5] = {1u, 10u, 100u, 0u, 1000u};
    pr_u77_28be_cpu cpu;
    int callee = 99;
    cpu = world(v, w, 0u, SP0);                                            /* divisor 0: the divide's runtime-error stop */
    CHECK(pr_u77_weighted_28be_cpu(ram, RAM, &cpu, &callee) == PR_U77_28BE_CPU_CALLEE && callee == 2);
    CHECK(cpu.ip == 0x00E2u && cpu.cs == SYS && cpu.ax == 0x00C8u && r32(SS, BP1 - 8u) == 1u + 20u + 300u + 0u + 5000u);
    cpu = world(v, w, 200u, SP0);                                          /* CODE at the multiply site at j = 0: the second SHL's flags, */
                                                                           /* AF set (pinned SHL, since #1433): ZF PF AF */
    w16(CS, 0x28FEu, 0x0A5Cu);
    CHECK(pr_u77_weighted_28be_cpu(ram, RAM, &cpu, &callee) == PR_U77_28BE_CPU_CODE && callee == -1 && cpu.ip == 0x28FDu);
    CHECK((cpu.flags & 0x08D5u) == 0x0054u && cpu.di == 0u && cpu.ax == 1u && cpu.dx == 0u && cpu.cx == 1u && cpu.bx == 0u && cpu.es == PSEG);
    cpu = world(v, w, 200u, SP0);                                          /* CODE at the divide site: ADC DX,0's flags */
    w8(CS, 0x2922u, 0xCBu);
    CHECK(pr_u77_weighted_28be_cpu(ram, RAM, &cpu, &callee) == PR_U77_28BE_CPU_CODE && callee == 0 && cpu.ip == 0x2922u);
    CHECK(cpu.ax == 1u + 20u + 300u + 800u + 5000u + 100u && cpu.dx == 0u && cpu.cx == 200u && cpu.bx == 0u);
    CHECK((cpu.flags & 0x08D5u) == 0x0044u);
    cpu = world(v, w, 200u, 0x000Eu);                                      /* the multiply's return frame at SS:FFFE reads CS past the segment */
    CHECK(pr_u77_weighted_28be_cpu(ram, RAM, &cpu, &callee) == PR_U77_28BE_CPU_TRANSFER && callee == 0);
    CHECK(cpu.ip == 0x2902u && cpu.cs == 0x5C5Cu && cpu.sp == 0x0002u);
    cpu = world(v, w, 200u, SP0);                                          /* entered at another offset: the sites move with it */
    memmove(&ram[at(CS, 0x38BEu)], &ram[at(CS, 0x28BEu)], 0x80u);
    cpu.ip = 0x38BEu;
    CHECK(pr_u77_weighted_28be_cpu(ram, RAM, &cpu, &callee) == PR_U77_28BE_CPU_OK && cpu.ax == (6121u + 100u) / 200u && cpu.cx == (6121u + 100u) % 200u);
}

static void arguments(void)
{
    unsigned v[5] = {0};
    uint32_t w[5] = {0};
    pr_u77_28be_cpu cpu, before;
    int callee = 99;
    cpu = world(v, w, 3u, SP0);
    before = cpu;
    memcpy(want, ram, RAM);
    CHECK(pr_u77_weighted_28be_cpu(NULL, RAM, &cpu, &callee) == PR_U77_28BE_CPU_ARGUMENT && callee == -1);
    CHECK(pr_u77_weighted_28be_cpu(ram, RAM, NULL, NULL) == PR_U77_28BE_CPU_ARGUMENT);
    CHECK(pr_u77_weighted_28be_cpu(ram, RAM - 1u, &cpu, NULL) == PR_U77_28BE_CPU_ARGUMENT);
    CHECK(pr_u77_weighted_28be_cpu(ram, RAM + 1u, &cpu, NULL) == PR_U77_28BE_CPU_ARGUMENT);
    CHECK(memcmp(&cpu, &before, sizeof cpu) == 0 && memcmp(ram, want, RAM) == 0);
}

int main(void)
{
    ram = malloc(RAM);
    want = malloc(RAM);
    CHECK(ram != NULL && want != NULL);
    sums();
    stops();
    arguments();
    free(ram);
    free(want);
    printf("u77 +28be cpu: %u checks passed\n", checks);
    return 0;
}
