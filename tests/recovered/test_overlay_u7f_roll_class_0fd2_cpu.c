/* Focused synthetic test for raw-CPU U7F +0FD2 composing the U9E entry-8 dice chain through the loaded stub 00B0:0048
 * (docs/re/U7F_ROLL_CLASS_0FD2_CPU.md). No game bytes: only the five-byte far-call site and the loaded-stub EA jump; the
 * chain model runs on synthetic RAM. Each roll is checked against the class rule, the frame bytes and the last CMP's
 * flags; the chain's own arithmetic is verified by its merged tests. */
#include "poolrad/recovered/overlay_u7f_roll_class_0fd2_cpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM PR_U7F_0FD2_RAM_SIZE
#define CHECK(x)                                                                  \
    do {                                                                          \
        if (!(x)) {                                                               \
            fprintf(stderr, "u7f 0fd2 line %d: %s\n", __LINE__, #x);              \
            exit(1);                                                              \
        }                                                                         \
        ++checks;                                                                 \
    } while (0)

enum { CS = 0x8800, DICE = 0x4800, DS = 0x1800, SS = 0x2800, SP0 = 0x8000, BP1 = SP0 - 2, BP0 = 0x7777, RET_IP = 0x7000, RET_CS = 0x9001,
       FLAGS0 = 0x0602 };

static uint8_t *ram;
static unsigned checks;

static size_t at(unsigned seg, unsigned off) { return (((size_t)seg << 4) + (off & 0xFFFFu)) & (RAM - 1u); }
static void w8(unsigned seg, unsigned off, unsigned v) { ram[at(seg, off)] = (uint8_t)v; }
static uint8_t r8(unsigned seg, unsigned off) { return ram[at(seg, off)]; }
static void w16(unsigned seg, unsigned off, unsigned v) { w8(seg, off, v & 0xFFu); w8(seg, off + 1u, v >> 8); }
static unsigned r16(unsigned seg, unsigned off) { return r8(seg, off) | (unsigned)r8(seg, off + 1u) << 8; }

static pr_u7f_0fd2_raw_cpu world(unsigned seed, unsigned base)
{
    pr_u7f_0fd2_raw_cpu cpu = {0x1234u, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu, BP0, SP0, CS, (uint16_t)(base + 0x0FD2u), DS, 0x4444u, SS,
                               FLAGS0};
    memset(ram, 0x5C, RAM);
    w8(CS, base + 0x0FDEu, 0x9Au); w16(CS, base + 0x0FDFu, 0x0048u); w16(CS, base + 0x0FE1u, 0x00B0u);
    w8(0x00B0u, 0x0048u, 0xEAu); w16(0x00B0u, 0x0049u, 0x0DE3u); w16(0x00B0u, 0x004Bu, DICE);
    w16(0x0AF8u, 0x1245u, 0x8405u);                                        /* the SYSTEM multiplier word the chain reads */
    w16(DS, 0x4428u, seed & 0xFFFFu); w16(DS, 0x442Au, seed >> 16);
    w16(SS, SP0, RET_IP); w16(SS, SP0 + 2u, RET_CS);
    return cpu;
}

static unsigned parity_even(unsigned v)
{
    unsigned ones = 0u;
    for (unsigned b = 0u; b < 8u; ++b) ones += (v >> b) & 1u;
    return (ones & 1u) == 0u;
}

static unsigned cmp8(unsigned a, unsigned b)
{
    unsigned r = (a - b) & 0xFFu, f = 0u;
    if (a < b) f |= 0x0001u;
    if (parity_even(r)) f |= 0x0004u;
    if ((a ^ b ^ r) & 0x10u) f |= 0x0010u;
    if (r == 0u) f |= 0x0040u;
    if (r & 0x80u) f |= 0x0080u;
    if ((a ^ b) & (a ^ r) & 0x80u) f |= 0x0800u;
    return f;
}

static void rolls(void)
{
    unsigned seen[4] = {0u, 0u, 0u, 0u};
    for (unsigned n = 1u; n <= 600u; ++n) {
        pr_u7f_0fd2_raw_cpu cpu = world(n * 0x9E3779B9u, 0u);
        int callee = 99;
        unsigned r, cls, last;
        CHECK(pr_u7f_roll_class_0fd2_cpu(ram, RAM, &cpu, 256u, &callee) == PR_U7F_0FD2_RAW_OK && callee == 0);
        r = r8(SS, BP1 - 2u);
        cls = (r >= 1u && r <= 0x0Eu) ? 1u : (r >= 0x0Fu && r <= 0x13u) ? 2u : 3u;
        last = (r >= 1u && r <= 0x0Eu) ? 0x0Eu : r == 0u ? 0x0Fu : 0x13u;
        CHECK(r >= 1u && r <= 20u && r8(SS, BP1 - 1u) == cls && (cpu.ax & 0xFFu) == cls);
        CHECK((cpu.flags & 0x08D5u) == cmp8(r, last) && r16(SS, BP1) == BP0 && r16(SS, BP1 - 4u) == 0x0001u + ((r16(SS, BP1 - 4u)) & 0xFF00u));
        CHECK(r16(SS, BP1 - 8u) == CS && r16(SS, BP1 - 0x0Au) == 0x0FE3u);
        CHECK(cpu.bp == BP0 && cpu.sp == SP0 + 4u && cpu.cs == RET_CS && cpu.ip == RET_IP && cpu.ds == DS && cpu.ss == SS);
        ++seen[cls];
    }
    CHECK(seen[1] != 0u && seen[2] != 0u && seen[3] != 0u);                /* 600 d20s reach every class */
}

static void stops(void)
{
    pr_u7f_0fd2_raw_cpu cpu;
    int callee = 99;
    cpu = world(12345u, 0u);                                               /* the site is not 9A 48 00: CODE with SUB SP,2's flags */
    w8(CS, 0x0FDFu, 0x49u);
    cpu.flags = 0x0AD7u;
    CHECK(pr_u7f_roll_class_0fd2_cpu(ram, RAM, &cpu, 256u, &callee) == PR_U7F_0FD2_RAW_CODE && callee == -1 && cpu.ip == 0x0FDEu);
    CHECK(cpu.flags == 0x0206u && cpu.sp == BP1 - 2u - 4u && r16(SS, cpu.sp) == 0x0014u + (cpu.ax & 0xFF00u)); /* 7FFE - 2 = 7FFC: PF */
    cpu = world(12345u, 0u);                                               /* the stub does not jump to +0DE3 */
    w16(0x00B0u, 0x0049u, 0x0DE4u);
    CHECK(pr_u7f_roll_class_0fd2_cpu(ram, RAM, &cpu, 256u, &callee) == PR_U7F_0FD2_RAW_CODE && cpu.ip == 0x0FDEu);
    cpu = world(12345u, 0u);                                               /* no SYSTEM call allowed: the chain stops with LIMIT */
    CHECK(pr_u7f_roll_class_0fd2_cpu(ram, RAM, &cpu, 0u, &callee) == PR_U7F_0FD2_RAW_CALLEE && callee != 0 && callee != -1);
    cpu = world(12345u, 0x1000u);                                          /* entered at another offset: the site moves with it */
    CHECK(pr_u7f_roll_class_0fd2_cpu(ram, RAM, &cpu, 256u, &callee) == PR_U7F_0FD2_RAW_OK && cpu.ip == RET_IP);
}

static void arguments(void)
{
    pr_u7f_0fd2_raw_cpu cpu = world(1u, 0u), before = cpu;
    int callee = 99;
    CHECK(pr_u7f_roll_class_0fd2_cpu(NULL, RAM, &cpu, 256u, &callee) == PR_U7F_0FD2_RAW_ARGUMENT && callee == -1);
    CHECK(pr_u7f_roll_class_0fd2_cpu(ram, RAM, NULL, 256u, NULL) == PR_U7F_0FD2_RAW_ARGUMENT);
    CHECK(pr_u7f_roll_class_0fd2_cpu(ram, RAM - 1u, &cpu, 256u, NULL) == PR_U7F_0FD2_RAW_ARGUMENT);
    CHECK(memcmp(&cpu, &before, sizeof cpu) == 0);
}

int main(void)
{
    ram = malloc(RAM);
    CHECK(ram != NULL);
    rolls();
    stops();
    arguments();
    free(ram);
    printf("u7f +0fd2 cpu: %u checks passed\n", checks);
    return 0;
}
