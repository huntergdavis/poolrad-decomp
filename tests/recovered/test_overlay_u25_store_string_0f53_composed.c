/* Focused synthetic test for the composed U25 +0F53 store string with the raw-CPU SYSTEM 0B25 string store
 * (docs/re/U25_STORE_STRING_0F53.md). No game bytes: only the two five-byte far-call sites, synthetic
 * strings, window pointers and records. Every case compares all RAM outside the routine's stack frame with
 * an expected copy built from a byte-level reference, plus the frame's locals and the return state. */
#include "poolrad/recovered/overlay_u25_store_string_0f53.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM PR_U25_0F53_RAM_SIZE
#define CHECK(x)                                                                  \
    do {                                                                          \
        if (!(x)) {                                                               \
            fprintf(stderr, "u25 0f53 composed line %d: %s\n", __LINE__, #x);     \
            exit(1);                                                              \
        }                                                                         \
        ++checks;                                                                 \
    } while (0)

enum { CS = 0x8800, SYS = 0x0AF8, DS = 0x1800, SS = 0x2800, SP0 = 0x8000, BP1 = SP0 - 2, SRC = 0x6000, SOFF = 0x0010,
       WIN = 0x4000, REC = 0x7000, ROFF = 0x0300, RET_IP = 0x7000, RET_CS = 0x9001, BP0 = 0x7777, CAP = 300 };

static const unsigned pointers[4] = {0x49D2u, 0x49D6u, 0x49DAu, 0x49DEu};
static const unsigned displacements[4] = {0x6E00u, 0x2A00u, 0xD200u, 0x6700u};
static const unsigned ranges[4][2] = {{0x4900u, 0x4CFFu}, {0x6B00u, 0x6EFFu}, {0x9700u, 0x98FFu}, {0x9900u, 0xB6FFu}};
static const unsigned incs[4] = {0x0FA2u, 0x1030u, 0x109Du, 0x1109u};

static uint8_t *ram, *want;
static unsigned checks;

static size_t at(unsigned seg, unsigned off) { return (((size_t)seg << 4) + (off & 0xFFFFu)) & (RAM - 1u); }
static void w8(unsigned seg, unsigned off, unsigned v) { ram[at(seg, off)] = (uint8_t)v; }
static uint8_t r8(unsigned seg, unsigned off) { return ram[at(seg, off)]; }
static void w16(unsigned seg, unsigned off, unsigned v) { w8(seg, off, v & 0xFFu); w8(seg, off + 1u, v >> 8); }
static unsigned r16(unsigned seg, unsigned off) { return r8(seg, off) | (unsigned)r8(seg, off + 1u) << 8; }
static void e8(unsigned seg, unsigned off, unsigned v) { want[at(seg, off)] = (uint8_t)v; }

static int class_of(unsigned address)
{
    for (int c = 0; c < 4; ++c)
        if (address >= ranges[c][0] && address <= ranges[c][1]) return c;
    return 4;
}

/* Window c's far pointer puts element 0 at WIN:(0x0100 + 0x0400 c). */
static unsigned window_offset(int c, unsigned address)
{
    unsigned width = c < 3 ? 2u : 1u;
    return (0x0100u + 0x0400u * (unsigned)c - width * address - displacements[c]) & 0xFFFFu;
}

static pr_u25_0f53_cpu world(unsigned base, unsigned address, unsigned n, unsigned seed, unsigned flags)
{
    pr_u25_0f53_cpu cpu = {0x1234u, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu, BP0, SP0, CS,
                           (uint16_t)(base + 0x0F53u), DS, 0x4444u, SS, (uint16_t)flags};
    memset(ram, 0x5C, RAM);
    for (unsigned s = 0u; s < 2u; ++s) {
        unsigned site = base + (s ? 0x1010u : 0x0F69u);
        w8(CS, site, 0x9Au); w16(CS, site + 1u, 0x0B25u); w16(CS, site + 3u, SYS);
    }
    w8(SRC, SOFF, n);
    for (unsigned i = 1u; i < 256u; ++i) w8(SRC, SOFF + i, (seed + 7u * i) & 0xFFu);
    for (int c = 0; c < 4; ++c) { w16(DS, pointers[c], window_offset(c, address)); w16(DS, pointers[c] + 2u, WIN); }
    w16(DS, 0x5D92u, ROFF); w16(DS, 0x5D94u, REC);
    w16(SS, SP0, RET_IP); w16(SS, SP0 + 2u, RET_CS); w16(SS, SP0 + 4u, SOFF); w16(SS, SP0 + 6u, SRC); w16(SS, SP0 + 8u, address);
    return cpu;
}

/* The frame window SS:[SP0-120h, SP0+0Ah) is checked separately: copy what the model left there into the expectation. */
static void frame_free(void)
{
    for (unsigned off = SP0 - 0x120u; off < SP0 + 0x0Au; ++off) want[at(SS, off)] = ram[at(SS, off)];
}

static uint16_t add_flags(unsigned flags, unsigned left, unsigned right)
{
    unsigned wide = left + right, result = wide & 0xFFFFu, ones = 0u;
    for (unsigned bit = 0u; bit < 8u; ++bit) ones += (result >> bit) & 1u;
    flags &= ~0x08D5u;
    if (wide > 0xFFFFu) flags |= 0x0001u;
    if ((ones & 1u) == 0u) flags |= 0x0004u;
    if ((left ^ right ^ result) & 0x10u) flags |= 0x0010u;
    if (result == 0u) flags |= 0x0040u;
    if (result & 0x8000u) flags |= 0x0080u;
    if (~(left ^ right) & (left ^ result) & 0x8000u) flags |= 0x0800u;
    return (uint16_t)flags;
}

/* One complete call: byte-level reference of the copy, class, element stores and terminator. */
static void run_case(unsigned base, unsigned address, unsigned n, unsigned seed, unsigned flags)
{
    pr_u25_0f53_cpu cpu = world(base, address, n, seed, flags);
    int c = class_of(address), callee = 99;
    unsigned last = (n - 1u) & 0xFFu, local[257], element0 = WIN * 16u;
    memcpy(want, ram, RAM);
    for (unsigned i = 1u; i <= 255u; ++i) local[i] = i <= n ? r8(SRC, SOFF + i) : r8(SS, BP1 - 0x100u + i);
    local[256] = BP0 & 0xFFu;                                             /* the saved BP's low byte */
    (void)element0;
    if (c == 1 && address == 0x6B00u) {
        unsigned k = n < 15u ? n : 15u;
        e8(REC, ROFF, k);
        for (unsigned i = 1u; i <= k; ++i) e8(REC, ROFF + i, local[i]);
    } else if (c < 4) {
        unsigned width = c < 3 ? 2u : 1u, origin = 0x0100u + 0x0400u * (unsigned)c;
        for (unsigned i = 0u; i <= last; ++i) {
            e8(WIN, origin + width * i, local[i + 1u]);
            if (width == 2u) e8(WIN, origin + 2u * i + 1u, 0u);
        }
        e8(WIN, origin + width * n, 0u);                                  /* terminator at element n (8-bit n) */
        if (width == 2u) e8(WIN, origin + 2u * n + 1u, 0u);
    }
    CHECK(pr_u25_store_string_0f53_composed(ram, RAM, &cpu, CAP, &callee) == PR_U25_0F53_OK && callee == 0);
    /* frame: the copied string, the class, length, index and bound locals */
    CHECK(r8(SS, BP1 - 0x100u) == n && r8(SS, BP1 - 0x101u) == (unsigned)c && r8(SS, BP1 - 0x102u) == n);
    for (unsigned i = 1u; i <= n; ++i) CHECK(r8(SS, BP1 - 0x100u + i) == r8(SRC, SOFF + i));
    if (c < 4 && !(c == 1 && address == 0x6B00u))
        CHECK(r8(SS, BP1 - 0x104u) == last && r8(SS, BP1 - 0x103u) == last);
    if (c == 1 && address == 0x6B00u)
        CHECK(r16(SS, BP1 - 0x10Au) == REC);                              /* the name copy's PUSH ES reused the slot */
    else
        CHECK(r16(SS, BP1 - 0x10Au) == ((base + 0x0F75u) & 0xFFFFu));      /* the classifier's near return IP */
    CHECK(r16(SS, BP1 - 0x112u) == ((base + 0x0F6Eu) & 0xFFFFu) || (c == 1 && address == 0x6B00u));
    CHECK(cpu.sp == SP0 + 10u && cpu.bp == BP0 && cpu.cs == RET_CS && cpu.ip == RET_IP && cpu.ss == SS && cpu.ds == DS);
    CHECK((cpu.flags & 0x0400u) == 0u);                                   /* the store's CLD */
    if (c == 1 && address == 0x6B00u) {
        CHECK(r16(SS, BP1 - 0x112u) == ((base + 0x1015u) & 0xFFFFu));      /* the second store's return IP */
        CHECK(cpu.cx == 0u && cpu.es == REC && (cpu.flags & 0x08D5u) == 0x0044u);
    } else if (c < 3) {
        CHECK(cpu.ax == 0u && (cpu.flags & 0x08D5u) == 0x0044u && cpu.dx == local[last + 1u] && cpu.es == WIN);
    } else if (c == 3) {
        unsigned pointer = window_offset(3, address);
        CHECK((cpu.dx & 0xFFu) == local[last + 1u] && cpu.es == WIN);
        CHECK(cpu.flags == add_flags(cpu.flags, pointer, (n + address) & 0xFFFFu));
        CHECK(cpu.di == ((pointer + n + address) & 0xFFFFu));
    } else {
        CHECK((cpu.ax & 0xFFu) == 4u && (cpu.flags & 0x08D5u) == 0u);      /* CMP AL,3 with AL = 4 */
    }
    frame_free();
    CHECK(memcmp(ram, want, RAM) == 0);
}

static void classes(void)
{
    static const unsigned lengths[] = {0u, 1u, 3u, 20u, 255u};
    static const unsigned flags[] = {0x0002u, 0x0ED7u, 0x0602u, 0x08D7u};
    unsigned k = 0u;
    for (int c = 0; c < 4; ++c)
        for (unsigned a = 0u; a < 4u; ++a) {
            unsigned address = a == 0u ? ranges[c][0] : a == 1u ? ranges[c][0] + 1u : a == 2u ? ranges[c][1] - 1u : ranges[c][1];
            for (unsigned l = 0u; l < 5u; ++l, ++k) run_case(0u, address, lengths[l], 0x31u * k, flags[k & 3u]);
        }
    for (unsigned a = 0u; a < 6u; ++a) {
        static const unsigned outside[] = {0x0000u, 0x48FFu, 0x4D00u, 0x6F00u, 0xB700u, 0xFFFFu};
        run_case(0u, outside[a], 7u, a, 0x0002u);
    }
    run_case(0x1000u, 0x4A00u, 9u, 3u, 0x0002u);                          /* another entry offset */
    run_case(0x1000u, 0x6B00u, 9u, 4u, 0x0002u);
}

/* A cap below the loop length stops at that loop's INC with IP relative to the entry. */
static void limits(void)
{
    for (int c = 0; c < 4; ++c) {
        unsigned address = ranges[c][0] + 1u, width = c < 3 ? 2u : 1u, origin = 0x0100u + 0x0400u * (unsigned)c;
        pr_u25_0f53_cpu cpu = world(0u, address, 20u, 9u, 0x0002u);
        int callee = 99;
        CHECK(pr_u25_store_string_0f53_composed(ram, RAM, &cpu, 3u, &callee) == PR_U25_0F53_LIMIT && callee == 0);
        CHECK(cpu.ip == incs[c] && cpu.cs == CS && r8(SS, BP1 - 0x103u) == 3u && r8(SS, BP1 - 0x104u) == 19u);
        CHECK(r8(WIN, origin + width * 3u) == r8(SRC, SOFF + 4u) && r8(WIN, origin + width * 4u) == 0x5Cu);
        CHECK((cpu.flags & 0x0041u) == 0x0001u);                           /* CMP 3,19: JNE taken */
    }
}

static void stops(void)
{
    pr_u25_0f53_cpu cpu;
    int callee = 99;
    cpu = world(0u, 0x4A00u, 5u, 1u, 0x0002u);
    w8(CS, 0x0F69u, 0x90u);
    CHECK(pr_u25_store_string_0f53_composed(ram, RAM, &cpu, CAP, &callee) == PR_U25_0F53_CODE && callee == -1);
    CHECK(cpu.ip == 0x0F69u && cpu.sp == ((BP1 - 0x104u - 10u) & 0xFFFFu) && cpu.bp == BP1 && cpu.ax == 0x00FFu);
    CHECK(r16(SS, cpu.sp) == 0x00FFu && r16(SS, cpu.sp + 2u) == BP1 - 0x100u && r16(SS, cpu.sp + 4u) == SS);
    cpu = world(0u, 0x6B00u, 5u, 1u, 0x0002u);
    w8(CS, 0x1011u, 0x26u);
    CHECK(pr_u25_store_string_0f53_composed(ram, RAM, &cpu, CAP, &callee) == PR_U25_0F53_CODE && callee == 0);
    CHECK(cpu.ip == 0x1010u && cpu.ax == 0x000Fu && cpu.es == REC && cpu.di == ROFF && (cpu.flags & 0x0040u) != 0u);
    /* the name copy onto the second store's own return frame: its RETF 0Ah goes elsewhere */
    cpu = world(0u, 0x6B00u, 40u, 2u, 0x0002u);
    w16(DS, 0x5D92u, BP1 - 0x112u); w16(DS, 0x5D94u, SS);
    CHECK(pr_u25_store_string_0f53_composed(ram, RAM, &cpu, CAP, &callee) == PR_U25_0F53_TRANSFER && callee == 0);
    CHECK(cpu.ip == (15u | ((unsigned)r8(SRC, SOFF + 1u) << 8)) && cpu.sp == ((BP1 - 0x104u) & 0xFFFFu));
}

static void arguments(void)
{
    pr_u25_0f53_cpu cpu = world(0u, 0x4A00u, 5u, 1u, 0x0002u), before = cpu;
    int callee = 99;
    memcpy(want, ram, RAM);
    CHECK(pr_u25_store_string_0f53_composed(NULL, RAM, &cpu, CAP, &callee) == PR_U25_0F53_ARGUMENT && callee == -1);
    callee = 99;
    CHECK(pr_u25_store_string_0f53_composed(ram, RAM, NULL, CAP, &callee) == PR_U25_0F53_ARGUMENT && callee == -1);
    CHECK(pr_u25_store_string_0f53_composed(ram, RAM - 1u, &cpu, CAP, NULL) == PR_U25_0F53_ARGUMENT);
    CHECK(pr_u25_store_string_0f53_composed(ram, RAM + 1u, &cpu, CAP, NULL) == PR_U25_0F53_ARGUMENT);
    CHECK(memcmp(&cpu, &before, sizeof cpu) == 0 && memcmp(ram, want, RAM) == 0);
    CHECK(pr_u25_store_string_0f53_composed(ram, RAM, &cpu, CAP, NULL) == PR_U25_0F53_OK);
}

int main(void)
{
    ram = malloc(RAM);
    want = malloc(RAM);
    CHECK(ram != NULL && want != NULL);
    classes();
    limits();
    stops();
    arguments();
    free(ram);
    free(want);
    printf("u25 +0f53 composed: %u checks passed\n", checks);
    return 0;
}
