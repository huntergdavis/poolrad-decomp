/* Focused synthetic test for raw-CPU U3C6 +10A7, composing entry 15 (+0C5F, folded), the merged entry 16 (+0CB9) and
 * entry 5 (+03A0) models and SYSTEM FillChar (docs/re/U3C6_CALLER_10A7_CPU.md). No game bytes: only the FillChar LCALL
 * encodings at +11DF and +03B2 are placed in the code segment; the merged callee models need no code bytes. All shape
 * points are negative, so entry 16 leaves its two outputs at their initial 0 and 17h. */
#include "poolrad/recovered/overlay_u3c6_caller_10a7_cpu.h"
#include "poolrad/recovered/overlay_u3c6_rebuild_03a0.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM PR_U3C6_10A7_RAM_SIZE
#define CHECK(x)                                                                  \
    do {                                                                          \
        if (!(x)) {                                                               \
            fprintf(stderr, "u3c6 10a7 line %d: %s\n", __LINE__, #x);             \
            exit(1);                                                              \
        }                                                                         \
        ++checks;                                                                 \
    } while (0)

enum { CS = 0x3000, SYS = 0x0AF8, DS = 0x5000, SS = 0x2000, REC = 0x6000, MAP_SEG = 0x7000, MAP_OFF = 0x0100, SP0 = 0xF000,
       BP1 = SP0 - 2, RET_IP = 0x7000, RET_CS = 0x9001, BP0 = 0x7777, FLAGS0 = 0x0ED7, KEPT = 0x0602, R_OFF = 0x3000,
       ORIGIN_X = 3, ORIGIN_Y = 4 };

typedef struct { unsigned offset, segment, x, y, b6; } entry;

static uint8_t *ram, *want;
static unsigned checks;

static size_t at(unsigned seg, unsigned off) { return (((size_t)seg << 4) + (off & 0xFFFFu)) & (RAM - 1u); }
static void w8(unsigned seg, unsigned off, unsigned v) { ram[at(seg, off)] = (uint8_t)v; }
static uint8_t r8(unsigned seg, unsigned off) { return ram[at(seg, off)]; }
static void w16(unsigned seg, unsigned off, unsigned v) { w8(seg, off, v & 0xFFu); w8(seg, off + 1u, v >> 8); }
static unsigned r16(unsigned seg, unsigned off) { return r8(seg, off) | (unsigned)r8(seg, off + 1u) << 8; }

static int parity_even(unsigned v)
{
    unsigned ones = 0u;
    for (unsigned b = 0u; b < 8u; ++b) ones += (v >> b) & 1u;
    return (ones & 1u) == 0u;
}

/* CMP left,right on bytes. */
static unsigned cmp_flags(unsigned left, unsigned right)
{
    unsigned r = (left - right) & 0xFFu, f = 0u;
    if (left < right) f |= 0x0001u;
    if (parity_even(r)) f |= 0x0004u;
    if ((left ^ right ^ r) & 0x10u) f |= 0x0010u;
    if (r == 0u) f |= 0x0040u;
    if (r & 0x80u) f |= 0x0080u;
    if ((left ^ right) & (left ^ r) & 0x80u) f |= 0x0800u;
    return f;
}

/* The world: two slots (Q is slot 1 at REC:0200h), negative shape points, the map with origin (3, 4), entries, and the
 * frame F, Y, X, Q. */
static pr_u3c6_10a7_cpu world(unsigned flag49, unsigned F, unsigned x, unsigned y, unsigned q_type, unsigned r13, const entry *list,
                              unsigned count)
{
    pr_u3c6_10a7_cpu cpu = {0x1234u, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu, BP0, SP0, CS, 0x10A7u, DS, 0x4444u, SS, FLAGS0};
    unsigned i, t;
    memset(ram, 0xA5, RAM);
    w8(CS, 0x11DFu, 0x9Au); w16(CS, 0x11E0u, 0x16A8u); w16(CS, 0x11E2u, SYS);
    w8(CS, 0x03B2u, 0x9Au); w16(CS, 0x03B3u, 0x16A8u); w16(CS, 0x03B5u, SYS);
    w8(DS, 0x49F3u, flag49);
    for (i = 0u; i < 256u; ++i) { w16(DS, 0x65B9u + 4u * i, 0x9999u); w16(DS, 0x65BBu + 4u * i, 0x8888u); w8(DS, 0x2744u + 4u * i, 0u); }
    w8(DS, 0x5F2Au, 2u);
    for (i = 1u; i <= 2u; ++i) {
        w16(DS, 0x65B9u + 4u * i, 0x0200u * i); w16(DS, 0x65BBu + 4u * i, REC);
        w8(DS, 0x5F27u + 4u * i, 20u + i); w8(DS, 0x5F28u + 4u * i, 12u); w8(DS, 0x5F2Au + 4u * i, 1u);
    }
    for (t = 0u; t < 256u; ++t)
        for (i = 0u; i < 4u; ++i) { w8(DS, 0x2844u + 8u * t + 2u * i, 0xFFu); w8(DS, 0x2845u + 8u * t + 2u * i, 0u); }
    w8(DS, 0x2736u + 8u, 0u); w8(DS, 0x273Fu + 8u, 0u);
    w16(DS, 0x6716u, MAP_OFF); w16(DS, 0x6718u, MAP_SEG);
    w8(MAP_SEG, MAP_OFF + 2u, ORIGIN_X); w8(MAP_SEG, MAP_OFF + 3u, ORIGIN_Y);
    for (i = 0u; i < 1250u; ++i) { w8(MAP_SEG, MAP_OFF + 7u + i, 2u); w8(DS, 0x60DBu + i, 0u); }
    w8(DS, 0x6715u, count);
    for (i = 1u; i <= count; ++i) {
        unsigned a = 0x66D6u + 7u * i;
        w16(DS, a, list[i - 1u].offset); w16(DS, a + 2u, list[i - 1u].segment);
        w8(DS, a + 4u, list[i - 1u].x); w8(DS, a + 5u, list[i - 1u].y); w8(DS, a + 6u, list[i - 1u].b6);
    }
    w8(REC, 0x0200u + 0x6Cu, q_type); w16(REC, 0x0200u + 0x108u, R_OFF); w16(REC, 0x0200u + 0x10Au, REC); w8(REC, R_OFF + 0x13u, r13);
    w16(SS, SP0, RET_IP); w16(SS, SP0 + 2u, RET_CS); w16(SS, SP0 + 4u, 0x5A00u | F); w16(SS, SP0 + 6u, 0x5B00u | y);
    w16(SS, SP0 + 8u, 0x5C00u | x); w16(SS, SP0 + 10u, 0x0200u); w16(SS, SP0 + 12u, REC);
    return cpu;
}

static pr_u3c6_10a7_status run(pr_u3c6_10a7_cpu *cpu, unsigned cap, pr_u3c6_10a7_callee *callee, int *status)
{
    return pr_u3c6_caller_10a7_cpu(ram, RAM, cpu, cap, 512u, 4096u, callee, status);
}

static void returned(const pr_u3c6_10a7_cpu *cpu)
{
    CHECK(cpu->cs == RET_CS && cpu->ip == RET_IP && cpu->sp == SP0 + 4u + 10u && cpu->bp == BP0 && cpu->ds == DS && cpu->ss == SS);
}

static void paths(void)
{
    static const entry none[1] = {{0u, 0u, 0u, 0u, 0u}};
    pr_u3c6_10a7_cpu cpu;
    pr_u3c6_10a7_callee callee;
    int status;
    size_t i, differ = 0u;

    cpu = world(4u, 1u, 10u, 10u, 0x81u, 0u, none, 0u);                   /* [49F3h] is not 5: AL 1 at once */
    memcpy(want, ram, RAM);
    CHECK(run(&cpu, 64u, &callee, &status) == PR_U3C6_10A7_OK && callee == PR_U3C6_10A7_NO_CALLEE && status == -1);
    returned(&cpu);
    CHECK((cpu.ax & 0xFFu) == 1u && cpu.flags == ((FLAGS0 & ~0x08D5u) | cmp_flags(4u, 5u)) && cpu.di == 0xBCDEu && cpu.es == 0x4444u);
    for (i = 0u; i < RAM; ++i) differ += ram[i] != want[i];
    CHECK(differ == 3u && r16(SS, BP1) == BP0 && r8(SS, BP1 - 1u) == 1u);

    cpu = world(5u, 0u, 10u, 11u, 0x83u, 0u, none, 0u);                   /* success, F = 0: straight to +03A0 */
    CHECK(run(&cpu, 64u, &callee, &status) == PR_U3C6_10A7_OK && callee == PR_U3C6_10A7_ENTRY5 && status == 0);
    returned(&cpu);
    CHECK((cpu.ax & 0xFFu) == 1u && r8(DS, 0x5F2Bu + 0u) == 10u && r8(DS, 0x5F2Cu) == 11u && r8(DS, 0x5F2Eu) == 3u);
    CHECK(r8(DS, 0x604Au + 1u) == 10u - ORIGIN_X && r8(DS, 0x6092u + 1u) == 11u - ORIGIN_Y);   /* +03A0's relative arrays */
    CHECK(r8(DS, 0x604Au + 2u) == 22u - ORIGIN_X && r8(DS, 0x6092u + 2u) == 12u - ORIGIN_Y);
    CHECK(r8(SS, BP1 - 4u) == 0u && r8(SS, BP1 - 5u) == 0x17u);           /* entry 16's outputs with no valid point */

    cpu = world(5u, 1u, 10u, 11u, 0x80u, 0u, none, 0u);                   /* type 0 (80h & 7Fh) still succeeds; +03A0 skips it */
    CHECK(run(&cpu, 64u, NULL, NULL) == PR_U3C6_10A7_OK && (cpu.ax & 0xFFu) == 1u && r8(DS, 0x5F2Eu) == 0u);
    CHECK(r8(DS, 0x604Au + 1u) == 0xA5u);

    cpu = world(5u, 1u, 10u, 11u, 0x81u, 0u, none, 0u);                   /* rank[17h] = FFh: the slot's type is cleared, AL 0 */
    w8(DS, 0x2744u + 4u * 0x17u, 0xFFu);
    CHECK(run(&cpu, 64u, &callee, &status) == PR_U3C6_10A7_OK && callee == PR_U3C6_10A7_ENTRY16 && status == 0);
    returned(&cpu);
    CHECK((cpu.ax & 0xFFu) == 0u && r8(DS, 0x5F2Eu) == 0u && r8(DS, 0x5F2Bu) == 10u && r8(DS, 0x5F2Cu) == 11u && cpu.di == 4u);
    CHECK(cpu.flags == (KEPT | 0x0010u));                                 /* SHL DI,1 giving 4: PF 0, AF 1 as the pinned core sets it */
    cpu = world(5u, 1u, 10u, 11u, 0x81u, 0u, none, 0u);
    w8(DS, 0x2744u + 4u * 0x17u, 0xFFu);
    w16(DS, 0x65B9u + 4u * 1u, 0x0F00u);                                  /* Q absent: slot 0 is the count byte itself */
    CHECK(run(&cpu, 64u, NULL, NULL) == PR_U3C6_10A7_OK && (cpu.ax & 0xFFu) == 0u && cpu.di == 0u);
    CHECK(cpu.flags == (KEPT | 0x0010u | 0x0040u | 0x0004u) && r8(DS, 0x5F2Au) == 0u && r8(DS, 0x5F27u) == 10u);
}

static void entries(void)
{
    static const entry three[3] = {{0x0200u, REC, 9u, 9u, 7u}, {0x1234u, REC, 10u, 11u, 0x1Fu}, {0x0200u, REC, 5u, 5u, 0x1Fu}};
    static const entry other[1] = {{0x1234u, REC, 30u, 11u, 3u}};
    pr_u3c6_10a7_cpu cpu;
    pr_u3c6_10a7_callee callee;
    int status;
    unsigned i;

    cpu = world(5u, 1u, 10u, 11u, 0x81u, 0u, three, 3u);                  /* loop 1 clears entries 1 and 3, keeping 7 at BP-5; */
    CHECK(run(&cpu, 64u, &callee, &status) == PR_U3C6_10A7_OK);           /* loop 2 sees entry 2 at (10, 11): no map write */
    returned(&cpu);
    for (i = 0u; i < 7u; ++i) CHECK(r8(DS, 0x66DDu + i) == 0u && r8(DS, 0x66EBu + i) == 0u);
    CHECK(r16(DS, 0x66E4u) == 0x1234u && r8(SS, BP1 - 5u) == 7u && r8(SS, BP1 - 6u) == 1u && r8(MAP_SEG, MAP_OFF + 7u + 50u * 11u + 10u) == 2u);

    cpu = world(5u, 1u, 10u, 11u, 0x81u, 0u, other, 1u);                  /* no entry at (10, 11): the map cell takes BP-5 = 17h */
    CHECK(run(&cpu, 64u, NULL, NULL) == PR_U3C6_10A7_OK && (cpu.ax & 0xFFu) == 1u);
    CHECK(r8(SS, BP1 - 6u) == 0u && r8(MAP_SEG, MAP_OFF + 7u + 50u * 11u + 10u) == 0x17u && r16(DS, 0x66DDu) == 0x1234u);

    cpu = world(5u, 1u, 10u, 11u, 0x81u, 1u, other, 1u);                 /* R^[13h] nonzero: no entry work, no map write */
    CHECK(run(&cpu, 64u, NULL, NULL) == PR_U3C6_10A7_OK && r8(MAP_SEG, MAP_OFF + 7u + 50u * 11u + 10u) == 2u);
}

static void stops(void)
{
    static const entry two[2] = {{0x0200u, REC, 9u, 9u, 7u}, {0x0200u, REC, 9u, 9u, 7u}};
    pr_u3c6_10a7_cpu cpu;
    pr_u3c6_10a7_callee callee;
    int status;

    cpu = world(5u, 1u, 10u, 11u, 0x81u, 0u, two, 2u);                    /* cap 0: the first taken JNE +117F stops */
    CHECK(run(&cpu, 0u, &callee, &status) == PR_U3C6_10A7_LIMIT && callee == PR_U3C6_10A7_FILLCHAR && status == 0);
    CHECK(cpu.ip == 0x117Fu && cpu.cs == CS && cpu.sp == BP1 - 8u && r8(SS, BP1 - 3u) == 1u);
    CHECK(cpu.flags == ((cpu.flags & ~0x08D5u) | cmp_flags(1u, 2u)) && (cpu.flags & 0x0400u) == 0u);
    cpu = world(5u, 1u, 10u, 11u, 0x81u, 0u, two, 2u);
    CHECK(run(&cpu, 1u, NULL, NULL) == PR_U3C6_10A7_OK);

    cpu = world(5u, 1u, 10u, 11u, 0x81u, 0u, two, 2u);
    w16(CS, 0x11E0u, 0x16A9u);
    CHECK(run(&cpu, 64u, &callee, &status) == PR_U3C6_10A7_CODE && callee == PR_U3C6_10A7_ENTRY16);
    CHECK(cpu.ip == 0x11DFu && cpu.sp == BP1 - 8u - 8u && r16(SS, BP1 - 0x0Cu) == 0x66DDu && r16(SS, BP1 - 0x0Eu) == 7u);

    cpu = world(5u, 0u, 10u, 11u, 0x81u, 0u, two, 0u);
    CHECK(pr_u3c6_caller_10a7_cpu(ram, RAM, &cpu, 64u, 1u, 4096u, &callee, &status) == PR_U3C6_10A7_CALLEE);
    CHECK(callee == PR_U3C6_10A7_ENTRY5 && status == (int)PR_U3C6_03A0_SLOT_LIMIT);

    cpu = world(5u, 0u, 10u, 11u, 0x81u, 0u, two, 0u);                    /* SS = DS: +03A0's grid FillChar clears its own return */
    cpu.ss = DS; cpu.sp = 0x60DBu + 0x0Eu;
    w16(DS, cpu.sp, RET_IP); w16(DS, cpu.sp + 2u, RET_CS); w16(DS, cpu.sp + 4u, 0u); w16(DS, cpu.sp + 6u, 11u);
    w16(DS, cpu.sp + 8u, 10u); w16(DS, cpu.sp + 10u, 0x0200u); w16(DS, cpu.sp + 12u, REC);
    CHECK(run(&cpu, 64u, &callee, &status) == PR_U3C6_10A7_TRANSFER && callee == PR_U3C6_10A7_ENTRY5 && status == 0);
    CHECK(cpu.cs == 0u && cpu.ip == 0u);
}

static void arguments(void)
{
    static const entry none[1] = {{0u, 0u, 0u, 0u, 0u}};
    pr_u3c6_10a7_cpu cpu, before;
    pr_u3c6_10a7_callee callee = PR_U3C6_10A7_ENTRY5;
    int status = 99;
    cpu = world(5u, 1u, 10u, 11u, 0x81u, 0u, none, 0u);
    before = cpu;
    memcpy(want, ram, RAM);
    CHECK(pr_u3c6_caller_10a7_cpu(NULL, RAM, &cpu, 64u, 1u, 1u, &callee, &status) == PR_U3C6_10A7_ARGUMENT);
    CHECK(callee == PR_U3C6_10A7_NO_CALLEE && status == -1);
    CHECK(pr_u3c6_caller_10a7_cpu(ram, RAM, NULL, 64u, 1u, 1u, NULL, NULL) == PR_U3C6_10A7_ARGUMENT);
    CHECK(pr_u3c6_caller_10a7_cpu(ram, RAM + 1u, &cpu, 64u, 1u, 1u, NULL, NULL) == PR_U3C6_10A7_ARGUMENT);
    CHECK(pr_u3c6_caller_10a7_cpu(ram, RAM, &cpu, 64u, 0u, 1u, NULL, NULL) == PR_U3C6_10A7_ARGUMENT);
    CHECK(pr_u3c6_caller_10a7_cpu(ram, RAM, &cpu, 64u, 1u, 0u, NULL, NULL) == PR_U3C6_10A7_ARGUMENT);
    cpu.ip = 0x10A8u;
    CHECK(pr_u3c6_caller_10a7_cpu(ram, RAM, &cpu, 64u, 1u, 1u, NULL, NULL) == PR_U3C6_10A7_ARGUMENT);
    cpu.ip = before.ip; cpu.sp = 0x00FFu;
    CHECK(pr_u3c6_caller_10a7_cpu(ram, RAM, &cpu, 64u, 1u, 1u, NULL, NULL) == PR_U3C6_10A7_ARGUMENT);
    cpu.sp = 0xFFF1u;
    CHECK(pr_u3c6_caller_10a7_cpu(ram, RAM, &cpu, 64u, 1u, 1u, NULL, NULL) == PR_U3C6_10A7_ARGUMENT);
    cpu.sp = before.sp;
    CHECK(memcmp(&cpu, &before, sizeof cpu) == 0 && memcmp(ram, want, RAM) == 0);
}

int main(void)
{
    ram = malloc(RAM);
    want = malloc(RAM);
    CHECK(ram != NULL && want != NULL);
    paths();
    entries();
    stops();
    arguments();
    free(ram);
    free(want);
    printf("u3c6 +10a7 cpu: %u checks passed\n", checks);
    return 0;
}
