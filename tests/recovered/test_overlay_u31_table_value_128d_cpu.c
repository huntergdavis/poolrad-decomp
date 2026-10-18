/* Focused synthetic test for raw-CPU U31 +128D composing SYSTEM Move (docs/re/U31_TABLE_VALUE_128D.md).
 * No game bytes: only the five-byte far-call site and synthetic records and table rows. Every case
 * compares the whole 1 MiB buffer with an expected copy built from a byte-level reference. */
#include "poolrad/recovered/overlay_u31_table_value_128d_cpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM PR_U31_128D_RAM_SIZE
#define CHECK(x)                                                                  \
    do {                                                                          \
        if (!(x)) {                                                               \
            fprintf(stderr, "u31 128d line %d: %s\n", __LINE__, #x);              \
            exit(1);                                                              \
        }                                                                         \
        ++checks;                                                                 \
    } while (0)

enum { CS = 0x3800, SYS = 0x0AF8, DS = 0x1800, SS = 0x2800, SP0 = 0x8000, BP1 = SP0 - 2, BP0 = 0x7777,
       PSEG = 0x5000, POFF = 0x0100, QSEG = 0x6000, QOFF = 0x0200, RSEG = 0x6800, ROFF = 0x0300, TSEG = 0x7000, TOFF = 0x0400,
       RET_IP = 0x7A00, RET_CS = 0x7A01 };

typedef struct {
    uint8_t row[16];                 /* the DS:5582 + 16c table row */
    uint8_t c, p32, p36, p3d, p3e;   /* P fields */
    uint8_t q100, qa0;               /* Q fields */
    uint16_t r0a, r0c;               /* R words: T's offset and segment */
    uint8_t t76;
} fields;

static uint8_t *ram, *want;
static unsigned checks;

static size_t at(unsigned seg, unsigned off) { return (((size_t)seg << 4) + (off & 0xFFFFu)) & (RAM - 1u); }
static void w8(unsigned seg, unsigned off, unsigned v) { ram[at(seg, off)] = (uint8_t)v; }
static void w16(unsigned seg, unsigned off, unsigned v) { w8(seg, off, v & 0xFFu); w8(seg, off + 1u, v >> 8); }
static void e8(unsigned seg, unsigned off, unsigned v) { want[at(seg, off)] = (uint8_t)v; }
static void e16(unsigned seg, unsigned off, unsigned v) { e8(seg, off, v & 0xFFu); e8(seg, off + 1u, v >> 8); }

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

static int s8(unsigned v) { return (int)(signed char)(unsigned char)v; }

static pr_u31_128d_cpu world(unsigned base, const fields *f, unsigned flags)
{
    pr_u31_128d_cpu cpu = {0x1234u, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu, BP0, SP0, CS, (uint16_t)(base + 0x128Du),
                           DS, 0x4444u, SS, (uint16_t)flags};
    unsigned table = 0x5582u + 16u * f->c;
    memset(ram, 0x5C, RAM);
    w8(CS, base + 0x12B1u, 0x9Au); w16(CS, base + 0x12B2u, 0x0A3Fu); w16(CS, base + 0x12B4u, SYS);
    for (unsigned i = 0u; i < 16u; ++i) w8(DS, table + i, f->row[i]);
    w8(PSEG, POFF + 0x2Eu, f->c); w8(PSEG, POFF + 0x32u, f->p32); w8(PSEG, POFF + 0x36u, f->p36);
    w8(PSEG, POFF + 0x3Du, f->p3d); w8(PSEG, POFF + 0x3Eu, f->p3e);
    w8(QSEG, QOFF + 0x100u, f->q100); w8(QSEG, QOFF + 0xA0u, f->qa0);
    w16(QSEG, QOFF + 0x108u, ROFF); w16(QSEG, QOFF + 0x10Au, RSEG);
    w16(RSEG, ROFF + 0x0Au, f->r0a); w16(RSEG, ROFF + 0x0Cu, f->r0c);
    w8(f->r0c, f->r0a + 0x76u, f->t76);
    w16(SS, SP0, RET_IP); w16(SS, SP0 + 2u, RET_CS); w16(SS, SP0 + 4u, POFF); w16(SS, SP0 + 6u, PSEG);
    w16(SS, SP0 + 8u, QOFF); w16(SS, SP0 + 0x0Au, QSEG);
    return cpu;
}

/* Byte-level reference; returns v and fills the expected buffer and final AH/DX/CX. */
static unsigned reference(unsigned base, const fields *f, unsigned *ah, unsigned *cx)
{
    const uint8_t *l = f->row;
    unsigned v = (unsigned)(l[9] * l[10]) & 0xFFu, sum, table = 0x5582u + 16u * f->c;
    *cx = 0u;
    if (s8(f->p32) > 0) { v = (v + (unsigned)(s8(f->p32) * 8)) & 0xFFu; *cx = 3u; }
    if (s8(l[11]) > 0) v = (v + (unsigned)(s8(l[11]) * 2)) & 0xFFu;
    if (f->c == 0x55u && (f->r0a | f->r0c) != 0u && s8(f->t76) > 0) v = 8u;
    if (l[14] & 8u) v = (v + 2u * ((l[5] - 1u) & 0xFFFFu)) & 0xFFu;
    if (l[1] <= 1u) v = (v + 3u) & 0xFFu;
    sum = f->q100 + l[1];
    if ((int)sum > 3) v = 0u;
    if (f->p3e == 0x84u && (f->p3d & 0x0Fu) != f->qa0) v = 0u;
    if (f->p3d == 0x53u) v = 0u;
    if (f->p36 != 0u) v = 0u;
    *ah = sum >> 8;
    e16(SS, BP1, BP0);                                                    /* PUSH BP */
    for (unsigned i = 0u; i < 16u; ++i) e8(SS, BP1 - 0x12u + i, l[i]);    /* the copied row */
    e8(SS, BP1 - 2u, v); e8(SS, BP1 - 1u, v);
    e16(SS, BP1 - 0x14u, DS); e16(SS, BP1 - 0x16u, table);               /* Move's argument and return frame */
    e16(SS, BP1 - 0x18u, SS); e16(SS, BP1 - 0x1Au, BP1 - 0x12u); e16(SS, BP1 - 0x1Cu, 0x10u);
    e16(SS, BP1 - 0x1Eu, CS); e16(SS, BP1 - 0x20u, base + 0x12B6u);
    return v;
}

static unsigned run_case(unsigned base, const fields *f, unsigned flags)
{
    pr_u31_128d_cpu cpu = world(base, f, flags);
    unsigned ah, cx, v;
    int callee = 99;
    memcpy(want, ram, RAM);
    v = reference(base, f, &ah, &cx);
    CHECK(pr_u31_table_value_128d_cpu(ram, RAM, &cpu, &callee) == PR_U31_128D_OK && callee == 0);
    CHECK(memcmp(ram, want, RAM) == 0);
    CHECK(cpu.ax == ((ah << 8) | v) && cpu.dx == f->q100 && cpu.cx == cx && cpu.bx == ((BP1 - 0x20u) & 0xFFFFu));
    CHECK(cpu.si == ((0x5582u + 16u * f->c + 16u) & 0xFFFFu) && cpu.di == POFF && cpu.es == PSEG && cpu.ds == DS);
    CHECK(cpu.bp == BP0 && cpu.sp == SP0 + 12u && cpu.cs == RET_CS && cpu.ip == RET_IP && cpu.ss == SS);
    CHECK(cpu.flags == cmp_flags(flags & ~0x0400u, f->p36, 0u));
    return v;
}

static fields base_fields(void)
{
    fields f;
    memset(&f, 0, sizeof f);
    for (unsigned i = 0u; i < 16u; ++i) f.row[i] = (uint8_t)(0x10u + i);
    f.row[1] = 0; f.row[9] = 3; f.row[10] = 5; f.row[11] = 0; f.row[14] = 0;
    f.c = 0x12; f.q100 = 0; f.r0a = TOFF; f.r0c = TSEG;
    return f;
}

static void directed(void)
{
    fields f = base_fields();
    CHECK(run_case(0u, &f, 0x0002u) == 15u + 3u);                       /* 3*5, then L[1] <= 1 adds 3 */
    f.p32 = 2; CHECK(run_case(0u, &f, 0x0ED7u) == 15u + 16u + 3u);       /* +8 * P[32h] */
    f.p32 = 0xFE; CHECK(run_case(0u, &f, 0x0002u) == 18u);               /* negative: skipped */
    f.p32 = 0; f.row[11] = 4; CHECK(run_case(0u, &f, 0x0002u) == 15u + 8u + 3u);
    f.row[11] = 0x80; CHECK(run_case(0u, &f, 0x0002u) == 18u);
    f.row[11] = 0; f.c = 0x55; f.t76 = 1; CHECK(run_case(0u, &f, 0x0002u) == 8u + 3u);   /* class 55h with T^[76h] > 0 */
    f.t76 = 0; CHECK(run_case(0u, &f, 0x0002u) == 18u);
    f.t76 = 1; f.r0a = 0; f.r0c = 0; CHECK(run_case(0u, &f, 0x0002u) == 18u);            /* nil R^ pointer pair */
    f = base_fields(); f.row[14] = 8; f.row[5] = 4; CHECK(run_case(0u, &f, 0x0002u) == 15u + 6u + 3u);
    f.row[5] = 0; CHECK(run_case(0u, &f, 0x0002u) == ((15u + 0x1FFFEu + 3u) & 0xFFu));  /* L[5]-1 wraps */
    f = base_fields(); f.row[1] = 2; CHECK(run_case(0u, &f, 0x0002u) == 15u);
    f.q100 = 2; CHECK(run_case(0u, &f, 0x0002u) == 0u);                   /* Q[100h] + L[1] > 3 */
    f.q100 = 0xFF; f.row[1] = 0xFF; CHECK(run_case(0u, &f, 0x0002u) == 0u);              /* the word sum's AH = 1 */
    f = base_fields(); f.p3e = 0x84; f.p3d = 0x27; f.qa0 = 7; CHECK(run_case(0u, &f, 0x0002u) == 18u);
    f.qa0 = 6; CHECK(run_case(0u, &f, 0x0002u) == 0u);
    f = base_fields(); f.p3d = 0x53; CHECK(run_case(0u, &f, 0x0002u) == 0u);
    f = base_fields(); f.p36 = 1; CHECK(run_case(0u, &f, 0x0ED7u) == 0u);
    f = base_fields(); CHECK(run_case(0x1000u, &f, 0x0002u) == 18u);      /* another entry offset */
}

static void random_worlds(void)
{
    unsigned seed = 0x128Du;
    for (unsigned n = 0u; n < 3000u; ++n) {
        fields f;
        uint8_t *bytes = (uint8_t *)&f;
        for (unsigned i = 0u; i < sizeof f; ++i) { seed = seed * 1103515245u + 12345u; bytes[i] = (uint8_t)(seed >> 16); }
        f.r0a = (uint16_t)(TOFF + (f.r0a & 0x0FF0u));
        f.r0c = (f.r0c & 3u) ? TSEG : 0u;                                 /* T in its own segment, or a nil pair */
        if (f.r0c == 0u) f.r0a = (f.r0a & 1u) ? TOFF : 0u;
        if (n & 1u) f.c = 0x55;
        if (n % 3u == 0u) f.p3e = 0x84;
        if (n % 5u == 0u) f.p36 = 0;
        if (n % 7u == 0u) f.p3d = 0x53;
        if (n % 4u == 0u) { f.q100 = (uint8_t)(f.q100 & 1u); f.row[1] = (uint8_t)(f.row[1] & 1u); }
        (void)run_case((n & 8u) ? 0x0400u : 0u, &f, (n & 2u) ? 0x0ED7u : 0x0202u);
    }
}

static void code_stop(void)
{
    fields f = base_fields();
    pr_u31_128d_cpu cpu = world(0u, &f, 0x0ED7u);
    unsigned di = (0x5582u + 16u * f.c) & 0xFFFFu, ones = 0u, flags;
    int callee = 99;
    w8(CS, 0x12B2u, 0x40u);
    CHECK(pr_u31_table_value_128d_cpu(ram, RAM, &cpu, &callee) == PR_U31_128D_CODE && callee == -1);
    CHECK(cpu.ip == 0x12B1u && cpu.sp == ((BP1 - 0x1Cu) & 0xFFFFu) && cpu.bp == BP1 && cpu.ax == 0x10u && cpu.di == BP1 - 0x12u);
    CHECK(cpu.cx == 0x5604u && cpu.es == PSEG);
    for (unsigned bit = 0u; bit < 8u; ++bit) ones += (di >> bit) & 1u;
    flags = (0x0ED7u & ~0x08D5u) | ((ones & 1u) ? 0u : 0x0004u);         /* ADD DI,5582h: 0120h + 5582h, no carry */
    if (((0x0120u ^ 0x5582u ^ di) & 0x10u) != 0u) flags |= 0x0010u;
    CHECK(cpu.flags == flags);
}

static void arguments(void)
{
    fields f = base_fields();
    pr_u31_128d_cpu cpu = world(0u, &f, 0x0002u), before = cpu;
    int callee = 99;
    memcpy(want, ram, RAM);
    CHECK(pr_u31_table_value_128d_cpu(NULL, RAM, &cpu, &callee) == PR_U31_128D_ARGUMENT && callee == -1);
    CHECK(pr_u31_table_value_128d_cpu(ram, RAM, NULL, NULL) == PR_U31_128D_ARGUMENT);
    CHECK(pr_u31_table_value_128d_cpu(ram, RAM - 1u, &cpu, NULL) == PR_U31_128D_ARGUMENT);
    CHECK(pr_u31_table_value_128d_cpu(ram, RAM + 1u, &cpu, NULL) == PR_U31_128D_ARGUMENT);
    CHECK(memcmp(&cpu, &before, sizeof cpu) == 0 && memcmp(ram, want, RAM) == 0);
}

int main(void)
{
    ram = malloc(RAM);
    want = malloc(RAM);
    CHECK(ram != NULL && want != NULL);
    directed();
    random_worlds();
    code_stop();
    arguments();
    free(ram);
    free(want);
    printf("u31 +128d: %u checks passed\n", checks);
    return 0;
}
