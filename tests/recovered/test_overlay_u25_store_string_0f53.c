#include "poolrad/recovered/overlay_u25_store_string_0f53.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define DS 0x3000u
#define SS 0xE000u
#define SP 0x8000u
#define BP (SP - 2u)
#define CS 0xF000u
#define SRC_SEG 0x2000u
#define SRC_OFF 0x0080u

typedef struct {
    unsigned calls;
    unsigned abort_at;
} fixture;

static const struct { uint16_t pointer, seg, base, disp; } windows[4] = {
    {0x49D2u, 0x4000u, 0x0100u, 0x6E00u}, {0x49D6u, 0x5000u, 0x0200u, 0x2A00u},
    {0x49DAu, 0x6000u, 0x0300u, 0xD200u}, {0x49DEu, 0x7000u, 0x0400u, 0x6700u}
};

static size_t physical(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & (PR_U25_0F53_RAM_SIZE - 1u);
}

static void put16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t value)
{
    size_t p = physical(seg, off);
    ram[p] = (uint8_t)value;
    ram[(p + 1u) & (PR_U25_0F53_RAM_SIZE - 1u)] = (uint8_t)(value >> 8);
}

static uint16_t get16(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    size_t p = physical(seg, off);
    return (uint16_t)(ram[p] |
        ((uint16_t)ram[(p + 1u) & (PR_U25_0F53_RAM_SIZE - 1u)] << 8));
}

static uint16_t arg(const uint8_t *ram, const pr_u25_0f53_cpu *cpu, unsigned i)
{
    return get16(ram, cpu->ss, (uint16_t)(cpu->sp + 2u * i));
}

/* Synthetic StrStore with the decoded SYSTEM AF8:0B25 effect. */
static int external(void *context, uint8_t *ram, size_t ram_size,
                    pr_u25_0f53_cpu *cpu, pr_u25_0f53_call call)
{
    fixture *f = context;
    uint16_t cap = arg(ram, cpu, 0), doff = arg(ram, cpu, 1), dseg = arg(ram, cpu, 2);
    uint16_t soff = arg(ram, cpu, 3), sseg = arg(ram, cpu, 4);
    unsigned n, i;
    assert(ram_size == PR_U25_0F53_RAM_SIZE && call == PR_U25_0F53_STRSTORE);
    ++f->calls;
    if (f->calls == f->abort_at) return 0;
    n = ram[physical(sseg, soff)];
    if (n > (cap & 0xFFu)) n = cap & 0xFFu;
    ram[physical(dseg, doff)] = (uint8_t)n;
    for (i = 1u; i <= n; ++i)
        ram[physical(dseg, (uint16_t)(doff + i))] =
            ram[physical(sseg, (uint16_t)(soff + i))];
    cpu->flags = 0x0A53u;
    return 1;
}

static uint8_t *prepare(pr_u25_0f53_cpu *cpu, uint16_t address,
                        const char *text, size_t n)
{
    uint8_t *ram = calloc(PR_U25_0F53_RAM_SIZE, 1u);
    unsigned i;
    assert(ram != NULL);
    memset(cpu, 0, sizeof *cpu);
    cpu->ax = 0xA5CDu; cpu->bp = 0x7777u; cpu->sp = SP; cpu->dx = 0x5555u;
    cpu->cs = CS; cpu->ip = 0x0F53u; cpu->ds = DS; cpu->ss = SS;
    cpu->flags = 0x0202u;
    put16(ram, SS, SP, 0x0A05u);
    put16(ram, SS, SP + 2u, 0xD000u);
    put16(ram, SS, SP + 4u, SRC_OFF);
    put16(ram, SS, SP + 6u, SRC_SEG);
    put16(ram, SS, SP + 8u, address);
    ram[physical(SRC_SEG, SRC_OFF)] = (uint8_t)n;
    memcpy(&ram[physical(SRC_SEG, SRC_OFF + 1u)], text, n);
    for (i = 0u; i < 4u; ++i) {
        put16(ram, DS, windows[i].pointer, windows[i].base);
        put16(ram, DS, (uint16_t)(windows[i].pointer + 2u), windows[i].seg);
    }
    /* Stale bytes in the local string buffer [bp-0x100..bp-1]. */
    for (i = 0u; i < 0x100u; ++i)
        ram[physical(SS, (uint16_t)(BP - 0x100u + i))] = (uint8_t)(0x80u + i);
    return ram;
}

static pr_u25_0f53_status run(uint8_t *ram, pr_u25_0f53_cpu *cpu, fixture *f)
{
    return pr_u25_store_string_0f53(ram, PR_U25_0F53_RAM_SIZE, cpu, external, f);
}

static uint16_t word_at(const uint8_t *ram, unsigned w, uint16_t address, unsigned i)
{
    return get16(ram, windows[w].seg,
                 (uint16_t)(windows[w].base + 2u * (uint16_t)(address + i) +
                            windows[w].disp));
}

static void returned(const pr_u25_0f53_cpu *cpu)
{
    assert(cpu->sp == SP + 10u && cpu->bp == 0x7777u);
    assert(cpu->ip == 0x0A05u && cpu->cs == 0xD000u);
}

static void test_word_and_byte_classes(void)
{
    static const uint16_t addresses[3] = {0x4CFEu, 0x6B10u, 0x9700u};
    pr_u25_0f53_cpu cpu;
    fixture f;
    uint8_t *ram;
    unsigned w;
    uint16_t at;
    for (w = 0u; w < 3u; ++w) {
        memset(&f, 0, sizeof f);
        ram = prepare(&cpu, addresses[w], "AB", 2u);
        put16(ram, windows[w].seg, (uint16_t)(windows[w].base +
              2u * (uint16_t)(addresses[w] + 2u) + windows[w].disp), 0xBEEFu);
        assert(run(ram, &cpu, &f) == PR_U25_0F53_OK && f.calls == 1u);
        assert(word_at(ram, w, addresses[w], 0u) == 'A');
        assert(word_at(ram, w, addresses[w], 1u) == 'B');
        assert(word_at(ram, w, addresses[w], 2u) == 0u);
        assert(word_at(ram, w, addresses[w], 3u) == 0u);
        returned(&cpu);
        /* XOR AX,AX before the terminator store is the last flag writer. */
        assert((cpu.flags & 0x08D5u) == 0x0044u && cpu.ax == 0u);
        assert(ram[physical(SS, (uint16_t)(BP - 0x101u))] == w);
        free(ram);
    }
    memset(&f, 0, sizeof f);
    ram = prepare(&cpu, 0xB6FDu, "xyz", 3u);
    assert(run(ram, &cpu, &f) == PR_U25_0F53_OK && f.calls == 1u);
    at = (uint16_t)(0x0400u + 0xB6FDu + 0x6700u);
    assert(memcmp(&ram[physical(0x7000u, at)], "xyz", 4u) == 0);
    assert((cpu.dx & 0xFFu) == 'z' && (cpu.dx >> 8) == 0x55u);
    returned(&cpu);
    free(ram);
}

static void test_name_none_and_zero_length(void)
{
    pr_u25_0f53_cpu cpu;
    fixture f = {0u, 0u};
    uint8_t *ram = prepare(&cpu, 0x6B00u, "abcdefghijklmnopqrst", 20u);
    unsigned i;
    put16(ram, DS, 0x5D92u, 0x0040u); put16(ram, DS, 0x5D94u, 0x2100u);
    assert(run(ram, &cpu, &f) == PR_U25_0F53_OK && f.calls == 2u);
    assert(ram[physical(0x2100u, 0x0040u)] == 15u);
    assert(memcmp(&ram[physical(0x2100u, 0x0041u)], "abcdefghijklmno", 15u) == 0);
    assert(cpu.flags == 0x0A53u);
    returned(&cpu);
    free(ram);

    memset(&f, 0, sizeof f);
    ram = prepare(&cpu, 0x0100u, "q", 1u);
    assert(run(ram, &cpu, &f) == PR_U25_0F53_OK && f.calls == 1u);
    assert((cpu.flags & 0x08D5u) == 0u && (cpu.ax & 0xFFu) == 4u);
    returned(&cpu);
    free(ram);

    /* Zero length: 256 words from the stale local buffer, the last one
     * read from [bp+0] (the saved BP's low byte), then the terminator
     * overwrites element 0. */
    memset(&f, 0, sizeof f);
    ram = prepare(&cpu, 0x4900u, "", 0u);
    assert(run(ram, &cpu, &f) == PR_U25_0F53_OK);
    assert(word_at(ram, 0u, 0x4900u, 0u) == 0u);
    for (i = 1u; i < 255u; ++i)
        assert(word_at(ram, 0u, 0x4900u, i) == (uint8_t)(0x80u + i + 1u));
    assert(word_at(ram, 0u, 0x4900u, 255u) == 0x77u);
    assert(ram[physical(SS, (uint16_t)(BP - 0x103u))] == 0xFFu);
    returned(&cpu);
    free(ram);
}

static void test_abort_and_arguments(void)
{
    pr_u25_0f53_cpu cpu;
    fixture f = {0u, 1u};
    uint8_t *ram = prepare(&cpu, 0x4900u, "A", 1u);
    assert(run(ram, &cpu, &f) == PR_U25_0F53_CALL_ABORT && cpu.ip == 0x0F69u);
    free(ram);
    f.calls = 0u; f.abort_at = 2u;
    ram = prepare(&cpu, 0x6B00u, "A", 1u);
    assert(run(ram, &cpu, &f) == PR_U25_0F53_CALL_ABORT && cpu.ip == 0x1010u);
    free(ram);
    ram = prepare(&cpu, 0x4900u, "A", 1u);
    assert(pr_u25_store_string_0f53(NULL, PR_U25_0F53_RAM_SIZE, &cpu, external, &f)
           == PR_U25_0F53_ARGUMENT);
    assert(pr_u25_store_string_0f53(ram, PR_U25_0F53_RAM_SIZE - 1u, &cpu, external, &f)
           == PR_U25_0F53_ARGUMENT);
    assert(pr_u25_store_string_0f53(ram, PR_U25_0F53_RAM_SIZE, NULL, external, &f)
           == PR_U25_0F53_ARGUMENT);
    assert(pr_u25_store_string_0f53(ram, PR_U25_0F53_RAM_SIZE, &cpu, NULL, &f)
           == PR_U25_0F53_ARGUMENT);
    assert(cpu.sp == SP);
    free(ram);
}

int main(void)
{
    test_word_and_byte_classes();
    test_name_none_and_zero_length();
    test_abort_and_arguments();
    return 0;
}
