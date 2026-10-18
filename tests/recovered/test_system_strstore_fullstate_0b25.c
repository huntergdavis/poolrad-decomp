/* Focused synthetic test for the raw-CPU SYSTEM 0AF8:0B25 string store (docs/re/SYSTEM_STRSTORE_0B25.md).
 * No game bytes: the model is data-only, so every case builds a frame and strings in a 1 MiB buffer and
 * compares the whole buffer with an expected copy. */
#include "poolrad/recovered/system_strstore_fullstate_0b25.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM PR_SYSTEM_0B25_RAM_SIZE
#define CHECK(x)                                                                  \
    do {                                                                          \
        if (!(x)) {                                                               \
            fprintf(stderr, "system 0b25 line %d: %s\n", __LINE__, #x);          \
            exit(1);                                                              \
        }                                                                         \
        ++checks;                                                                 \
    } while (0)

enum { SS = 0x2800, SP0 = 0x8000, SRC = 0x5000, DST = 0x7000, DS0 = 0x1111, RET_IP = 0x7A00, RET_CS = 0x7A01 };

static uint8_t *ram, *want;
static unsigned checks;

static size_t at(unsigned seg, unsigned off) { return (((size_t)seg << 4) + (off & 0xFFFFu)) & (RAM - 1u); }
static void w8(unsigned seg, unsigned off, unsigned v) { ram[at(seg, off)] = (uint8_t)v; }
static uint8_t r8(unsigned seg, unsigned off) { return ram[at(seg, off)]; }
static void w16(unsigned seg, unsigned off, unsigned v) { w8(seg, off, v & 0xFFu); w8(seg, off + 1u, v >> 8); }
static void expect8(unsigned seg, unsigned off, unsigned v) { want[at(seg, off)] = (uint8_t)v; }

/* XOR CH,CH's flags after CLD: ZF and PF set; CF, OF, SF, AF and DF clear; everything else kept. */
static uint16_t final_flags(unsigned flags) { return (uint16_t)((flags & ~0x0CD5u) | 0x0044u); }

/* [SP] return IP, [SP+2] return CS, [SP+4] maximum length, [SP+6]/[SP+8] destination, [SP+0Ah]/[SP+0Ch] source. */
static pr_system_0b25_cpu frame(unsigned ss, unsigned sp, unsigned cap, unsigned dseg, unsigned doff, unsigned sseg,
                                unsigned soff, unsigned flags)
{
    pr_system_0b25_cpu cpu = {0x1234u, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu, 0x7777u, (uint16_t)sp,
                              0x0AF8u, 0x0B25u, DS0, 0x4444u, (uint16_t)ss, (uint16_t)flags};
    w16(ss, sp, RET_IP); w16(ss, sp + 2u, RET_CS); w16(ss, sp + 4u, cap);
    w16(ss, sp + 6u, doff); w16(ss, sp + 8u, dseg); w16(ss, sp + 10u, soff); w16(ss, sp + 12u, sseg);
    return cpu;
}

static void source(unsigned seg, unsigned off, unsigned length, unsigned seed)
{
    w8(seg, off, length);
    for (unsigned i = 1u; i < 256u; ++i) w8(seg, off + i, (seed + 3u * i) & 0xFFu);
}

static void run(pr_system_0b25_cpu *cpu)
{
    CHECK(pr_system_strstore_0b25(ram, RAM, cpu) == PR_SYSTEM_0B25_RETURNED);
}

/* Registers every return shares: AH kept, BX = entry SP, CX = 0, DX/DS = entry DS, SI/DI past the copy. */
static void registers(const pr_system_0b25_cpu *cpu, unsigned ss, unsigned sp, unsigned k, unsigned sseg, unsigned soff,
                      unsigned dseg, unsigned doff, unsigned flags)
{
    (void)sseg;
    CHECK(cpu->ax == (0x1200u | k));
    CHECK(cpu->bx == sp && cpu->cx == 0u && cpu->dx == DS0);
    CHECK(cpu->si == ((soff + 1u + k) & 0xFFFFu));
    CHECK(cpu->di == ((doff + 1u + k) & 0xFFFFu));
    CHECK(cpu->bp == 0x7777u && cpu->sp == ((sp + 14u) & 0xFFFFu));
    CHECK(cpu->ds == DS0 && cpu->es == dseg && cpu->ss == ss);
    CHECK(cpu->flags == final_flags(flags));
}

static void arguments(void)
{
    pr_system_0b25_cpu cpu, before;
    memset(ram, 0x5C, RAM);
    cpu = frame(SS, SP0, 0x28u, DST, 0x0200u, SRC, 0x0100u, 0x0ED7u);
    source(SRC, 0x0100u, 0x10u, 0x40u);
    before = cpu;
    memcpy(want, ram, RAM);
    CHECK(pr_system_strstore_0b25(NULL, RAM, &cpu) == PR_SYSTEM_0B25_ARGUMENT);
    CHECK(pr_system_strstore_0b25(ram, RAM, NULL) == PR_SYSTEM_0B25_ARGUMENT);
    CHECK(pr_system_strstore_0b25(ram, RAM - 1u, &cpu) == PR_SYSTEM_0B25_ARGUMENT);
    CHECK(pr_system_strstore_0b25(ram, RAM + 1u, &cpu) == PR_SYSTEM_0B25_ARGUMENT);
    CHECK(memcmp(&cpu, &before, sizeof cpu) == 0 && memcmp(ram, want, RAM) == 0);
}

/* Every source length against caps below, at and above it; only CL limits the copy. */
static void sweep(void)
{
    static const unsigned fixed[] = {0x0000u, 0x0001u, 0x0028u, 0x00FFu, 0x0100u, 0x0105u, 0xFF80u, 0xFFFFu};
    static const unsigned flags[] = {0x0002u, 0x0ED7u, 0x0602u, 0x08D7u};
    for (unsigned n = 0u; n < 256u; ++n)
        for (unsigned c = 0u; c < 11u; ++c) {
            unsigned cap = c < 8u ? fixed[c] : (((n + c - 9u) & 0xFFu) | ((n * 0x0101u) & 0xFF00u));
            unsigned k = n < (cap & 0xFFu) ? n : (cap & 0xFFu), f = flags[(n + c) & 3u], seed = n ^ (c << 4);
            pr_system_0b25_cpu cpu;
            memset(ram, 0x5C, RAM);
            source(SRC, 0x0100u, n, seed);
            cpu = frame(SS, SP0, cap, DST, 0x0200u, SRC, 0x0100u, f);
            memcpy(want, ram, RAM);
            expect8(DST, 0x0200u, k);
            for (unsigned i = 1u; i <= k; ++i) expect8(DST, 0x0200u + i, (seed + 3u * i) & 0xFFu);
            run(&cpu);
            registers(&cpu, SS, SP0, k, SRC, 0x0100u, DST, 0x0200u, f);
            CHECK(cpu.cs == RET_CS && cpu.ip == RET_IP);
            CHECK(memcmp(ram, want, RAM) == 0);
        }
}

static void literal_flags(void)
{
    pr_system_0b25_cpu cpu;
    memset(ram, 0x5C, RAM);
    source(SRC, 0x0100u, 3u, 0x61u);
    cpu = frame(SS, SP0, 0x28u, DST, 0x0200u, SRC, 0x0100u, 0x0ED7u);
    run(&cpu);
    CHECK(cpu.flags == 0x0246u);                                           /* DF, OF, SF, AF, CF cleared */
    memset(ram, 0x5C, RAM);
    source(SRC, 0x0100u, 3u, 0x61u);
    cpu = frame(SS, SP0, 0x28u, DST, 0x0200u, SRC, 0x0100u, 0x0002u);
    run(&cpu);
    CHECK(cpu.flags == 0x0046u);
}

/* Forward byte copies: an overlapping destination reads back what it just wrote. */
static void overlaps(void)
{
    pr_system_0b25_cpu cpu;
    /* destination one above the source: the stored length byte propagates through the copy */
    memset(ram, 0x5C, RAM);
    for (unsigned i = 1u; i < 0x40u; ++i) w8(SRC, 0x0400u + i, 0x80u + i);
    w8(SRC, 0x0400u, 0x10u);
    cpu = frame(SS, SP0, 0xFFu, SRC, 0x0401u, SRC, 0x0400u, 0x0002u);
    memcpy(want, ram, RAM);
    for (unsigned i = 0u; i <= 0x11u; ++i) expect8(SRC, 0x0400u + i, 0x10u);
    run(&cpu);
    CHECK(memcmp(ram, want, RAM) == 0 && r8(SRC, 0x0412u) == 0x92u);
    registers(&cpu, SS, SP0, 0x10u, SRC, 0x0400u, SRC, 0x0401u, 0x0002u);
    /* destination one below: a plain left shift */
    memset(ram, 0x5C, RAM);
    for (unsigned i = 1u; i < 0x40u; ++i) w8(SRC, 0x0400u + i, 0x80u + i);
    w8(SRC, 0x0400u, 0x10u);
    cpu = frame(SS, SP0, 0xFFu, SRC, 0x03FFu, SRC, 0x0400u, 0x0002u);
    memcpy(want, ram, RAM);
    expect8(SRC, 0x03FFu, 0x10u);
    for (unsigned i = 0u; i < 0x10u; ++i) expect8(SRC, 0x0400u + i, 0x81u + i);
    run(&cpu);
    CHECK(memcmp(ram, want, RAM) == 0 && r8(SRC, 0x040Fu) == 0x90u && r8(SRC, 0x0410u) == 0x90u);
    registers(&cpu, SS, SP0, 0x10u, SRC, 0x0400u, SRC, 0x03FFu, 0x0002u);
    /* destination equal to the source: only the length byte changes when the cap truncates */
    memset(ram, 0x5C, RAM);
    source(SRC, 0x0400u, 0x10u, 0x20u);
    cpu = frame(SS, SP0, 0x0108u, SRC, 0x0400u, SRC, 0x0400u, 0x0002u);
    memcpy(want, ram, RAM);
    expect8(SRC, 0x0400u, 0x08u);
    run(&cpu);
    CHECK(memcmp(ram, want, RAM) == 0);
    registers(&cpu, SS, SP0, 0x08u, SRC, 0x0400u, SRC, 0x0400u, 0x0002u);
}

/* SI and DI wrap at 16 bits inside their segments; physical addresses wrap at 1 MiB. */
static void wraps(void)
{
    pr_system_0b25_cpu cpu;
    memset(ram, 0x5C, RAM);
    w8(SRC, 0xFFFEu, 4u); w8(SRC, 0xFFFFu, 0xA1u); w8(SRC, 0x0000u, 0xA2u); w8(SRC, 0x0001u, 0xA3u); w8(SRC, 0x0002u, 0xA4u);
    ram[((size_t)SRC << 4) + 0x10000u] = 0xEEu;                            /* the byte past the segment is never read */
    cpu = frame(SS, SP0, 0x28u, DST, 0x0200u, SRC, 0xFFFEu, 0x0002u);
    memcpy(want, ram, RAM);
    expect8(DST, 0x0200u, 4u); expect8(DST, 0x0201u, 0xA1u); expect8(DST, 0x0202u, 0xA2u);
    expect8(DST, 0x0203u, 0xA3u); expect8(DST, 0x0204u, 0xA4u);
    run(&cpu);
    CHECK(memcmp(ram, want, RAM) == 0 && cpu.si == 0x0003u);
    memset(ram, 0x5C, RAM);
    w8(SRC, 0x0100u, 3u); w8(SRC, 0x0101u, 0xB1u); w8(SRC, 0x0102u, 0xB2u); w8(SRC, 0x0103u, 0xB3u);
    cpu = frame(SS, SP0, 0x28u, DST, 0xFFFFu, SRC, 0x0100u, 0x0002u);
    memcpy(want, ram, RAM);
    expect8(DST, 0xFFFFu, 3u); expect8(DST, 0x0000u, 0xB1u); expect8(DST, 0x0001u, 0xB2u); expect8(DST, 0x0002u, 0xB3u);
    run(&cpu);
    CHECK(memcmp(ram, want, RAM) == 0 && cpu.di == 0x0003u && ram[((size_t)DST << 4) + 0x10000u] == 0x5Cu);
    /* FFFF:0100 is physical 000F0h and FFFF:0300 is 002F0h */
    memset(ram, 0x5C, RAM);
    ram[0x000F0u] = 3u; ram[0x000F1u] = 0xC1u; ram[0x000F2u] = 0xC2u; ram[0x000F3u] = 0xC3u;
    cpu = frame(SS, SP0, 0x28u, 0xFFFFu, 0x0300u, 0xFFFFu, 0x0100u, 0x0002u);
    memcpy(want, ram, RAM);
    want[0x002F0u] = 3u; want[0x002F1u] = 0xC1u; want[0x002F2u] = 0xC2u; want[0x002F3u] = 0xC3u;
    run(&cpu);
    CHECK(memcmp(ram, want, RAM) == 0);
    registers(&cpu, SS, SP0, 3u, 0xFFFFu, 0x0100u, 0xFFFFu, 0x0300u, 0x0002u);
}

/* Stack frames at a segment end: BX+disp wraps at 16 bits, but the LDS pair and the RETF frame are read
 * physically contiguous, as Unicorn reads them (docs/re/SYSTEM_STRSTORE_0B25.md). */
static void stack_edges(void)
{
    pr_system_0b25_cpu cpu;
    size_t past = ((size_t)SS << 4) + 0x10000u;
    /* SP = FFF4: the source segment word comes from the byte pair past the segment, not SS:0000 */
    memset(ram, 0x5C, RAM);
    source(SRC, 0x0100u, 2u, 0x30u);
    source(0x6600u, 0x0100u, 2u, 0x90u);
    cpu = frame(SS, 0xFFF4u, 0x28u, DST, 0x0200u, SRC, 0x0100u, 0x0002u);
    w16(SS, 0x0000u, 0x6600u);                                             /* the 16-bit-wrapped word, not read */
    ram[past] = (uint8_t)SRC; ram[past + 1u] = (uint8_t)(SRC >> 8);
    memcpy(want, ram, RAM);
    expect8(DST, 0x0200u, 2u); expect8(DST, 0x0201u, 0x33u); expect8(DST, 0x0202u, 0x36u);
    run(&cpu);
    CHECK(memcmp(ram, want, RAM) == 0 && cpu.sp == 0x0002u && cpu.bx == 0xFFF4u);
    /* SP = FFFE: the argument words wrap to SS:0002..000B, the return CS is read past the segment */
    memset(ram, 0x5C, RAM);
    source(SRC, 0x0100u, 2u, 0x30u);
    w16(SS, 0xFFFEu, RET_IP); w16(SS, 0x0000u, 0xDEADu);
    ram[past] = (uint8_t)RET_CS; ram[past + 1u] = (uint8_t)(RET_CS >> 8);
    w16(SS, 0x0002u, 0x28u); w16(SS, 0x0004u, 0x0200u); w16(SS, 0x0006u, DST); w16(SS, 0x0008u, 0x0100u); w16(SS, 0x000Au, SRC);
    cpu = (pr_system_0b25_cpu){0x1234u, 0x3456u, 0x5678u, 0x789Au, 0x9ABCu, 0xBCDEu, 0x7777u, 0xFFFEu,
                               0x0AF8u, 0x0B25u, DS0, 0x4444u, SS, 0x0002u};
    memcpy(want, ram, RAM);
    expect8(DST, 0x0200u, 2u); expect8(DST, 0x0201u, 0x33u); expect8(DST, 0x0202u, 0x36u);
    run(&cpu);
    CHECK(memcmp(ram, want, RAM) == 0 && cpu.cs == RET_CS && cpu.ip == RET_IP && cpu.sp == 0x000Cu);
    /* SS:SP = FFFF:000E is physical FFFFEh: the return CS comes from physical 00000h */
    memset(ram, 0x5C, RAM);
    source(SRC, 0x0100u, 1u, 0x40u);
    cpu = frame(0xFFFFu, 0x000Eu, 0x28u, DST, 0x0200u, SRC, 0x0100u, 0x0002u);
    CHECK(ram[0xFFFFEu] == (uint8_t)RET_IP && ram[0x00000u] == (uint8_t)RET_CS);
    memcpy(want, ram, RAM);
    expect8(DST, 0x0200u, 1u); expect8(DST, 0x0201u, 0x43u);
    run(&cpu);
    CHECK(memcmp(ram, want, RAM) == 0 && cpu.cs == RET_CS && cpu.ip == RET_IP && cpu.sp == 0x001Cu);
}

int main(void)
{
    ram = malloc(RAM);
    want = malloc(RAM);
    CHECK(ram != NULL && want != NULL);
    arguments();
    sweep();
    literal_flags();
    overlaps();
    wraps();
    stack_edges();
    free(ram);
    free(want);
    printf("system 0b25 string store: %u checks passed\n", checks);
    return 0;
}
