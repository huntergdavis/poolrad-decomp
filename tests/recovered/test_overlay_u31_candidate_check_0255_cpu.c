#include "loaded_stub_fixture.h"
/* Focused synthetic test for raw-CPU U31 +0255, composing U3C2 entry 6 through the loaded stub 03DB:003E and the
 * resident zero test 00BA:23B7 (docs/re/U31_CANDIDATE_CHECK_0255_CPU.md). No game bytes: only the two LCALL encodings,
 * the stub, U3C2's three SYSTEM Move call operands and its synthetic world (as in its merged test) are placed in RAM.
 * With a cold stub the routine stops right after pushing U3C2's frame, predicted by hand; the warm run must equal that
 * state carried through the merged pr_u3c2_caller_0909 and then an independent reference of the candidate loop. */
#include "poolrad/recovered/overlay_u31_candidate_check_0255_cpu.h"
#include "poolrad/recovered/overlay_u3c2_caller_0909.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM PR_U31_0255_RAM_SIZE
#define CHECK(x)                                                                  \
    do {                                                                          \
        if (!(x)) {                                                               \
            fprintf(stderr, "u31 0255 line %d: %s\n", __LINE__, #x);              \
            exit(1);                                                              \
        }                                                                         \
        ++checks;                                                                 \
    } while (0)

enum { CS = 0x8800, U3C2 = 0x3000, PAIR = 0x4000, STUB = 0x03DB, RES = 0x00BA, SYS = 0x0AF8, DS = 0x5000, SS = 0x2000,
       REC = 0x6000, MAP = 0x8000, ACTIVE = 0x1800, SP0 = 0xF000, BP1 = SP0 - 2, RET_IP = 0x7000, RET_CS = 0x9001,
       BP0 = 0x7777, FLAGS0 = 0x0ED7, ROW = 0x21, CALLER_BUDGET = 20000, LINE_BUDGET = 512, SORT_BUDGET = 65536 };

typedef struct { unsigned x, y, type; } member;

static uint8_t *ram, *want;
static unsigned checks;

static size_t at(unsigned seg, unsigned off) { return (((size_t)seg << 4) + (off & 0xFFFFu)) & (RAM - 1u); }
static void w8(uint8_t *m, unsigned seg, unsigned off, unsigned v) { m[at(seg, off)] = (uint8_t)v; }
static uint8_t r8(const uint8_t *m, unsigned seg, unsigned off) { return m[at(seg, off)]; }
static void w16(uint8_t *m, unsigned seg, unsigned off, unsigned v) { w8(m, seg, off, v & 0xFFu); w8(m, seg, off + 1u, v >> 8); }
static unsigned r16(const uint8_t *m, unsigned seg, unsigned off) { return r8(m, seg, off) | (unsigned)r8(m, seg, off + 1u) << 8; }

static void stub(int warm)
{
    if (warm) {
        w8(ram, STUB, 0x3Eu, 0xEAu); w16(ram, STUB, 0x3Fu, 0x0909u); w16(ram, STUB, 0x41u, U3C2);
    } else {
        w8(ram, STUB, 0x3Eu, 0xCDu); w8(ram, STUB, 0x3Fu, 0x3Fu); w16(ram, STUB, 0x40u, 0x0909u); w8(ram, STUB, 0x42u, 0u);
    }
}

/* The world: the two LCALLs for an entry at `entry`, the stub, U3C2's synthetic world and Move operands, the radius
 * row, the map pointer, eleven candidate records with bytes +10Eh from `fields`, the active record and the frame. */
static pr_u31_0255_cpu world_at(unsigned entry, const member *party, unsigned count, unsigned x, unsigned y, unsigned radius,
                                const uint8_t *fields, unsigned active, int warm)
{
    static const int shape[8] = {0, 0, 1, 0, 0, 1, -1, 0}, dx[9] = {0, 1, 1, 1, 0, -1, -1, -1, 0}, dy[9] = {-1, -1, 0, 1, 1, 1, 0, -1, 0};
    pr_u31_0255_cpu cpu = {0x1234u, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu, BP0, SP0, CS, (uint16_t)entry, DS, 0x4444u, SS, FLAGS0};
    unsigned i, j, site = entry + 0x0287u - 0x0255u;
    memset(ram, 0xA5, RAM);
    test_loaded_stub(ram,U3C2,0x092Au,0x4100u,0x0025u,PAIR,0u);
    test_loaded_stub(ram,U3C2,0x09D8u,0x4100u,0x0025u,PAIR,0u);
    w8(ram, CS, site, 0x9Au); w16(ram, CS, site + 1u, 0x003Eu); w16(ram, CS, site + 3u, STUB);
    site = entry + 0x02A9u - 0x0255u;
    w8(ram, CS, site, 0x9Au); w16(ram, CS, site + 1u, 0x23B7u); w16(ram, CS, site + 3u, RES);
    stub(warm);
    for (i = 0u; i < 3u; ++i) {
        static const unsigned moves[3] = {0x131u, 0x160u, 0x181u};
        w8(ram, U3C2, moves[i], 0x9Au); w16(ram, U3C2, moves[i] + 1u, 0x0A3Fu); w16(ram, U3C2, moves[i] + 3u, SYS);
    }
    for (i = 0u; i < 256u; ++i)
        for (j = 0u; j < 8u; ++j) w8(ram, DS, 0x2844u + 8u * i + j, (unsigned)shape[j] & 0xFFu);
    for (i = 0u; i < 9u; ++i) {
        w8(ram, DS, 0x2736u + i, (unsigned)dx[i] & 0xFFu); w8(ram, DS, 0x273Fu + i, (unsigned)dy[i] & 0xFFu); w8(ram, DS, 0x25C0u + i, 0u);
    }
    for (i = 0u; i < 64u; ++i) { w8(ram, DS, 0x2745u + 4u * i, 0u); w8(ram, DS, 0x2746u + 4u * i, 0u); }
    for (i = 0u; i < 0x1800u; ++i) w8(ram, DS, MAP + i, 0u);
    w8(ram, DS, MAP + 6u, 1u);
    w8(ram, DS, 0x5F2Au, count);
    for (i = 1u; i <= count; ++i) {
        w8(ram, DS, 0x5F27u + 4u * i, party[i - 1u].x); w8(ram, DS, 0x5F28u + 4u * i, party[i - 1u].y); w8(ram, DS, 0x5F2Au + 4u * i, party[i - 1u].type);
    }
    w8(ram, DS, 0x3209u + 16u * ROW, radius);
    w16(ram, DS, 0x6716u, MAP); w16(ram, DS, 0x6718u, DS);
    for (i = 0u; i < 11u; ++i) {
        w16(ram, DS, 0x65B9u + 4u * i, 0x0200u * i + i); w16(ram, DS, 0x65BBu + 4u * i, REC);
        w8(ram, REC, 0x0200u * i + i + 0x10Eu, fields[i]);
    }
    w16(ram, DS, 0x5D92u, ACTIVE); w16(ram, DS, 0x5D94u, REC); w8(ram, REC, ACTIVE + 0x10Eu, active);
    w16(ram, SS, SP0, RET_IP); w16(ram, SS, SP0 + 2u, RET_CS); w16(ram, SS, SP0 + 4u, 0x1357u);
    w16(ram, SS, SP0 + 6u, 0x5A00u | ROW); w16(ram, SS, SP0 + 8u, 0x5B00u | y); w16(ram, SS, SP0 + 10u, 0x5C00u | x);
    return cpu;
}

static pr_u31_0255_cpu world(const member *party, unsigned count, const uint8_t *fields, unsigned active, int warm)
{
    return world_at(0x0255u, party, count, 10u, 10u, 100u, fields, active, warm);
}

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

static void cpu_equal(const pr_u31_0255_cpu *a, const pr_u31_0255_cpu *b)
{
    CHECK(memcmp(a, b, sizeof *a) == 0);
}

/* An independent reference of +028C..+02EA over `want`, starting from U3C2's return, for frames and records that do not
 * alias: count at BP-3; per candidate the active pointer, return frame, saved BP and resident local are pushed or
 * written at fixed frame offsets, DL is the resident result and any differing byte +10Eh sets the flag. */
static pr_u31_0255_cpu reference_tail(pr_u31_0255_cpu c, unsigned entry, unsigned *flag_out)
{
    unsigned bp = c.bp, count = r8(want, DS, 0x671Au), flag = r8(want, SS, bp - 1u), dl = 0u, i;
    w8(want, SS, bp - 3u, count);
    if (count == 0u) {
        c.ax = (uint16_t)((c.ax & 0xFF00u) | flag);
        c.flags = (uint16_t)((c.flags & ~0x08D5u) | cmp_flags(1u, 0u));
    } else {
        for (i = 1u; i <= count; ++i) {
            unsigned aoff = r16(want, DS, 0x5D92u), aseg = r16(want, DS, 0x5D94u), local = r8(want, aseg, aoff + 0x10Eu) == 0u;
            unsigned index = r8(want, DS, 0x6718u + 3u * i), poff = r16(want, DS, 0x65B9u + 4u * index), pseg = r16(want, DS, 0x65BBu + 4u * index);
            w16(want, SS, bp - 6u, aseg); w16(want, SS, bp - 8u, aoff);
            w16(want, SS, bp - 0x0Au, CS); w16(want, SS, bp - 0x0Cu, entry + 0x02AEu - 0x0255u);
            w16(want, SS, bp - 0x0Eu, bp); w8(want, SS, bp - 0x0Fu, local);
            dl = local;
            if (r8(want, pseg, poff + 0x10Eu) != dl) flag = 1u;
            c.di = (uint16_t)poff; c.es = (uint16_t)pseg;
        }
        w8(want, SS, bp - 1u, flag); w8(want, SS, bp - 2u, count);
        c.ax = (uint16_t)flag;
        c.dx = (uint16_t)((c.dx & 0xFF00u) | dl);
        c.si = (uint16_t)count;
        c.flags = (uint16_t)((c.flags & ~0x08D5u) | 0x0044u);
    }
    c.bp = (uint16_t)r16(want, SS, bp);
    c.ip = (uint16_t)r16(want, SS, bp + 2u); c.cs = (uint16_t)r16(want, SS, bp + 4u);
    c.sp = (uint16_t)(bp + 2u + 4u + 8u);
    *flag_out = flag;
    return c;
}

/* Cold stub: the stop after U3C2's frame is pushed, predicted by hand; then the warm run against that state carried
 * through pr_u3c2_caller_0909 and the reference tail. Returns the routine's AL. */
static unsigned scenario(const member *party, unsigned count, const uint8_t *fields, unsigned active, unsigned expect_count)
{
    pr_u31_0255_cpu cold, warm, carried;
    pr_u3c2_0909_cpu u3c2;
    pr_u31_0255_callee callee;
    int status;
    unsigned flag;
    cold = world(party, count, fields, active, 0);
    CHECK(pr_u31_candidate_check_0255_cpu(ram, RAM, &cold, PAIR, CALLER_BUDGET, LINE_BUDGET, SORT_BUDGET, &callee, &status) == PR_U31_0255_CODE);
    CHECK(callee == PR_U31_0255_NO_CALLEE && status == -1);
    {
        pr_u31_0255_cpu expect = {0x0001u, 0x3456u, 0x5604u, 0x789Au, 0x9ABCu, ROW << 4, BP1, BP1 - 0x16u, STUB, 0x003Eu, DS, 0x4444u, SS,
                                  (uint16_t)((FLAGS0 & ~0x08D5u) | 0x0044u)};
        cpu_equal(&cold, &expect);
    }
    CHECK(r8(ram, SS, BP1 - 1u) == 0u && r16(ram, SS, BP1) == BP0);
    CHECK(r16(ram, SS, BP1 - 6u) == 0x120Au && r16(ram, SS, BP1 - 8u) == 0x120Au && r16(ram, SS, BP1 - 0x0Au) == 100u);
    CHECK(r16(ram, SS, BP1 - 0x0Cu) == 0x00FFu && r16(ram, SS, BP1 - 0x0Eu) == 0x0001u && r16(ram, SS, BP1 - 0x10u) == DS);
    CHECK(r16(ram, SS, BP1 - 0x12u) == MAP && r16(ram, SS, BP1 - 0x14u) == CS && r16(ram, SS, BP1 - 0x16u) == 0x028Cu);

    stub(1);                                                              /* the state the warm stub reaches */
    memcpy(want, ram, RAM);
    u3c2.ax = cold.ax; u3c2.bx = cold.bx; u3c2.cx = cold.cx; u3c2.dx = cold.dx; u3c2.si = cold.si; u3c2.di = cold.di;
    u3c2.bp = cold.bp; u3c2.sp = cold.sp; u3c2.cs = U3C2; u3c2.ip = 0x0909u; u3c2.ds = cold.ds; u3c2.es = cold.es;
    u3c2.ss = cold.ss; u3c2.flags = cold.flags;
    CHECK(pr_u3c2_caller_0909(want, RAM, &u3c2, PAIR, CALLER_BUDGET, LINE_BUDGET, SORT_BUDGET) == PR_U3C2_0909_OK);
    CHECK(u3c2.cs == CS && u3c2.ip == 0x028Cu && u3c2.sp == BP1 - 4u && r8(want, DS, 0x671Au) == expect_count);
    carried.ax = u3c2.ax; carried.bx = u3c2.bx; carried.cx = u3c2.cx; carried.dx = u3c2.dx; carried.si = u3c2.si; carried.di = u3c2.di;
    carried.bp = u3c2.bp; carried.sp = u3c2.sp; carried.cs = u3c2.cs; carried.ip = u3c2.ip; carried.ds = u3c2.ds; carried.es = u3c2.es;
    carried.ss = u3c2.ss; carried.flags = u3c2.flags;
    carried = reference_tail(carried, 0x0255u, &flag);
    warm = world(party, count, fields, active, 1);
    CHECK(pr_u31_candidate_check_0255_cpu(ram, RAM, &warm, PAIR, CALLER_BUDGET, LINE_BUDGET, SORT_BUDGET, &callee, &status) == PR_U31_0255_OK);
    CHECK(callee == (expect_count ? PR_U31_0255_RESIDENT : PR_U31_0255_U3C2) && status == 0);
    CHECK(memcmp(ram, want, RAM) == 0);
    cpu_equal(&warm, &carried);
    CHECK(warm.cs == RET_CS && warm.ip == RET_IP && warm.sp == SP0 + 12u && warm.bp == BP0 && (warm.ax & 0xFFu) == flag);
    return flag;
}

static void checks_by_party(void)
{
    static const member near[4] = {{12u, 12u, 1u}, {11u, 9u, 1u}, {8u, 13u, 1u}, {14u, 10u, 1u}};
    static const member ten[10] = {{8u, 9u, 1u}, {9u, 9u, 1u}, {10u, 9u, 1u}, {11u, 9u, 1u}, {12u, 9u, 1u},
                                   {8u, 10u, 1u}, {9u, 10u, 1u}, {10u, 10u, 1u}, {11u, 10u, 1u}, {12u, 10u, 1u}};
    uint8_t ones[11], zeros[11], mixed[11];
    memset(ones, 1, sizeof ones); memset(zeros, 0, sizeof zeros); memset(mixed, 0, sizeof mixed);
    mixed[3] = 1u;
    CHECK(scenario(NULL, 0u, ones, 0u, 0u) == 0u);                         /* no candidate: AL 0 after CMP 1,0 */
    CHECK(scenario(near, 1u, ones, 0u, 1u) == 0u);                         /* active +10Eh zero: DL 1; candidate 1 matches */
    CHECK(scenario(near, 1u, zeros, 0u, 1u) == 1u);                        /* candidate 0 differs from DL 1 */
    CHECK(scenario(near, 4u, zeros, 7u, 4u) == 0u);                        /* active nonzero: DL 0; all candidates 0 */
    CHECK(scenario(near, 4u, mixed, 0x80u, 4u) == 1u);                     /* one candidate 1 differs from DL 0 */
    CHECK(scenario(near, 4u, ones, 0u, 4u) == 0u);
    CHECK(scenario(ten, 10u, mixed, 0u, 10u) == 1u);
    {
        member skipped[2] = {{12u, 12u, 0u}, {11u, 9u, 1u}};               /* a type-0 member is not a candidate */
        CHECK(scenario(skipped, 2u, ones, 0u, 1u) == 0u);
    }
}

static void stops(void)
{
    static const member near[2] = {{12u, 12u, 1u}, {11u, 9u, 1u}};
    uint8_t ones[11];
    pr_u31_0255_cpu cpu;
    pr_u31_0255_callee callee;
    int status;
    memset(ones, 1, sizeof ones);
    cpu = world(near, 2u, ones, 0u, 1);
    w16(ram, CS, 0x0288u, 0x003Fu);
    CHECK(pr_u31_candidate_check_0255_cpu(ram, RAM, &cpu, PAIR, CALLER_BUDGET, LINE_BUDGET, SORT_BUDGET, &callee, &status) == PR_U31_0255_CODE);
    CHECK(callee == PR_U31_0255_NO_CALLEE && cpu.ip == 0x0287u && cpu.cs == CS && cpu.sp == BP1 - 0x12u);
    CHECK(cpu.flags == ((FLAGS0 & ~0x08D5u) | 0x0044u));

    cpu = world(near, 2u, ones, 0u, 1);
    w8(ram, CS, 0x02A9u, 0xEAu);
    CHECK(pr_u31_candidate_check_0255_cpu(ram, RAM, &cpu, PAIR, CALLER_BUDGET, LINE_BUDGET, SORT_BUDGET, &callee, &status) == PR_U31_0255_CODE);
    CHECK(callee == PR_U31_0255_U3C2 && status == 0 && cpu.ip == 0x02A9u && cpu.sp == BP1 - 8u);
    CHECK(r8(ram, SS, BP1 - 3u) == 2u && r8(ram, SS, BP1 - 2u) == 1u && cpu.flags == ((cpu.flags & ~0x08D5u) | cmp_flags(1u, 2u)));

    cpu = world(near, 2u, ones, 0u, 1);                                   /* a jump to another U3C2 entry is not +0909 */
    w16(ram, STUB, 0x3Fu, 0x0422u);
    CHECK(pr_u31_candidate_check_0255_cpu(ram, RAM, &cpu, PAIR, CALLER_BUDGET, LINE_BUDGET, SORT_BUDGET, &callee, &status) == PR_U31_0255_CODE);
    CHECK(cpu.cs == STUB && cpu.ip == 0x003Eu && cpu.sp == BP1 - 0x16u);

    cpu = world(near, 2u, ones, 0u, 1);
    CHECK(pr_u31_candidate_check_0255_cpu(ram, RAM, &cpu, PAIR, 1u, LINE_BUDGET, SORT_BUDGET, &callee, &status) == PR_U31_0255_CALLEE);
    CHECK(callee == PR_U31_0255_U3C2 && status == (int)PR_U3C2_0909_CALLER_LIMIT && cpu.cs == U3C2 && cpu.ip == 0x0918u);
    cpu = world(near, 2u, ones, 0u, 1);
    CHECK(pr_u31_candidate_check_0255_cpu(ram, RAM, &cpu, PAIR, CALLER_BUDGET, 1u, SORT_BUDGET, &callee, &status) == PR_U31_0255_CALLEE);
    CHECK(status == (int)PR_U3C2_0909_LINE_LIMIT && cpu.ip == 0x04DDu);
    {
        member five[5] = {{20u, 20u, 1u}, {19u, 19u, 1u}, {18u, 18u, 1u}, {17u, 17u, 1u}, {16u, 16u, 1u}};
        cpu = world(five, 5u, ones, 0u, 1);
        CHECK(pr_u31_candidate_check_0255_cpu(ram, RAM, &cpu, PAIR, CALLER_BUDGET, LINE_BUDGET, 1u, &callee, &status) == PR_U31_0255_CALLEE);
        CHECK(status == (int)PR_U3C2_0909_SORT_LIMIT);
    }
}

/* U3C2's candidate records land on its own return frame (SS = DS, U3C2 entry SP 671Bh): the composed stop must be
 * U3C2's own return, carried from the cold-stub state. */
static void aliases(void)
{
    static const member near[1] = {{12u, 12u, 1u}};
    uint8_t ones[11];
    pr_u31_0255_cpu cold, warm;
    pr_u3c2_0909_cpu u3c2;
    pr_u31_0255_callee callee;
    int status;
    memset(ones, 1, sizeof ones);
    cold = world(near, 1u, ones, 0u, 0);
    cold.ss = DS; cold.sp = 0x671Bu + 0x18u;
    w16(ram, DS, cold.sp, RET_IP); w16(ram, DS, cold.sp + 2u, RET_CS); w16(ram, DS, cold.sp + 6u, ROW); w16(ram, DS, cold.sp + 8u, 10u);
    w16(ram, DS, cold.sp + 10u, 10u);
    CHECK(pr_u31_candidate_check_0255_cpu(ram, RAM, &cold, PAIR, CALLER_BUDGET, LINE_BUDGET, SORT_BUDGET, NULL, NULL) == PR_U31_0255_CODE);
    stub(1);
    memcpy(want, ram, RAM);
    u3c2.ax = cold.ax; u3c2.bx = cold.bx; u3c2.cx = cold.cx; u3c2.dx = cold.dx; u3c2.si = cold.si; u3c2.di = cold.di;
    u3c2.bp = cold.bp; u3c2.sp = cold.sp; u3c2.cs = U3C2; u3c2.ip = 0x0909u; u3c2.ds = cold.ds; u3c2.es = cold.es;
    u3c2.ss = cold.ss; u3c2.flags = cold.flags;
    CHECK(pr_u3c2_caller_0909(want, RAM, &u3c2, PAIR, CALLER_BUDGET, LINE_BUDGET, SORT_BUDGET) == PR_U3C2_0909_OK);
    CHECK(!(u3c2.cs == CS && u3c2.ip == 0x028Cu));
    warm = world(near, 1u, ones, 0u, 1);
    warm.ss = DS; warm.sp = 0x671Bu + 0x18u;
    w16(ram, DS, warm.sp, RET_IP); w16(ram, DS, warm.sp + 2u, RET_CS); w16(ram, DS, warm.sp + 6u, ROW); w16(ram, DS, warm.sp + 8u, 10u);
    w16(ram, DS, warm.sp + 10u, 10u);
    CHECK(pr_u31_candidate_check_0255_cpu(ram, RAM, &warm, PAIR, CALLER_BUDGET, LINE_BUDGET, SORT_BUDGET, &callee, &status) == PR_U31_0255_TRANSFER);
    CHECK(callee == PR_U31_0255_U3C2 && status == (int)PR_U3C2_0909_OK && memcmp(ram, want, RAM) == 0);
    CHECK(warm.cs == u3c2.cs && warm.ip == u3c2.ip && warm.sp == u3c2.sp && warm.bp == u3c2.bp && warm.flags == u3c2.flags);

    {   /* each candidate pointer aims its byte +10Eh at the resident routine's local, written just before the read */
        uint8_t zeros[11];
        static const member two[2] = {{12u, 12u, 1u}, {11u, 9u, 1u}};
        unsigned k;
        memset(zeros, 0, sizeof zeros);
        warm = world(two, 2u, zeros, 0u, 1);
        warm.ss = DS; warm.sp = 0xE000u;
        w16(ram, DS, 0xE000u, RET_IP); w16(ram, DS, 0xE002u, RET_CS); w16(ram, DS, 0xE006u, ROW); w16(ram, DS, 0xE008u, 10u); w16(ram, DS, 0xE00Au, 10u);
        for (k = 0u; k < 11u; ++k) { w16(ram, DS, 0x65B9u + 4u * k, 0xE000u - 2u - 0x0Fu - 0x10Eu); w16(ram, DS, 0x65BBu + 4u * k, DS); }
        CHECK(pr_u31_candidate_check_0255_cpu(ram, RAM, &warm, PAIR, CALLER_BUDGET, LINE_BUDGET, SORT_BUDGET, NULL, NULL) == PR_U31_0255_OK);
        CHECK((warm.ax & 0xFFu) == 0u && r8(ram, DS, 0x671Au) == 2u);    /* zero candidates would differ; the alias reads DL */
    }
}

static void placement(void)
{
    static const member near[2] = {{12u, 12u, 1u}, {11u, 9u, 1u}};
    uint8_t zeros[11];
    pr_u31_0255_cpu cpu;
    memset(zeros, 0, sizeof zeros);
    cpu = world_at(0x1255u, near, 2u, 10u, 10u, 100u, zeros, 0u, 1);      /* relocated entry */
    CHECK(pr_u31_candidate_check_0255_cpu(ram, RAM, &cpu, PAIR, CALLER_BUDGET, LINE_BUDGET, SORT_BUDGET, NULL, NULL) == PR_U31_0255_OK);
    CHECK(cpu.cs == RET_CS && cpu.ip == RET_IP && (cpu.ax & 0xFFu) == 1u);
    CHECK(r16(ram, SS, BP1 - 0x16u) == 0x128Cu && r16(ram, SS, BP1 - 0x0Cu) == 0x12AEu);
    {
        static const member idle[2] = {{12u, 12u, 0u}, {11u, 9u, 0u}};   /* type-0 members are not candidates */
        cpu = world_at(0x0255u, idle, 2u, 10u, 10u, 100u, zeros, 0u, 1);
        CHECK(pr_u31_candidate_check_0255_cpu(ram, RAM, &cpu, PAIR, CALLER_BUDGET, LINE_BUDGET, SORT_BUDGET, NULL, NULL) == PR_U31_0255_OK);
        CHECK((cpu.ax & 0xFFu) == 0u && r8(ram, DS, 0x671Au) == 0u && (cpu.flags & 0x08D5u) == cmp_flags(1u, 0u));
    }
}

static void arguments(void)
{
    uint8_t ones[11];
    pr_u31_0255_cpu cpu, before;
    pr_u31_0255_callee callee = PR_U31_0255_U3C2;
    int status = 99;
    memset(ones, 1, sizeof ones);
    cpu = world(NULL, 0u, ones, 0u, 1);
    before = cpu;
    memcpy(want, ram, RAM);
    CHECK(pr_u31_candidate_check_0255_cpu(NULL, RAM, &cpu, PAIR, 1u, 1u, 1u, &callee, &status) == PR_U31_0255_ARGUMENT);
    CHECK(callee == PR_U31_0255_NO_CALLEE && status == -1);
    CHECK(pr_u31_candidate_check_0255_cpu(ram, RAM, NULL, PAIR, 1u, 1u, 1u, NULL, NULL) == PR_U31_0255_ARGUMENT);
    CHECK(pr_u31_candidate_check_0255_cpu(ram, RAM - 1u, &cpu, PAIR, 1u, 1u, 1u, NULL, NULL) == PR_U31_0255_ARGUMENT);
    CHECK(pr_u31_candidate_check_0255_cpu(ram, RAM, &cpu, PAIR, 0u, 1u, 1u, NULL, NULL) == PR_U31_0255_ARGUMENT);
    CHECK(pr_u31_candidate_check_0255_cpu(ram, RAM, &cpu, PAIR, 1u, 0u, 1u, NULL, NULL) == PR_U31_0255_ARGUMENT);
    CHECK(pr_u31_candidate_check_0255_cpu(ram, RAM, &cpu, PAIR, 1u, 1u, 0u, NULL, NULL) == PR_U31_0255_ARGUMENT);
    cpu.sp = 0x00A5u;                                                     /* U3C2 entry SP 008Dh */
    CHECK(pr_u31_candidate_check_0255_cpu(ram, RAM, &cpu, PAIR, 1u, 1u, 1u, NULL, NULL) == PR_U31_0255_ARGUMENT);
    cpu.sp = 0x0007u;                                                     /* U3C2 entry SP FFEFh */
    CHECK(pr_u31_candidate_check_0255_cpu(ram, RAM, &cpu, PAIR, 1u, 1u, 1u, NULL, NULL) == PR_U31_0255_ARGUMENT);
    cpu.sp = before.sp;
    CHECK(memcmp(&cpu, &before, sizeof cpu) == 0 && memcmp(ram, want, RAM) == 0);
}

int main(void)
{
    ram = malloc(RAM);
    want = malloc(RAM);
    CHECK(ram != NULL && want != NULL);
    checks_by_party();
    stops();
    aliases();
    placement();
    arguments();
    free(ram);
    free(want);
    printf("u31 +0255 cpu: %u checks passed\n", checks);
    return 0;
}
