/* Focused synthetic test for raw-CPU U1C +0AED (ECL COMPARE AND) composing U25 +0000/+0136 through the overlay stub
 * and SYSTEM FillChar (docs/re/U1C_COMPARE_AND_0AED_CPU.md). No game bytes: only the three five-byte far-call sites,
 * the two loaded-stub EA jumps and four synthetic tag-0 ECL operands. An independent reference predicts the result
 * flags at DS:6F78/6F79, the stored values, the loop byte and the final CMP flags. */
#include "poolrad/recovered/overlay_u1c_compare_and_0aed_cpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM PR_U1C_AND_0AED_RAM_SIZE
#define CHECK(x)                                                                  \
    do {                                                                          \
        if (!(x)) {                                                               \
            fprintf(stderr, "u1c 0aed line %d: %s\n", __LINE__, #x);              \
            exit(1);                                                              \
        }                                                                         \
        ++checks;                                                                 \
    } while (0)

enum { CS = 0x8800, U25 = 0x7800, STUB = 0x002B, SYS = 0x0AF8, DS = 0x1800, SS = 0x2800, SP0 = 0x8000, BP1 = SP0 - 2, BP0 = 0x7777,
       RET_IP = 0x7000, RET_CS = 0x9001, FLAGS0 = 0x0602 };

static uint8_t *ram;
static unsigned checks;

static size_t at(unsigned seg, unsigned off) { return (((size_t)seg << 4) + (off & 0xFFFFu)) & (RAM - 1u); }
static void w8(unsigned seg, unsigned off, unsigned v) { ram[at(seg, off)] = (uint8_t)v; }
static uint8_t r8(unsigned seg, unsigned off) { return ram[at(seg, off)]; }
static void w16(unsigned seg, unsigned off, unsigned v) { w8(seg, off, v & 0xFFu); w8(seg, off + 1u, v >> 8); }
static unsigned r16(unsigned seg, unsigned off) { return r8(seg, off) | (unsigned)r8(seg, off + 1u) << 8; }

static void site(unsigned off, unsigned target, unsigned seg) { w8(CS, off, 0x9Au); w16(CS, off + 1u, target); w16(CS, off + 3u, seg); }

/* `value_stub` is the segment of the value call's stub (STUB normally); `sp` the entry SP. */
static pr_u1c_and_0aed_cpu world_at(const unsigned *v, unsigned base, unsigned value_stub, unsigned sp)
{
    pr_u1c_and_0aed_cpu cpu = {0x1234u, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu, BP0, (uint16_t)sp, CS, (uint16_t)(base + 0x0AEDu),
                               DS, 0x4444u, SS, FLAGS0};
    memset(ram, 0x5C, RAM);
    site(base + 0x0AFFu, 0x16A8u, SYS); site(base + 0x0B07u, 0x002Au, STUB); site(base + 0x0B19u, 0x0025u, value_stub);
    w8(value_stub, 0x0025u, 0xEAu); w16(value_stub, 0x0026u, 0x0136u); w16(value_stub, 0x0028u, U25);
    w8(STUB, 0x002Au, 0xEAu); w16(STUB, 0x002Bu, 0x0000u); w16(STUB, 0x002Du, U25);
    w16(DS, 0x49DEu, 0x1000u); w16(DS, 0x49E0u, 0x6300u); w16(DS, 0x49EDu, 0x9900u);       /* four tag-0 operands */
    for (unsigned k = 0u; k < 4u; ++k) { w8(0x6300u, 0x1001u + 2u * k, 0u); w8(0x6300u, 0x1002u + 2u * k, v[k]); }
    w16(SS, sp, RET_IP); w16(SS, sp + 2u, RET_CS);
    return cpu;
}

static pr_u1c_and_0aed_cpu world(const unsigned *v, unsigned base) { return world_at(v, base, STUB, SP0); }

static unsigned parity_even(unsigned v)
{
    unsigned ones = 0u;
    for (unsigned b = 0u; b < 8u; ++b) ones += (v >> b) & 1u;
    return (ones & 1u) == 0u;
}

static unsigned cmp_flags(unsigned a, unsigned b)
{
    unsigned r = (a - b) & 0xFFFFu, f = 0u;
    if (a < b) f |= 0x0001u;
    if (parity_even(r & 0xFFu)) f |= 0x0004u;
    if ((a ^ b ^ r) & 0x10u) f |= 0x0010u;
    if (r == 0u) f |= 0x0040u;
    if (r & 0x8000u) f |= 0x0080u;
    if ((a ^ b) & (a ^ r) & 0x8000u) f |= 0x0800u;
    return f;
}

static void compare(const unsigned *v)
{
    pr_u1c_and_0aed_cpu cpu = world(v, 0u);
    pr_u1c_and_0aed_limits limits = {SYS, 4u, 400u};
    pr_u1c_and_0aed_callee callee;
    int callee_status = 99, equal = v[0] == v[1] && v[2] == v[3];
    unsigned final = v[0] != v[1] ? cmp_flags(v[0], v[1]) : cmp_flags(v[2], v[3]);
    CHECK(pr_u1c_compare_and_0aed_cpu(ram, RAM, &cpu, &limits, &callee, &callee_status) == PR_U1C_AND_0AED_OK);
    CHECK(callee == PR_U1C_AND_0AED_SLOT_VALUE && callee_status == 0);
    CHECK(r8(DS, 0x6F78u) == (unsigned)equal && r8(DS, 0x6F79u) == (unsigned)!equal);
    for (unsigned i = 2u; i < 6u; ++i) CHECK(r8(DS, 0x6F78u + i) == 0u);
    for (unsigned k = 0u; k < 4u; ++k) CHECK(r16(SS, BP1 - 8u + 2u * k) == v[k] && r8(DS, 0x7006u + k) == 0u && r8(DS, 0x7086u + k) == v[k]);
    CHECK(r8(SS, BP1 - 9u) == 4u && r16(DS, 0x49EDu) == 0x9909u && r16(SS, BP1) == BP0);  /* PC += 2 per operand, then 1 on exit */
    CHECK(cpu.ax == (v[0] != v[1] ? v[0] : v[2]) && cpu.dx == v[3] && cpu.di == 8u);
    CHECK(cpu.bp == BP0 && cpu.sp == SP0 + 4u && cpu.cs == RET_CS && cpu.ip == RET_IP && cpu.ds == DS && cpu.ss == SS);
    CHECK(cpu.flags == (0x0202u | final));
}

static void compares(void)
{
    unsigned seed = 0x0AEDu, v[4];
    static const unsigned shapes[][4] = {{1, 1, 2, 2}, {1, 2, 3, 3}, {7, 7, 8, 9}, {0, 0, 0, 0}, {255, 255, 255, 255}, {255, 0, 0, 255},
                                         {0x80, 0x7F, 1, 1}, {9, 9, 0, 1}};
    for (unsigned n = 0u; n < sizeof shapes / sizeof shapes[0]; ++n) compare(shapes[n]);
    for (unsigned n = 0u; n < 400u; ++n) {
        for (unsigned k = 0u; k < 4u; ++k) { seed = seed * 1103515245u + 12345u; v[k] = (seed >> 16) & ((n & 1u) ? 3u : 0xFFu); }
        compare(v);
    }
}

static void stops(void)
{
    unsigned v[4] = {5u, 5u, 6u, 6u};
    pr_u1c_and_0aed_cpu cpu;
    pr_u1c_and_0aed_limits limits = {SYS, 4u, 400u}, tight = {SYS, 2u, 400u};
    pr_u1c_and_0aed_callee callee;
    int callee_status = 99;
    cpu = world(v, 0u);                                                    /* CODE at the FillChar site: SUB SP,0Ah's flags */
    w8(CS, 0x0AFFu, 0x90u);
    cpu.flags = 0x0AD7u;
    CHECK(pr_u1c_compare_and_0aed_cpu(ram, RAM, &cpu, &limits, &callee, &callee_status) == PR_U1C_AND_0AED_CODE);
    CHECK(callee == PR_U1C_AND_0AED_NO_CALLEE && callee_status == -1 && cpu.ip == 0x0AFFu && cpu.sp == BP1 - 0x0Au - 8u);
    CHECK(cpu.flags == 0x0202u && r16(SS, cpu.sp) == 0u && r16(SS, cpu.sp + 2u) == 6u);   /* 7FFE - 0A = 7FF4: none set */
    cpu = world(v, 0u);                                                    /* the load stub is not EA 0000: CODE before the LCALL */
    w16(STUB, 0x002Bu, 0x0001u);
    CHECK(pr_u1c_compare_and_0aed_cpu(ram, RAM, &cpu, &limits, &callee, &callee_status) == PR_U1C_AND_0AED_CODE);
    CHECK(callee == PR_U1C_AND_0AED_FILLCHAR && cpu.ip == 0x0B07u && r16(SS, cpu.sp) == 4u && (cpu.flags & 0x0400u) == 0u);
    cpu = world(v, 0u);                                                    /* the value stub moved: CODE on the first pass */
    w16(STUB, 0x0026u, 0x0137u);
    CHECK(pr_u1c_compare_and_0aed_cpu(ram, RAM, &cpu, &limits, &callee, &callee_status) == PR_U1C_AND_0AED_CODE);
    CHECK(callee == PR_U1C_AND_0AED_LOAD_OPERANDS && cpu.ip == 0x0B19u && r16(SS, cpu.sp) == (unsigned)((cpu.ax & 0xFF00u) | 1u));
    CHECK(r8(SS, BP1 - 9u) == 1u);
    /* The value stub over this frame: stub segment 2FF0h puts EA at SS:BP-7 (BP = 7F2Ch), so the first pass's store of
     * v[1] at BP-8 clears it and the second pass stops at the site with INC BYTE [BP-9]'s flags (1 -> 2, CF from CMP 1,4). */
    cpu = world_at(v, 0u, 0x2FF0u, 0x7F2Eu);
    CHECK(pr_u1c_compare_and_0aed_cpu(ram, RAM, &cpu, &limits, &callee, &callee_status) == PR_U1C_AND_0AED_CODE);
    CHECK(callee == PR_U1C_AND_0AED_SLOT_VALUE && callee_status == 0 && cpu.ip == 0x0B19u && r8(SS, 0x7F2Cu - 9u) == 2u);
    CHECK((cpu.flags & 0x08D5u) == 0x0001u && r16(SS, 0x7F2Cu - 8u) == 5u && r16(SS, cpu.sp) == 2u);
    cpu = world(v, 0u);                                                    /* a budget of two operands: U25 +0000 stops */
    CHECK(pr_u1c_compare_and_0aed_cpu(ram, RAM, &cpu, &tight, &callee, &callee_status) == PR_U1C_AND_0AED_CALLEE);
    CHECK(callee == PR_U1C_AND_0AED_LOAD_OPERANDS && callee_status == 2);
    cpu = world(v, 0x1000u);                                               /* entered at another offset: the sites move with it */
    CHECK(pr_u1c_compare_and_0aed_cpu(ram, RAM, &cpu, &limits, &callee, &callee_status) == PR_U1C_AND_0AED_OK && r8(DS, 0x6F78u) == 1u);
}

static void arguments(void)
{
    unsigned v[4] = {0u, 0u, 0u, 0u};
    pr_u1c_and_0aed_cpu cpu = world(v, 0u), before = cpu;
    pr_u1c_and_0aed_limits limits = {SYS, 4u, 400u};
    pr_u1c_and_0aed_callee callee = PR_U1C_AND_0AED_FILLCHAR;
    int callee_status = 99;
    uint8_t *copy = malloc(RAM);
    CHECK(copy != NULL);
    memcpy(copy, ram, RAM);
    CHECK(pr_u1c_compare_and_0aed_cpu(NULL, RAM, &cpu, &limits, &callee, &callee_status) == PR_U1C_AND_0AED_ARGUMENT);
    CHECK(callee == PR_U1C_AND_0AED_NO_CALLEE && callee_status == -1);
    CHECK(pr_u1c_compare_and_0aed_cpu(ram, RAM, NULL, &limits, NULL, NULL) == PR_U1C_AND_0AED_ARGUMENT);
    CHECK(pr_u1c_compare_and_0aed_cpu(ram, RAM, &cpu, NULL, NULL, NULL) == PR_U1C_AND_0AED_ARGUMENT);
    CHECK(pr_u1c_compare_and_0aed_cpu(ram, RAM - 1u, &cpu, &limits, NULL, NULL) == PR_U1C_AND_0AED_ARGUMENT);
    CHECK(memcmp(&cpu, &before, sizeof cpu) == 0 && memcmp(ram, copy, RAM) == 0);
    free(copy);
}

int main(void)
{
    ram = malloc(RAM);
    CHECK(ram != NULL);
    compares();
    stops();
    arguments();
    free(ram);
    printf("u1c +0aed cpu: %u checks passed\n", checks);
    return 0;
}
