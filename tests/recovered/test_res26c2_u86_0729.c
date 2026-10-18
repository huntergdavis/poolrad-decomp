/* Focused synthetic test for raw-CPU RES_00BA_26C2 and u86 +0729 (docs/re/RES_26C2_U86_0729.md). No game bytes:
 * only +0729's far-call site bytes and synthetic tables. */
#include "poolrad/recovered/overlay_u86_entry3_0729.h"
#include "poolrad/recovered/resident_00ba_class_value_26c2_cpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM PR_RES_26C2_RAM_SIZE
#define CHECK(x)                                                                  \
    do {                                                                          \
        if (!(x)) {                                                               \
            fprintf(stderr, "res26c2/u86 line %d: %s\n", __LINE__, #x);          \
            exit(1);                                                              \
        }                                                                         \
        ++checks;                                                                 \
    } while (0)

enum { CS = 0x3800, RES_CS = 0x00BA, DS = 0x1800, SS = 0x2800, SP0 = 0x8000, REC_SEG = 0x6000, REC_OFF = 0x0100 };

static uint8_t *ram;
static unsigned checks;

static size_t at(unsigned seg, unsigned off) { return (((size_t)seg << 4) + (off & 0xFFFFu)) & (RAM - 1u); }
static void w8(unsigned seg, unsigned off, unsigned v) { ram[at(seg, off)] = (uint8_t)v; }
static uint8_t r8(unsigned seg, unsigned off) { return ram[at(seg, off)]; }
static void w16(unsigned seg, unsigned off, unsigned v) { w8(seg, off, v & 0xFFu); w8(seg, off + 1u, v >> 8); }
static unsigned r16(unsigned seg, unsigned off) { return r8(seg, off) | (unsigned)r8(seg, off + 1u) << 8; }

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

/* Table entry k at DS:31FA+16k = {class, ?, T2, T3, ?, ?, T6, ...}; record +96h/+9Bh; flag DS:6DBF; frame at SS:SP0. */
static void world(unsigned k, unsigned cls, unsigned t2, unsigned t3, unsigned t6, unsigned flag, unsigned stale)
{
    unsigned entry = 0x31FAu + 16u * k;
    memset(ram, 0x5C, RAM);
    w8(DS, entry, cls); w8(DS, entry + 2u, t2); w8(DS, entry + 3u, t3); w8(DS, entry + 6u, t6);
    w8(DS, 0x6DBFu, flag);
    w16(DS, 0x5D92u, REC_OFF); w16(DS, 0x5D94u, REC_SEG);
    w8(REC_SEG, REC_OFF + 0x96u, 0x41u); w8(REC_SEG, REC_OFF + 0x9Bu, 0x42u);
    w16(SS, SP0, 0x7000u); w16(SS, SP0 + 2u, 0x7001u); w16(SS, SP0 + 4u, 0xC300u | k);
    w8(SS, SP0 - 3u, stale);                                               /* the callee's [BP-1] before it runs */
    w8(CS, 0x073Au, 0x9Au); w16(CS, 0x073Bu, 0x26C2u); w16(CS, 0x073Du, RES_CS);
}

static pr_res_26c2_cpu res_cpu(void)
{
    pr_res_26c2_cpu cpu = {0xA5A5u, 0x2222u, 0x3333u, 0x4444u, 0x5555u, 0x6666u, 0x7777u, SP0, RES_CS, 0x26C2u, DS, 0x9999u, SS, 0x0202u};
    return cpu;
}

static pr_u86_0729_cpu u86_cpu(void)
{
    pr_u86_0729_cpu cpu = {0xA5A5u, 0x2222u, 0x3333u, 0x4444u, 0x5555u, 0x6666u, 0x7777u, SP0, CS, 0x0729u, DS, 0x9999u, SS, 0x0202u};
    return cpu;
}

static void returned(uint16_t ip, uint16_t cs, uint16_t sp, uint16_t bp)
{
    CHECK(ip == 0x7000u && cs == 0x7001u && sp == SP0 + 6u && bp == 0x7777u);
}

int main(void)
{
    static const unsigned want[4] = {0x41u, 0x42u, 0x0Cu, 0x77u};
    pr_res_26c2_cpu rc;
    pr_u86_0729_cpu uc;
    ram = malloc(RAM);
    CHECK(ram != NULL);

    /* RES_00BA_26C2, flag clear: class 0/1 read the record (ES:DI = record), 2 gives 0Ch, others keep [BP-1]. */
    for (unsigned cls = 0u; cls < 4u; ++cls) {
        world(0x21u, cls, 0u, 0u, 0u, 0u, 0x77u);
        rc = res_cpu();
        CHECK(pr_res_00ba_26c2_cpu(ram, RAM, &rc) == PR_RES_26C2_OK);
        CHECK(rc.ax == want[cls] && (rc.cx & 0xFFu) == 4u && (rc.cx >> 8) == 0x33u);
        CHECK(cls <= 1u ? (rc.es == REC_SEG && rc.di == REC_OFF) : (rc.es == 0x9999u && rc.di == 0x210u));
        CHECK(rc.flags == cmp_flags(0x0202u, 0u, 0u));                    /* CMP [6DBF],0 */
        CHECK(r8(SS, SP0 - 3u) == want[cls] && r16(SS, SP0 - 2u) == 0x7777u);
        returned(rc.ip, rc.cs, rc.sp, rc.bp);
    }
    /* Flag set: any class but 2 becomes 6 with CMP class,2's flags and DI = 16k. */
    for (unsigned cls = 0u; cls < 4u; ++cls) {
        world(0x21u, cls, 0u, 0u, 0u, 1u, 0x77u);
        rc = res_cpu();
        CHECK(pr_res_00ba_26c2_cpu(ram, RAM, &rc) == PR_RES_26C2_OK);
        CHECK(rc.ax == (cls == 2u ? 0x0Cu : 6u) && rc.di == 0x210u && rc.flags == cmp_flags(0x0202u, cls, 2u));
        returned(rc.ip, rc.cs, rc.sp, rc.bp);
    }

    /* +0729, flag clear: r = RES(k) (class 2: 0Ch); [BP-2] = T3*r + T2 = 3*12 + 5 = 41; DX = 36 (MUL low word). */
    world(0x10u, 2u, 5u, 3u, 0u, 0u, 0u);
    uc = u86_cpu();
    CHECK(pr_u86_entry3_0729(ram, RAM, &uc) == PR_U86_0729_OK);
    CHECK(uc.ax == 41u && uc.dx == 36u && uc.di == 0x100u && (uc.cx & 0xFFu) == 4u && uc.si == 0x5555u);
    CHECK(uc.flags == cmp_flags(0x0202u, 41u, 0xFFu));
    CHECK(r16(SS, SP0 - 6u) == 0xA510u && r16(SS, SP0 - 8u) == CS && r16(SS, SP0 - 10u) == 0x073Fu); /* PUSH AX, return frame */
    CHECK(r8(SS, SP0 - 3u) == 41u && r8(SS, SP0 - 4u) == 41u);           /* [BP-1] and [BP-2] */
    returned(uc.ip, uc.cs, uc.sp, uc.bp);

    /* +0729, flag set: [BP-2] = 6*T3 + T2 = 6*7 + 4 = 46; SI = 2*T3, DX = T2. */
    world(0x10u, 3u, 4u, 7u, 0u, 1u, 0u);
    uc = u86_cpu();
    CHECK(pr_u86_entry3_0729(ram, RAM, &uc) == PR_U86_0729_OK);
    CHECK(uc.ax == 46u && uc.si == 14u && uc.dx == 4u && uc.flags == cmp_flags(0x0202u, 46u, 0xFFu));
    returned(uc.ip, uc.cs, uc.sp, uc.bp);

    /* [BP-2] = 0 with T6 nonzero becomes 1 (AX = k from the reread); with T6 = 0 it stays 0. FFh becomes 1. */
    world(0x10u, 3u, 0xD6u, 7u, 9u, 1u, 0u);                              /* 6*7 + D6h = 0 mod 256 */
    uc = u86_cpu();
    CHECK(pr_u86_entry3_0729(ram, RAM, &uc) == PR_U86_0729_OK && uc.ax == 1u);
    world(0x10u, 3u, 0xD6u, 7u, 0u, 1u, 0u);
    uc = u86_cpu();
    CHECK(pr_u86_entry3_0729(ram, RAM, &uc) == PR_U86_0729_OK && uc.ax == 0u);
    world(0x10u, 3u, 0xFFu, 0u, 0u, 1u, 0u);
    uc = u86_cpu();
    CHECK(pr_u86_entry3_0729(ram, RAM, &uc) == PR_U86_0729_OK && uc.ax == 1u && uc.flags == cmp_flags(0x0202u, 0xFFu, 0xFFu));

    /* A reached +073A that is not LCALL xxxx:26C2 stops with CODE after the argument push. */
    world(0x10u, 2u, 5u, 3u, 0u, 0u, 0u);
    w16(CS, 0x073Bu, 0x26C3u);
    uc = u86_cpu();
    CHECK(pr_u86_entry3_0729(ram, RAM, &uc) == PR_U86_0729_CODE && uc.sp == SP0 - 6u);

    /* Argument rejections. */
    rc = res_cpu();
    uc = u86_cpu();
    CHECK(pr_res_00ba_26c2_cpu(NULL, RAM, &rc) == PR_RES_26C2_ARGUMENT);
    CHECK(pr_res_00ba_26c2_cpu(ram, RAM - 1u, &rc) == PR_RES_26C2_ARGUMENT);
    CHECK(pr_u86_entry3_0729(ram, RAM, NULL) == PR_U86_0729_ARGUMENT);
    CHECK(pr_u86_entry3_0729(ram, RAM + 1u, &uc) == PR_U86_0729_ARGUMENT);
    CHECK(rc.sp == SP0 && uc.sp == SP0);

    free(ram);
    printf("PASS: RES_00BA_26C2 + u86 +0729, %u checks (classes, flag override, both paths, fix-ups, CODE, arguments)\n", checks);
    return 0;
}
