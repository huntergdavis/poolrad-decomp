/* Focused synthetic test for the raw-CPU SYSTEM 0AF8:0A76 LongInt divide (docs/re/SYSTEM_LONGDIV_0A76.md).
 * No game bytes: the model is register-and-stack only. Quotients and remainders are checked against 64-bit
 * truncating division, and every case compares the whole 1 MiB buffer with an expected copy. */
#include "poolrad/recovered/system_longdiv_0a76_cpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM PR_SYSTEM_LONGDIV_RAM_SIZE
#define CHECK(x)                                                                  \
    do {                                                                          \
        if (!(x)) {                                                               \
            fprintf(stderr, "longdiv line %d: %s\n", __LINE__, #x);               \
            exit(1);                                                              \
        }                                                                         \
        ++checks;                                                                 \
    } while (0)

enum { SS = 0x2800, SP0 = 0x8000, RET_IP = 0x7A00, RET_CS = 0x7A01, BP0 = 0x7777 };

static uint8_t *ram, *want;
static unsigned checks;

static size_t at(unsigned seg, unsigned off) { return (((size_t)seg << 4) + (off & 0xFFFFu)) & (RAM - 1u); }
static void w16(uint8_t *mem, unsigned seg, unsigned off, unsigned v) { mem[at(seg, off)] = (uint8_t)v; mem[at(seg, off + 1u)] = (uint8_t)(v >> 8); }

static unsigned parity_even(unsigned v)
{
    unsigned ones = 0u;
    for (unsigned bit = 0u; bit < 8u; ++bit) ones += (v >> bit) & 1u;
    return (ones & 1u) == 0u;
}

static uint16_t logic(unsigned flags, unsigned value)
{
    flags &= ~0x08D5u;
    if (parity_even(value & 0xFFu)) flags |= 0x0004u;
    if ((value & 0xFFFFu) == 0u) flags |= 0x0040u;
    if (value & 0x8000u) flags |= 0x0080u;
    return (uint16_t)flags;
}

static uint16_t canonical(unsigned flags) { return (uint16_t)((flags & 0x7FD7u) | 0x0002u); }

static pr_system_longdiv_cpu frame(uint32_t dividend, uint32_t divisor, unsigned flags)
{
    pr_system_longdiv_cpu cpu = {(uint16_t)dividend, (uint16_t)(divisor >> 16), (uint16_t)divisor, (uint16_t)(dividend >> 16),
                                 0x9ABCu, 0xBCDEu, BP0, SP0, 0x0AF8u, 0x0A76u, 0x1111u, 0x4444u, SS, (uint16_t)flags};
    memset(ram, 0x5C, RAM);
    w16(ram, SS, SP0, RET_IP); w16(ram, SS, SP0 + 2u, RET_CS);
    return cpu;
}

static void divide(uint32_t dividend, uint32_t divisor, unsigned flags)
{
    pr_system_longdiv_cpu cpu = frame(dividend, divisor, flags);
    int64_t a = (int32_t)dividend, b = (int32_t)divisor, q, r;
    uint32_t magnitude = (int32_t)divisor < 0 ? 0u - divisor : divisor;
    uint16_t f1, f2, final;
    q = a / b;
    r = a - b * q;
    f1 = canonical(logic(flags, divisor >> 16));
    f2 = canonical(logic(f1, dividend >> 16));
    if ((a < 0) != (b < 0)) {                                              /* the quotient negation's ADC DX,0 flags */
        uint32_t uq = (uint32_t)(-q), low = ~uq & 0xFFFFu, high = (~uq >> 16) & 0xFFFFu, carry = low == 0xFFFFu, result = (high + carry) & 0xFFFFu;
        final = (uint16_t)(f1 & ~0x08D5u);
        if (carry && high == 0xFFFFu) final |= 0x0001u;
        if (parity_even(result & 0xFFu)) final |= 0x0004u;
        if ((high ^ result) & 0x10u) final |= 0x0010u;
        if (result == 0u) final |= 0x0040u;
        if (result & 0x8000u) final |= 0x0080u;
        if (~high & result & 0x8000u) final |= 0x0800u;
    } else {
        final = f1;
    }
    memcpy(want, ram, RAM);
    w16(want, SS, SP0 - 2u, BP0); w16(want, SS, SP0 - 4u, f1); w16(want, SS, SP0 - 6u, f2);
    CHECK(pr_system_longdiv_0a76(ram, RAM, &cpu) == PR_SYSTEM_LONGDIV_RETURNED);
    CHECK(memcmp(ram, want, RAM) == 0);
    CHECK(cpu.ax == (uint16_t)(uint32_t)q && cpu.dx == (uint16_t)((uint32_t)q >> 16));
    CHECK(cpu.cx == (uint16_t)(uint32_t)r && cpu.bx == (uint16_t)((uint32_t)r >> 16));
    CHECK(cpu.si == (uint16_t)magnitude && cpu.di == (uint16_t)(magnitude >> 16));
    CHECK(cpu.bp == BP0 && cpu.sp == SP0 + 4u && cpu.cs == RET_CS && cpu.ip == RET_IP && cpu.ds == 0x1111u && cpu.es == 0x4444u);
    CHECK(cpu.flags == final);
}

static void divisions(void)
{
    static const uint32_t edges[] = {0u, 1u, 2u, 3u, 7u, 10u, 0xFFFFu, 0x10000u, 0x12345678u, 0x7FFFFFFFu, 0x80000000u, 0x80000001u,
                                     0xFFFFFFFFu, 0xFFFFFFFEu, 0xFFFFFFF9u, 0xFFFF0000u};
    static const unsigned flags[] = {0x0202u, 0x0ED7u, 0x0002u, 0x7A93u};
    uint32_t seed = 0x0A76u;
    for (unsigned i = 0u; i < 16u; ++i)
        for (unsigned j = 0u; j < 16u; ++j)
            if (edges[j]) divide(edges[i], edges[j], flags[(i + j) & 3u]);
    for (unsigned n = 0u; n < 3000u; ++n) {
        uint32_t a, b;
        seed = seed * 1103515245u + 12345u; a = seed;
        seed = seed * 1103515245u + 12345u; b = seed >> (seed & 31u);
        if (b == 0u) b = 1u;
        if (n & 1u) b = 0u - b;
        divide(a, b, flags[n & 3u]);
    }
    divide(0x80000000u, 0xFFFFFFFFu, 0x0202u);                             /* -2^31 / -1 wraps to -2^31 */
}

static void zero_divisor(void)
{
    pr_system_longdiv_cpu cpu = frame(0x12345678u, 0u, 0x0ED7u);
    memcpy(want, ram, RAM);
    w16(want, SS, SP0 - 2u, BP0);
    CHECK(pr_system_longdiv_0a76(ram, RAM, &cpu) == PR_SYSTEM_LONGDIV_DIVIDE_ERROR);
    CHECK(memcmp(ram, want, RAM) == 0);
    CHECK(cpu.ip == 0x00E2u && cpu.cs == 0x0AF8u && cpu.ax == 0x00C8u && cpu.bp == BP0 && cpu.sp == SP0);
    CHECK(cpu.dx == 0x1234u && cpu.bx == 0u && cpu.cx == 0u && cpu.flags == ((0x0ED7u & ~0x08D5u) | 0x0044u));
}

static void arguments(void)
{
    pr_system_longdiv_cpu cpu = frame(5u, 3u, 0x0202u), before = cpu;
    memcpy(want, ram, RAM);
    CHECK(pr_system_longdiv_0a76(NULL, RAM, &cpu) == PR_SYSTEM_LONGDIV_ARGUMENT);
    CHECK(pr_system_longdiv_0a76(ram, RAM, NULL) == PR_SYSTEM_LONGDIV_ARGUMENT);
    CHECK(pr_system_longdiv_0a76(ram, RAM - 1u, &cpu) == PR_SYSTEM_LONGDIV_ARGUMENT);
    CHECK(pr_system_longdiv_0a76(ram, RAM + 1u, &cpu) == PR_SYSTEM_LONGDIV_ARGUMENT);
    CHECK(memcmp(&cpu, &before, sizeof cpu) == 0 && memcmp(ram, want, RAM) == 0);
}

int main(void)
{
    ram = malloc(RAM);
    want = malloc(RAM);
    CHECK(ram != NULL && want != NULL);
    divisions();
    zero_divisor();
    arguments();
    free(ram);
    free(want);
    printf("system longdiv 0a76: %u checks passed\n", checks);
    return 0;
}
