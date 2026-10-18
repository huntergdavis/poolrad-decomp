/* Focused synthetic test for raw-CPU U6F +1446, the party insertion composing SYSTEM FillChar and set membership and,
 * through the loaded stub 00AC:0025, U9B +0000 (docs/re/U6F_PARTY_INSERT_1446_CPU.md). No game bytes: only the three
 * LCALL encodings, the {0..7} set and the stub are placed in RAM. With a cold stub the routine stops right after pushing
 * the U9B call frame, which is exactly where the merged pr_u6f_party_insert_1446_ss stops: the whole 1 MiB must match it,
 * and every register is predicted by hand. The warm run must then equal that state carried through the merged U9B model
 * and the epilogue. */
#include "poolrad/recovered/overlay_u6f_party_insert_1446_cpu.h"
#include "poolrad/recovered/overlay_u6f_party_insert_1446_ss.h"
#include "poolrad/recovered/overlay_u9b_entry_zero_0000.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM PR_U6F_INSERT_CPU_RAM_SIZE
#define CHECK(x)                                                                  \
    do {                                                                          \
        if (!(x)) {                                                               \
            fprintf(stderr, "u6f 1446 line %d: %s\n", __LINE__, #x);              \
            exit(1);                                                              \
        }                                                                         \
        ++checks;                                                                 \
    } while (0)

enum { CS = 0x8800, SYS = 0x0AF8, STUB = 0x00AC, U9B = 0x4800, DS = 0x1800, SS = 0x2800, SP0 = 0x8000, BP1 = SP0 - 2,
       REC = 0x6000, WSEG = 0x3800, WOFF = 0x0100, COUNTER0 = 0x7FFF, RET_IP = 0x7000, RET_CS = 0x9001, BP0 = 0x7777,
       FLAGS0 = 0x0ED7, KEPT = 0x0202 };

typedef struct { unsigned seg, off, slot; } node;

static uint8_t *ram, *want;
static unsigned checks;

static size_t at(unsigned seg, unsigned off) { return (((size_t)seg << 4) + (off & 0xFFFFu)) & (RAM - 1u); }
static void w8(unsigned seg, unsigned off, unsigned v) { ram[at(seg, off)] = (uint8_t)v; }
static uint8_t r8(unsigned seg, unsigned off) { return ram[at(seg, off)]; }
static void w16(unsigned seg, unsigned off, unsigned v) { w8(seg, off, v & 0xFFu); w8(seg, off + 1u, v >> 8); }
static unsigned r16(unsigned seg, unsigned off) { return r8(seg, off) | (unsigned)r8(seg, off + 1u) << 8; }

static void lcall(unsigned entry, unsigned site, unsigned target, unsigned segment)
{
    unsigned s = entry + site - 0x1446u;
    w8(CS, s, 0x9Au); w16(CS, s + 1u, target); w16(CS, s + 3u, segment);
}

static void stub(int warm)
{
    if (warm) {
        w8(STUB, 0x25u, 0xEAu); w16(STUB, 0x26u, 0x0000u); w16(STUB, 0x28u, U9B);
    } else {
        w8(STUB, 0x25u, 0xCDu); w8(STUB, 0x26u, 0x3Fu); w16(STUB, 0x27u, 0x0000u); w8(STUB, 0x29u, 0x00u);
    }
}

/* The world: LCALLs for an entry at `entry`, the set at CS:1426, the stub, U9B's DS tables, the list from the head, the
 * incoming record P = pseg:poff whose own link is nil, window1 and the entry frame. */
static pr_u6f_insert_cpu world_at(unsigned entry, const node *list, unsigned count, unsigned pseg, unsigned poff, int warm)
{
    pr_u6f_insert_cpu cpu = {0x1234u, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu, BP0, SP0, CS, (uint16_t)entry, DS, 0x4444u, SS, FLAGS0};
    memset(ram, 0x5C, RAM);
    lcall(entry, 0x14CAu, 0x16A8u, SYS); lcall(entry, 0x14F2u, 0x0DC0u, SYS); lcall(entry, 0x1574u, 0x0025u, STUB);
    for (unsigned b = 0u; b < 32u; ++b) w8(CS, 0x1426u + b, b == 0u ? 0xFFu : 0x00u);
    stub(warm);
    for (unsigned a = 0x31FAu; a < 0x4300u; ++a) w8(DS, a, (a * 37u + 11u) & 0xFFu);
    for (unsigned i = 0u; i < count; ++i) {
        w8(list[i].seg, list[i].off + 0xBFu, list[i].slot);
        w16(list[i].seg, list[i].off + 0x104u, i + 1u < count ? list[i + 1u].off : 0u);
        w16(list[i].seg, list[i].off + 0x106u, i + 1u < count ? list[i + 1u].seg : 0u);
    }
    w16(DS, 0x5D96u, count ? list[0].off : 0u); w16(DS, 0x5D98u, count ? list[0].seg : 0u);
    w16(DS, 0x5D92u, 0x1111u); w16(DS, 0x5D94u, 0x2222u);
    w16(DS, 0x49D6u, WOFF); w16(DS, 0x49D8u, WSEG); w16(WSEG, WOFF + 0x67Cu, COUNTER0);
    w16(pseg, poff + 0x104u, 0u); w16(pseg, poff + 0x106u, 0u);
    w16(SS, SP0, RET_IP); w16(SS, SP0 + 2u, RET_CS); w16(SS, SP0 + 4u, poff); w16(SS, SP0 + 6u, pseg);
    return cpu;
}

static pr_u6f_insert_cpu world(const node *list, unsigned count, unsigned poff, int warm)
{
    return world_at(0x1446u, list, count, REC, poff, warm);
}

static int parity_even(unsigned v)
{
    unsigned ones = 0u;
    for (unsigned b = 0u; b < 8u; ++b) ones += (v >> b) & 1u;
    return (ones & 1u) == 0u;
}

/* INC WORD giving r, with CF clear. */
static unsigned inc_flags(unsigned r)
{
    unsigned f = 0u;
    if (parity_even(r & 0xFFu)) f |= 0x0004u;
    if ((r & 0xFu) == 0u) f |= 0x0010u;
    if (r == 0u) f |= 0x0040u;
    if (r & 0x8000u) f |= 0x0080u;
    if (r == 0x8000u) f |= 0x0800u;
    return f;
}

/* OR/XOR result flags (CF, OF, AF clear). */
static unsigned logic_flags(unsigned r, unsigned sign)
{
    return (parity_even(r & 0xFFu) ? 0x0004u : 0u) | (r == 0u ? 0x0040u : 0u) | (r & sign ? 0x0080u : 0u);
}

static void cpu_equal(const pr_u6f_insert_cpu *a, const pr_u6f_insert_cpu *b)
{
    CHECK(memcmp(a, b, sizeof *a) == 0);
}

/* Cold stub: the stop after the U9B call's pushes, against the _ss model and the hand-predicted state; then the warm
 * run against that state carried through U9B +0000 and MOV SP,BP; POP BP; RETF 4. */
static void scenario(const node *list, unsigned count, unsigned poff, unsigned slot)
{
    pr_u6f_insert_cpu cpu, warm, carried;
    pr_u6f_insert_dos_memory memory;
    pr_u6f_insert_ss_frame frame = {CS, SS, SP0, BP0};
    pr_u6f_insert_ss_report report;
    pr_u6f_insert_cpu_callee callee;
    pr_u9b_0000_cpu u9b;
    unsigned marks[8] = {0}, tail = count ? count - 1u : 0u;
    int status;
    for (unsigned i = 0u; i < count; ++i)
        if (list[i].slot < 8u) marks[list[i].slot] = 1u;
    (void)world(list, count, poff, 0);
    memcpy(want, ram, RAM);
    memory.ram = want; memory.ram_size = RAM; memory.ds = DS;
    CHECK(pr_u6f_party_insert_1446_ss(&memory, &frame, &report) == PR_U6F_INSERT_SS_AT_U9B_TRANSFER);
    cpu = world(list, count, poff, 0);
    CHECK(pr_u6f_party_insert_1446_cpu(ram, RAM, &cpu, 64u, &callee, &status) == PR_U6F_INSERT_CPU_CODE);
    CHECK(callee == PR_U6F_INSERT_CPU_SET_IN && status == 0);
    CHECK(memcmp(ram, want, RAM) == 0);
    CHECK(report.selected_slot == slot && report.window_word == ((COUNTER0 + 1u) & 0xFFFFu));
    CHECK(report.transfer_sp == cpu.sp && report.callee_bp == cpu.bp);
    {
        pr_u6f_insert_cpu expect = {(uint16_t)(slot < 7u ? slot : 7u), 0x001Fu, 0x0007u, REC, 0x9ABCu, WOFF, BP1, BP1 - 0x18u,
                                    STUB, 0x0025u, DS, WSEG, SS, (uint16_t)(KEPT | inc_flags((COUNTER0 + 1u) & 0xFFFFu))};
        cpu_equal(&cpu, &expect);
    }
    CHECK(r8(REC, poff + 0xBFu) == slot && r16(WSEG, WOFF + 0x67Cu) == ((COUNTER0 + 1u) & 0xFFFFu));
    CHECK(r16(DS, 0x5D92u) == poff && r16(DS, 0x5D94u) == REC);
    CHECK(r16(DS, 0x5D96u) == (count ? list[0].off : poff) && r16(DS, 0x5D98u) == (count ? list[0].seg : (unsigned)REC));
    if (count)
        CHECK(r16(list[tail].seg, list[tail].off + 0x104u) == poff && r16(list[tail].seg, list[tail].off + 0x106u) == REC);
    for (unsigned s = 0u; s < 8u; ++s)
        CHECK(r8(SS, BP1 - 0x0Cu + s) == marks[s]);
    CHECK(r16(SS, BP1 - 4u) == 0u && r16(SS, BP1 - 2u) == 0u && r16(SS, BP1 - 0x10u) == poff && r16(SS, BP1 - 0x0Eu) == REC);
    CHECK(r16(SS, BP1) == BP0 && r16(SS, BP1 - 0x18u) == 0x1579u && r16(SS, BP1 - 0x16u) == CS);
    CHECK(r16(SS, BP1 - 0x14u) == poff && r16(SS, BP1 - 0x12u) == REC);

    stub(1);                                                              /* the state the warm stub would reach */
    memcpy(want, ram, RAM);
    memcpy(&u9b, &cpu, sizeof u9b);
    u9b.cs = U9B; u9b.ip = 0x0000u;
    CHECK(pr_u9b_entry_zero_0000(want, RAM, &u9b) == PR_U9B_0000_OK && u9b.cs == CS && u9b.ip == 0x1579u);
    memcpy(&carried, &u9b, sizeof carried);
    carried.sp = carried.bp;
    carried.bp = (uint16_t)(want[at(SS, carried.sp)] | want[at(SS, carried.sp + 1u)] << 8);
    carried.ip = (uint16_t)(want[at(SS, carried.sp + 2u)] | want[at(SS, carried.sp + 3u)] << 8);
    carried.cs = (uint16_t)(want[at(SS, carried.sp + 4u)] | want[at(SS, carried.sp + 5u)] << 8);
    carried.sp = (uint16_t)(carried.sp + 10u);
    warm = world(list, count, poff, 1);
    CHECK(pr_u6f_party_insert_1446_cpu(ram, RAM, &warm, 64u, &callee, &status) == PR_U6F_INSERT_CPU_OK);
    CHECK(callee == PR_U6F_INSERT_CPU_U9B && status == (int)PR_U9B_0000_OK);
    CHECK(memcmp(ram, want, RAM) == 0);
    cpu_equal(&warm, &carried);
    CHECK(warm.cs == RET_CS && warm.ip == RET_IP && warm.sp == SP0 + 8u && warm.bp == BP0);
}

static void insertions(void)
{
    static const node three[] = {{REC, 0x0400u, 0u}, {REC, 0x0600u, 1u}, {0x6100u, 0x0010u, 5u}};
    static const node dup[] = {{REC, 0x0400u, 3u}, {0x5F00u, 0x0F00u, 3u}, {REC, 0x0800u, 0u}};
    static const node outside[] = {{REC, 0x0400u, 8u}, {REC, 0x0600u, 0x10u}, {REC, 0x0800u, 0x80u}, {REC, 0x0A00u, 0xFEu}};
    node full[8];
    for (unsigned s = 0u; s < 8u; ++s) {
        full[s].seg = 0x6200u + 0x40u * s; full[s].off = 0x0010u * s; full[s].slot = 7u - s;
    }
    scenario(NULL, 0u, 0x0200u, 0u);
    scenario(three, 3u, 0x0200u, 2u);
    scenario(dup, 3u, 0x0201u, 1u);
    scenario(outside, 4u, 0x0200u, 0u);
    scenario(full, 8u, 0x0200u, 8u);
    scenario(full, 7u, 0x0200u, 0u);
    scenario(full + 1, 7u, 0x0200u, 7u);
}

static void limits(void)
{
    static const node one[] = {{REC, 0x0400u, 0u}};
    static const node three[] = {{REC, 0x0400u, 0u}, {REC, 0x0600u, 1u}, {0x6100u, 0x0010u, 5u}};
    pr_u6f_insert_cpu cpu;
    pr_u6f_insert_cpu_callee callee;
    int status;
    cpu = world(one, 1u, 0x0200u, 1);                                     /* the head links to itself: loop A runs forever */
    w16(REC, 0x0504u, 0x0400u); w16(REC, 0x0506u, REC);
    CHECK(pr_u6f_party_insert_1446_cpu(ram, RAM, &cpu, 5u, &callee, &status) == PR_U6F_INSERT_CPU_LIMIT);
    CHECK(callee == PR_U6F_INSERT_CPU_NO_CALLEE && status == -1);
    {
        pr_u6f_insert_cpu expect = {0x0400u, 0x3456u, 0x5678u, REC, 0x9ABCu, 0x0400u, BP1, BP1 - 0x10u, CS, 0x1487u, DS, REC, SS,
                                    (uint16_t)((FLAGS0 & ~0x08D5u) | logic_flags(0x6400u, 0x8000u))};
        cpu_equal(&cpu, &expect);
    }
    CHECK(r8(REC, 0x02BFu) == 0xFFu && r16(SS, BP1 - 4u) == 0x0400u && r16(SS, BP1 - 2u) == REC && r16(DS, 0x5D92u) == 0x1111u);

    cpu = world(three, 3u, 0x0200u, 1);                                   /* P links back to the head: loop B runs forever */
    w16(REC, 0x0304u, 0x0400u); w16(REC, 0x0306u, REC);
    CHECK(pr_u6f_party_insert_1446_cpu(ram, RAM, &cpu, 6u, &callee, &status) == PR_U6F_INSERT_CPU_LIMIT);
    CHECK(callee == PR_U6F_INSERT_CPU_SET_IN && status == 0);
    CHECK(cpu.ip == 0x14DCu && cpu.sp == BP1 - 0x10u && cpu.flags == (KEPT | logic_flags(0u, 0x80u)));
    CHECK(r16(SS, BP1 - 4u) == 0x0600u && r16(SS, BP1 - 2u) == REC);    /* two loop-backs in A, then the head again */
    CHECK(r8(SS, BP1 - 0x0Cu) == 1u && r8(SS, BP1 - 0x0Bu) == 1u && r8(SS, BP1 - 0x07u) == 1u && r8(SS, BP1 - 0x0Au) == 0u);

    cpu = world(three, 3u, 0x0200u, 1);                                   /* 2 + 4 loop-backs fit a cap of 6 exactly */
    CHECK(pr_u6f_party_insert_1446_cpu(ram, RAM, &cpu, 6u, NULL, NULL) == PR_U6F_INSERT_CPU_OK);
    cpu = world(three, 3u, 0x0200u, 1);
    CHECK(pr_u6f_party_insert_1446_cpu(ram, RAM, &cpu, 5u, NULL, NULL) == PR_U6F_INSERT_CPU_LIMIT && cpu.ip == 0x14DCu);
    cpu = world(NULL, 0u, 0x0200u, 1);                                    /* empty head: one loop-back, after P */
    CHECK(pr_u6f_party_insert_1446_cpu(ram, RAM, &cpu, 0u, NULL, NULL) == PR_U6F_INSERT_CPU_LIMIT && cpu.ip == 0x14DCu);
    CHECK(r16(SS, BP1 - 4u) == 0u && r16(SS, BP1 - 2u) == 0u);
}

/* The record P on the stack at SS:poff (its link nil) and, when `zero`, zero U9B tables. */
static void stack_record(unsigned poff, int zero)
{
    if (zero)
        for (unsigned a = 0x31FAu; a < 0x4300u; ++a) w8(DS, a, 0u);
    w16(SS, SP0 + 4u, poff); w16(SS, SP0 + 6u, SS);
    w16(SS, poff + 0x104u, 0u); w16(SS, poff + 0x106u, 0u);
}

/* With P on the stack U9B can divert itself or its return; the composed stop must be the U9B model's own stop carried
 * from the cold-stub state: the same 1 MiB and all 14 words. */
static pr_u6f_insert_cpu through_u9b(const node *list, unsigned count, unsigned poff, int zero, pr_u6f_insert_cpu_status expect,
                                     int u9b_status)
{
    pr_u6f_insert_cpu cold, warm;
    pr_u9b_0000_cpu u9b;
    pr_u6f_insert_cpu_callee callee;
    int status;
    cold = world(list, count, 0x0200u, 0);
    stack_record(poff, zero);
    CHECK(pr_u6f_party_insert_1446_cpu(ram, RAM, &cold, 64u, NULL, NULL) == PR_U6F_INSERT_CPU_CODE && cold.cs == STUB);
    stub(1);
    memcpy(want, ram, RAM);
    memcpy(&u9b, &cold, sizeof u9b);
    u9b.cs = U9B; u9b.ip = 0x0000u;
    CHECK((int)pr_u9b_entry_zero_0000(want, RAM, &u9b) == u9b_status);
    warm = world(list, count, 0x0200u, 1);
    stack_record(poff, zero);
    CHECK(pr_u6f_party_insert_1446_cpu(ram, RAM, &warm, 64u, &callee, &status) == expect);
    CHECK(callee == PR_U6F_INSERT_CPU_U9B && status == u9b_status);
    CHECK(memcmp(ram, want, RAM) == 0 && memcmp(&warm, &u9b, sizeof warm) == 0);
    return warm;
}

static void stops(void)
{
    static const node three[] = {{REC, 0x0400u, 0u}, {REC, 0x0600u, 1u}, {0x6100u, 0x0010u, 5u}};
    pr_u6f_insert_cpu cpu;
    pr_u6f_insert_cpu_callee callee;
    int status;
    cpu = world(three, 3u, 0x0200u, 1);
    w16(CS, 0x14CBu, 0x16A9u);
    CHECK(pr_u6f_party_insert_1446_cpu(ram, RAM, &cpu, 64u, &callee, &status) == PR_U6F_INSERT_CPU_CODE);
    CHECK(callee == PR_U6F_INSERT_CPU_NO_CALLEE && status == -1);
    CHECK(cpu.ip == 0x14CAu && cpu.cs == CS && cpu.sp == BP1 - 0x18u && cpu.flags == ((FLAGS0 & ~0x08D5u) | 0x0044u));
    CHECK(r16(0x6100u, 0x0114u) == 0x0200u && r16(SS, BP1 - 0x18u) == 0u && r16(SS, BP1 - 0x16u) == 8u);

    cpu = world(three, 3u, 0x0200u, 1);
    w8(CS, 0x14F2u, 0xEAu);
    CHECK(pr_u6f_party_insert_1446_cpu(ram, RAM, &cpu, 64u, &callee, &status) == PR_U6F_INSERT_CPU_CODE);
    CHECK(callee == PR_U6F_INSERT_CPU_FILLCHAR && status == 0);
    CHECK(cpu.ip == 0x14F2u && cpu.sp == BP1 - 0x16u && cpu.flags == (KEPT | logic_flags(0x6400u, 0x8000u)));
    CHECK(cpu.ax == 0x6400u && r16(SS, BP1 - 0x12u) == 0x6400u && r16(SS, BP1 - 0x14u) == CS && r16(SS, BP1 - 0x16u) == 0x1426u);

    cpu = world(three, 3u, 0x0200u, 1);
    w16(CS, 0x1575u, 0x002Au);
    CHECK(pr_u6f_party_insert_1446_cpu(ram, RAM, &cpu, 64u, &callee, &status) == PR_U6F_INSERT_CPU_CODE);
    CHECK(callee == PR_U6F_INSERT_CPU_SET_IN && status == 0);
    CHECK(cpu.ip == 0x1574u && cpu.cs == CS && cpu.sp == BP1 - 0x14u && cpu.flags == (KEPT | inc_flags(0x8000u)));

    cpu = world(three, 3u, 0x0200u, 1);                                   /* an EA to another U9B entry is not +0000 */
    w16(STUB, 0x26u, 0x01AFu);
    CHECK(pr_u6f_party_insert_1446_cpu(ram, RAM, &cpu, 64u, &callee, &status) == PR_U6F_INSERT_CPU_CODE);
    CHECK(cpu.cs == STUB && cpu.ip == 0x0025u && cpu.sp == BP1 - 0x18u && r16(SS, BP1 - 0x18u) == 0x1579u);

    cpu = through_u9b(three, 3u, BP1 - 0x18u - 0x2Du, 0, PR_U6F_INSERT_CPU_TRANSFER, (int)PR_U9B_0000_OK);
    CHECK(!(cpu.cs == CS && cpu.ip == 0x1579u));                          /* U9B clears record +2Dh: its own return IP */
    cpu = through_u9b(three, 3u, BP1 - 0x97u, 1, PR_U6F_INSERT_CPU_CALLEE, (int)PR_U9B_0000_TRANSFER);
    CHECK(cpu.cs != U9B);                                                 /* a leaf's RETF 4 reads a frame it overwrote */

    cpu = world(three, 3u, 0x0200u, 1);                                   /* FillChar's RETF frame straddles SS:FFFFh */
    w16(SS, 0x001Cu, RET_IP); w16(SS, 0x001Eu, RET_CS); w16(SS, 0x0020u, 0x0200u); w16(SS, 0x0022u, REC);
    cpu.sp = 0x001Cu;
    CHECK(pr_u6f_party_insert_1446_cpu(ram, RAM, &cpu, 64u, &callee, &status) == PR_U6F_INSERT_CPU_TRANSFER);
    CHECK(callee == PR_U6F_INSERT_CPU_FILLCHAR && status == 0);
    CHECK(cpu.ip == 0x14CFu && cpu.cs == 0x5C5Cu && cpu.sp == 0x000Au && r16(SS, 0x0000u) == CS);
}

static void placement(void)
{
    static const node three[] = {{REC, 0x0400u, 0u}, {REC, 0x0600u, 12u}, {0x6100u, 0x0010u, 5u}};
    pr_u6f_insert_cpu cpu;
    cpu = world_at(0x1546u, three, 3u, REC, 0x0200u, 1);                  /* relocated entry; the set stays at CS:1426h */
    CHECK(pr_u6f_party_insert_1446_cpu(ram, RAM, &cpu, 64u, NULL, NULL) == PR_U6F_INSERT_CPU_OK);
    CHECK(cpu.cs == RET_CS && cpu.ip == RET_IP && cpu.sp == SP0 + 8u && cpu.bp == BP0 && r16(SS, BP1 - 0x18u) == 0x1679u);
    CHECK(r8(REC, 0x02BFu) == 1u && r16(WSEG, WOFF + 0x67Cu) == 0x8000u);

    cpu = world(three, 3u, 0x0200u, 1);                                   /* the set is read from RAM: slot 12 marks [BP] */
    w8(CS, 0x1427u, 0x10u);
    CHECK(pr_u6f_party_insert_1446_cpu(ram, RAM, &cpu, 64u, NULL, NULL) == PR_U6F_INSERT_CPU_OK);
    CHECK(cpu.bp == ((BP0 & 0xFF00u) | 1u) && cpu.ip == RET_IP && r8(REC, 0x02BFu) == 1u);
}

static void arguments(void)
{
    pr_u6f_insert_cpu cpu, before;
    pr_u6f_insert_cpu_callee callee = PR_U6F_INSERT_CPU_U9B;
    int status = 99;
    cpu = world(NULL, 0u, 0x0200u, 1);
    before = cpu;
    memcpy(want, ram, RAM);
    CHECK(pr_u6f_party_insert_1446_cpu(NULL, RAM, &cpu, 64u, &callee, &status) == PR_U6F_INSERT_CPU_ARGUMENT);
    CHECK(callee == PR_U6F_INSERT_CPU_NO_CALLEE && status == -1);
    CHECK(pr_u6f_party_insert_1446_cpu(ram, RAM, NULL, 64u, NULL, NULL) == PR_U6F_INSERT_CPU_ARGUMENT);
    CHECK(pr_u6f_party_insert_1446_cpu(ram, RAM - 1u, &cpu, 64u, NULL, NULL) == PR_U6F_INSERT_CPU_ARGUMENT);
    CHECK(pr_u6f_party_insert_1446_cpu(ram, RAM + 1u, &cpu, 64u, NULL, NULL) == PR_U6F_INSERT_CPU_ARGUMENT);
    CHECK(memcmp(&cpu, &before, sizeof cpu) == 0 && memcmp(ram, want, RAM) == 0);
}

int main(void)
{
    ram = malloc(RAM);
    want = malloc(RAM);
    CHECK(ram != NULL && want != NULL);
    insertions();
    limits();
    stops();
    placement();
    arguments();
    free(ram);
    free(want);
    printf("u6f +1446 cpu: %u checks passed\n", checks);
    return 0;
}
