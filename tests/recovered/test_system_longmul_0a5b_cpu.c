/* Focused synthetic test for the raw-CPU SYSTEM 0AF8:0A5B LongInt multiply (docs/re/SYSTEM_LONGMUL_0A5B.md).
 * No game bytes: the model is register-and-stack only. Products are checked against 64-bit multiplication, the
 * partial products and flags against an independent reference, and every case compares the whole 1 MiB buffer. */
#include "poolrad/recovered/system_longmul_0a5b_cpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM PR_SYSTEM_LONGMUL_RAM_SIZE
#define CHECK(x)                                                                  \
    do {                                                                          \
        if (!(x)) {                                                               \
            fprintf(stderr, "longmul line %d: %s\n", __LINE__, #x);               \
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

/* ADD of two words: the entry flags with the six arithmetic flags replaced. */
static uint16_t add_flags(unsigned flags, unsigned a, unsigned b)
{
    unsigned r = (a + b) & 0xFFFFu;
    flags &= ~0x08D5u;
    if (a + b > 0xFFFFu) flags |= 0x0001u;
    if (parity_even(r & 0xFFu)) flags |= 0x0004u;
    if ((a ^ b ^ r) & 0x10u) flags |= 0x0010u;
    if (r == 0u) flags |= 0x0040u;
    if (r & 0x8000u) flags |= 0x0080u;
    if (!((a ^ b) & 0x8000u) && ((a ^ r) & 0x8000u)) flags |= 0x0800u;
    return (uint16_t)flags;
}

static void multiply(uint32_t a, uint32_t b, unsigned flags, unsigned sp)
{
    pr_system_longmul_cpu cpu = {(uint16_t)a, (uint16_t)(b >> 16), (uint16_t)b, (uint16_t)(a >> 16), 0x9ABCu, 0xBCDEu, BP0, (uint16_t)sp,
                                 0x0AF8u, 0x0A5Bu, 0x1111u, 0x4444u, SS, (uint16_t)flags};
    uint64_t wide = (uint64_t)a * b;
    uint32_t low = (a & 0xFFFFu) * (b & 0xFFFFu);
    unsigned cross1 = ((a & 0xFFFFu) * (b >> 16)) & 0xFFFFu, cross2 = ((a >> 16) * (b & 0xFFFFu)) & 0xFFFFu;
    unsigned partial = ((low >> 16) + cross1) & 0xFFFFu;
    memset(ram, 0x5C, RAM);
    w16(ram, SS, sp, RET_IP); w16(ram, SS, sp + 2u, RET_CS);
    memcpy(want, ram, RAM);
    w16(want, SS, sp - 2u, low & 0xFFFFu); w16(want, SS, sp - 4u, low >> 16);
    CHECK(pr_system_longmul_0a5b(ram, RAM, &cpu) == PR_SYSTEM_LONGMUL_RETURNED);
    CHECK(memcmp(ram, want, RAM) == 0);
    CHECK(cpu.ax == (uint16_t)wide && cpu.dx == (uint16_t)(wide >> 16));
    CHECK(cpu.bx == cross1 && cpu.cx == cross2 && cpu.si == (uint16_t)a && cpu.di == (uint16_t)(a >> 16));
    CHECK(cpu.bp == BP0 && cpu.sp == (uint16_t)(sp + 4u) && cpu.cs == RET_CS && cpu.ip == RET_IP && cpu.ds == 0x1111u && cpu.es == 0x4444u);
    CHECK(cpu.ss == SS && cpu.flags == add_flags(flags, partial, cross2));
}

static void products(void)
{
    static const uint32_t edges[] = {0u, 1u, 2u, 3u, 10u, 0xFFFFu, 0x10000u, 0x10001u, 0x12345678u, 0x7FFFFFFFu, 0x80000000u, 0x80000001u,
                                     0xFFFFFFFFu, 0xFFFFFFFEu, 0xFFFF0000u, 0x0000FFFFu * 3u};
    static const unsigned flags[] = {0x0202u, 0x0ED7u, 0x0002u, 0x7A93u};
    uint32_t seed = 0x0A5Bu;
    for (unsigned i = 0u; i < 16u; ++i)
        for (unsigned j = 0u; j < 16u; ++j)
            multiply(edges[i], edges[j], flags[(i + j) & 3u], SP0);
    for (unsigned n = 0u; n < 4000u; ++n) {
        uint32_t a, b;
        seed = seed * 1103515245u + 12345u; a = seed;
        seed = seed * 1103515245u + 12345u; b = seed >> (seed & 15u);
        if (n & 1u) b = 0u - b;
        multiply(a, b, flags[n & 3u], SP0);
    }
    multiply(0x89ABCDEFu, 0x76543210u, 0x0ED7u, 0x0002u);                  /* the pushes wrap below offset 0 */
    multiply(0x89ABCDEFu, 0x76543210u, 0x0202u, 0xFFFAu);
}

static void arguments(void)
{
    pr_system_longmul_cpu cpu = {5u, 0u, 3u, 0u, 0u, 0u, BP0, SP0, 0x0AF8u, 0x0A5Bu, 0u, 0u, SS, 0x0202u}, before = cpu;
    memset(ram, 0x5C, RAM);
    memcpy(want, ram, RAM);
    CHECK(pr_system_longmul_0a5b(NULL, RAM, &cpu) == PR_SYSTEM_LONGMUL_ARGUMENT);
    CHECK(pr_system_longmul_0a5b(ram, RAM, NULL) == PR_SYSTEM_LONGMUL_ARGUMENT);
    CHECK(pr_system_longmul_0a5b(ram, RAM - 1u, &cpu) == PR_SYSTEM_LONGMUL_ARGUMENT);
    CHECK(pr_system_longmul_0a5b(ram, RAM + 1u, &cpu) == PR_SYSTEM_LONGMUL_ARGUMENT);
    CHECK(memcmp(&cpu, &before, sizeof cpu) == 0 && memcmp(ram, want, RAM) == 0);
}

int main(void)
{
    ram = malloc(RAM);
    want = malloc(RAM);
    CHECK(ram != NULL && want != NULL);
    products();
    arguments();
    free(ram);
    free(want);
    printf("system longmul 0a5b: %u checks passed\n", checks);
    return 0;
}
