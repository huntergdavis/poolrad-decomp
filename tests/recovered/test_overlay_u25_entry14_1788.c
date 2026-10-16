#include "poolrad/recovered/overlay_u25_entry14_1788.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define DS 0x3000u
#define SS 0xE000u
#define SP 0x8000u
#define BP (SP - 2u)
#define CS 0xF000u

typedef struct {
    unsigned calls;
    unsigned abort_at;
    int redirect_156d;
    uint16_t forwarded[20];
    char log[256];
} fixture;

static size_t physical(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & (PR_U25_1788_RAM_SIZE - 1u);
}

static void put16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t value)
{
    size_t p = physical(seg, off);
    ram[p] = (uint8_t)value;
    ram[(p + 1u) & (PR_U25_1788_RAM_SIZE - 1u)] = (uint8_t)(value >> 8);
}

static uint16_t get16(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    size_t p = physical(seg, off);
    return (uint16_t)(ram[p] |
        ((uint16_t)ram[(p + 1u) & (PR_U25_1788_RAM_SIZE - 1u)] << 8));
}

static uint16_t arg(const uint8_t *ram, const pr_u25_1788_cpu *cpu, unsigned i)
{
    return get16(ram, cpu->ss, (uint16_t)(cpu->sp + 2u * i));
}

static uint8_t *at(uint8_t *ram, uint16_t seg, uint16_t off)
{
    return &ram[physical(seg, off)];
}

static void store(uint8_t *ram, const pr_u25_1788_cpu *cpu)
{
    uint8_t *d = at(ram, arg(ram, cpu, 2), arg(ram, cpu, 1));
    const uint8_t *s = at(ram, arg(ram, cpu, 4), arg(ram, cpu, 3));
    unsigned n = s[0], cap = arg(ram, cpu, 0) & 0xFFu;
    if (n > cap) n = cap;
    memmove(&d[1], &s[1], n);
    d[0] = (uint8_t)n;
}

/* Synthetic runtime: the decoded SYSTEM effects, plus a recording U3B0. */
static int external(void *context, uint8_t *ram, size_t ram_size,
                    pr_u25_1788_cpu *cpu, unsigned call)
{
    fixture *f = context;
    static const char codes[] = "SXMDULCAs";
    char note[3] = {codes[call], ' ', '\0'};
    unsigned i;
    assert(ram_size == PR_U25_1788_RAM_SIZE && call < 9u);
    ++f->calls;
    if (f->calls == f->abort_at) return 0;
    strcat(f->log, note);
    cpu->flags = 0x0A53u;
    switch (call) {
    case PR_U25_1788_STRSTORE:
    case PR_U25_1788_156D_BASE + PR_U25_156D_STRSTORE:
        store(ram, cpu);
        if (call != PR_U25_1788_STRSTORE && f->redirect_156d)
            put16(ram, cpu->ss, (uint16_t)(cpu->bp + 2u), 0x1234u);
        break;
    case PR_U25_1788_U3B0_4:
        assert(cpu->ip == 0x1806u);
        for (i = 0u; i < 20u; ++i) f->forwarded[i] = arg(ram, cpu, i);
        cpu->ax = 0x6C5Au;
        cpu->flags = 0x0457u;
        break;
    case PR_U25_1788_156D_BASE + PR_U25_156D_IN_SET: {
        uint8_t ch = (uint8_t)arg(ram, cpu, 2);
        int member = ch >= 'A' && ch <= 'Z';
        cpu->flags = (uint16_t)((cpu->flags & ~0x0040u) | (member ? 0u : 0x0040u));
        break;
    }
    case PR_U25_1788_156D_BASE + PR_U25_156D_DELETE: {
        uint16_t index = arg(ram, cpu, 1);
        uint8_t *s = at(ram, arg(ram, cpu, 3), arg(ram, cpu, 2));
        if (index >= 1u && index <= s[0]) {
            memmove(&s[index], &s[index + 1u], (size_t)(s[0] - index));
            s[0] = (uint8_t)(s[0] - 1u);
        }
        break;
    }
    case PR_U25_1788_156D_BASE + PR_U25_156D_UPCASE: {
        uint8_t ch = (uint8_t)arg(ram, cpu, 0);
        if (ch >= 'a' && ch <= 'z') ch = (uint8_t)(ch - 0x20u);
        cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | ch);
        break;
    }
    case PR_U25_1788_156D_BASE + PR_U25_156D_STRLOAD: {
        uint8_t *d = at(ram, arg(ram, cpu, 3), arg(ram, cpu, 2));
        const uint8_t *s = at(ram, arg(ram, cpu, 1), arg(ram, cpu, 0));
        memmove(d, s, (size_t)s[0] + 1u);
        break;
    }
    case PR_U25_1788_156D_BASE + PR_U25_156D_CHARSTR: {
        uint8_t *d = at(ram, arg(ram, cpu, 2), arg(ram, cpu, 1));
        d[0] = 1u;
        d[1] = (uint8_t)arg(ram, cpu, 0);
        break;
    }
    case PR_U25_1788_156D_BASE + PR_U25_156D_CONCAT: {
        uint8_t *d = at(ram, arg(ram, cpu, 3), arg(ram, cpu, 2));
        const uint8_t *s = at(ram, arg(ram, cpu, 1), arg(ram, cpu, 0));
        memcpy(&d[d[0] + 1u], &s[1], s[0]);
        d[0] = (uint8_t)(d[0] + s[0]);
        break;
    }
    }
    return 1;
}

static uint8_t *prepare(pr_u25_1788_cpu *cpu, const char *name, const char *text)
{
    uint8_t *ram = calloc(PR_U25_1788_RAM_SIZE, 1u);
    unsigned i;
    assert(ram != NULL);
    memset(cpu, 0, sizeof *cpu);
    cpu->ax = 0xA5CDu; cpu->bp = 0x7777u; cpu->sp = SP;
    cpu->cs = CS; cpu->ip = 0x1788u; cpu->ds = DS; cpu->ss = SS;
    cpu->flags = 0x0202u;
    put16(ram, SS, SP, 0x1058u);
    put16(ram, SS, SP + 2u, 0xD000u);
    /* Argument words [BP+6]..[BP+0x2C]: distinct markers. */
    for (i = 0u; i < 20u; ++i)
        put16(ram, SS, (uint16_t)(SP + 4u + 2u * i), (uint16_t)(0x1100u * (i + 1u) + i));
    put16(ram, SS, SP + 0x28u, 0x0200u); put16(ram, SS, SP + 0x2Au, 0x2000u);
    put16(ram, SS, SP + 0x24u, 0x0300u); put16(ram, SS, SP + 0x26u, 0x2000u);
    *at(ram, 0x2000u, 0x0200u) = (uint8_t)strlen(name);
    memcpy(at(ram, 0x2000u, 0x0201u), name, strlen(name));
    *at(ram, 0x2000u, 0x0300u) = (uint8_t)strlen(text);
    memcpy(at(ram, 0x2000u, 0x0301u), text, strlen(text));
    memset(at(ram, SS, (uint16_t)(BP - 0x154u)), 0xEE, 0x154u);
    return ram;
}

static void test_forward(void)
{
    static const char name[] = "A fairly long name that exceeds forty chars";
    pr_u25_1788_cpu cpu;
    fixture f;
    uint8_t *ram = prepare(&cpu, name, "~aBC");
    unsigned i;
    memset(&f, 0, sizeof f);
    assert(pr_u25_entry14_1788(ram, PR_U25_1788_RAM_SIZE, &cpu, external, &f)
           == PR_U25_1788_OK);
    assert(strcmp(f.log, "S S M D U L C A s M M M X ") == 0);
    /* Local copies: string[40] of the first, filtered copy of the second. */
    assert(*at(ram, SS, (uint16_t)(BP - 0x29u)) == 40u);
    assert(memcmp(at(ram, SS, (uint16_t)(BP - 0x28u)), name, 40u) == 0);
    assert(*at(ram, SS, (uint16_t)(BP - 0x129u)) == 3u);
    assert(memcmp(at(ram, SS, (uint16_t)(BP - 0x128u)), "Abcc", 4u) == 0);
    assert(*at(ram, SS, (uint16_t)(BP - 0x153u)) == 1u);
    /* The caller's own strings are unchanged. */
    assert(memcmp(at(ram, 0x2000u, 0x0300u), "\x04~aBC", 5u) == 0);
    /* Twenty forwarded words: far args 6/A/E, byte 12, words 14/16,
     * bytes 18..24 (AH carried from the previous AX), then both locals. */
    assert(f.forwarded[0] == 0x1100u && f.forwarded[1] == 0x2201u);
    assert(f.forwarded[2] == 0x3302u && f.forwarded[5] == 0x6605u);
    assert((f.forwarded[6] & 0xFFu) == 0x06u);
    assert(f.forwarded[7] == 0x8807u && f.forwarded[8] == 0x9908u);
    for (i = 0u; i < 7u; ++i)
        assert((f.forwarded[9u + i] & 0xFFu) == (uint8_t)(0x1100u * (10u + i) + 9u + i));
    assert(f.forwarded[16] == (uint16_t)(BP - 0x129u) && f.forwarded[17] == SS);
    assert(f.forwarded[18] == (uint16_t)(BP - 0x29u) && f.forwarded[19] == SS);
    /* The callee's AL is stored to [bp-0x12A]; AX and flags pass through. */
    assert(*at(ram, SS, (uint16_t)(BP - 0x12Au)) == 0x5Au);
    assert(cpu.ax == 0x6C5Au && cpu.flags == 0x0457u);
    assert(cpu.sp == SP + 4u + 0x28u && cpu.bp == 0x7777u);
    assert(cpu.ip == 0x1058u && cpu.cs == 0xD000u);
    free(ram);
}

static void test_abort_transfer_arguments(void)
{
    static const uint16_t sites[] = {0x179Du, 0x17B1u, 0x15A9u, 0x1806u};
    static const unsigned aborts[] = {1u, 2u, 3u, 13u};
    pr_u25_1788_cpu cpu;
    fixture f;
    uint8_t *ram;
    unsigned i;
    for (i = 0u; i < 4u; ++i) {
        memset(&f, 0, sizeof f);
        f.abort_at = aborts[i];
        ram = prepare(&cpu, "n", "~aBC");
        assert(pr_u25_entry14_1788(ram, PR_U25_1788_RAM_SIZE, &cpu, external, &f)
               == PR_U25_1788_CALL_ABORT);
        assert(cpu.ip == sites[i]);
        free(ram);
    }
    memset(&f, 0, sizeof f);
    f.redirect_156d = 1;
    ram = prepare(&cpu, "n", "~a");
    assert(pr_u25_entry14_1788(ram, PR_U25_1788_RAM_SIZE, &cpu, external, &f)
           == PR_U25_1788_TRANSFER);
    assert(cpu.ip == 0x1234u && cpu.cs == CS && strchr(f.log, 'X') == NULL);
    free(ram);
    ram = prepare(&cpu, "n", "a");
    assert(pr_u25_entry14_1788(NULL, PR_U25_1788_RAM_SIZE, &cpu, external, &f)
           == PR_U25_1788_ARGUMENT);
    assert(pr_u25_entry14_1788(ram, PR_U25_1788_RAM_SIZE - 1u, &cpu, external, &f)
           == PR_U25_1788_ARGUMENT);
    assert(pr_u25_entry14_1788(ram, PR_U25_1788_RAM_SIZE, NULL, external, &f)
           == PR_U25_1788_ARGUMENT);
    assert(pr_u25_entry14_1788(ram, PR_U25_1788_RAM_SIZE, &cpu, NULL, &f)
           == PR_U25_1788_ARGUMENT);
    assert(cpu.sp == SP);
    free(ram);
}

int main(void)
{
    test_forward();
    test_abort_transfer_arguments();
    return 0;
}
