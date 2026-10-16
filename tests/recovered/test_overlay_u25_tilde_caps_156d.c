#include "poolrad/recovered/overlay_u25_tilde_caps_156d.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define DS 0x3000u
#define SS 0xE000u
#define SP 0x8000u
#define BP (SP - 2u)
#define CS 0xF000u
#define SRC_SEG 0x2000u
#define SRC_OFF 0x0100u
#define DST_SEG 0x2100u
#define DST_OFF 0x0040u

typedef struct {
    unsigned calls;
    unsigned abort_at;
    char log[1024];
} fixture;

static size_t physical(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & (PR_U25_156D_RAM_SIZE - 1u);
}

static void put16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t value)
{
    size_t p = physical(seg, off);
    ram[p] = (uint8_t)value;
    ram[(p + 1u) & (PR_U25_156D_RAM_SIZE - 1u)] = (uint8_t)(value >> 8);
}

static uint16_t get16(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    size_t p = physical(seg, off);
    return (uint16_t)(ram[p] |
        ((uint16_t)ram[(p + 1u) & (PR_U25_156D_RAM_SIZE - 1u)] << 8));
}

static uint16_t arg(const uint8_t *ram, const pr_u25_156d_cpu *cpu, unsigned i)
{
    return get16(ram, cpu->ss, (uint16_t)(cpu->sp + 2u * i));
}

static uint8_t *at(uint8_t *ram, uint16_t seg, uint16_t off)
{
    return &ram[physical(seg, off)];
}

/* Synthetic SYSTEM runtime with the effects decoded from START.EXE. */
static int external(void *context, uint8_t *ram, size_t ram_size,
                    pr_u25_156d_cpu *cpu, pr_u25_156d_call call)
{
    fixture *f = context;
    static const char codes[] = "MDULCAS";
    char note[3] = {codes[call], ' ', '\0'};
    assert(ram_size == PR_U25_156D_RAM_SIZE);
    ++f->calls;
    if (f->calls == f->abort_at) return 0;
    strcat(f->log, note);
    cpu->flags = 0x0A53u;
    switch (call) {
    case PR_U25_156D_IN_SET: {
        uint8_t ch = (uint8_t)arg(ram, cpu, 2);
        uint8_t bits = *at(ram, arg(ram, cpu, 1), (uint16_t)(arg(ram, cpu, 0) + (ch >> 3)));
        assert(arg(ram, cpu, 0) == 0x154Du && arg(ram, cpu, 1) == CS);
        cpu->flags = (uint16_t)((cpu->flags & ~0x0040u) |
                                ((bits >> (ch & 7u)) & 1u ? 0u : 0x0040u));
        break;
    }
    case PR_U25_156D_DELETE: {
        uint16_t count = arg(ram, cpu, 0), index = arg(ram, cpu, 1);
        uint8_t *s = at(ram, arg(ram, cpu, 3), arg(ram, cpu, 2));
        assert(count == 1u && index >= 1u);
        if (index <= s[0]) {
            memmove(&s[index], &s[index + 1u], (size_t)(s[0] - index));
            s[0] = (uint8_t)(s[0] - 1u);
        }
        break;
    }
    case PR_U25_156D_UPCASE: {
        uint8_t ch = (uint8_t)arg(ram, cpu, 0);
        if (ch >= 'a' && ch <= 'z') ch = (uint8_t)(ch - 0x20u);
        cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | ch);
        break;
    }
    case PR_U25_156D_STRLOAD: {
        uint8_t *d = at(ram, arg(ram, cpu, 3), arg(ram, cpu, 2));
        const uint8_t *s = at(ram, arg(ram, cpu, 1), arg(ram, cpu, 0));
        assert(arg(ram, cpu, 2) == (uint16_t)(BP - 0x202u));
        memmove(d, s, (size_t)s[0] + 1u);
        break;
    }
    case PR_U25_156D_CHARSTR: {
        uint8_t *d = at(ram, arg(ram, cpu, 2), arg(ram, cpu, 1));
        assert(arg(ram, cpu, 1) == (uint16_t)(BP - 0x102u));
        d[0] = 1u;
        d[1] = (uint8_t)arg(ram, cpu, 0);
        break;
    }
    case PR_U25_156D_CONCAT: {
        uint8_t *d = at(ram, arg(ram, cpu, 3), arg(ram, cpu, 2));
        const uint8_t *s = at(ram, arg(ram, cpu, 1), arg(ram, cpu, 0));
        unsigned take = s[0];
        if (take > 255u - d[0]) take = 255u - d[0];
        memcpy(&d[d[0] + 1u], &s[1], take);
        d[0] = (uint8_t)(d[0] + take);
        break;
    }
    case PR_U25_156D_STRSTORE: {
        uint8_t *d = at(ram, arg(ram, cpu, 2), arg(ram, cpu, 1));
        const uint8_t *s = at(ram, arg(ram, cpu, 4), arg(ram, cpu, 3));
        unsigned n = s[0];
        assert(arg(ram, cpu, 0) == 0x28u);
        if (n > 0x28u) n = 0x28u;
        memmove(&d[1], &s[1], n);
        d[0] = (uint8_t)n;
        break;
    }
    }
    return 1;
}

static void set_add(uint8_t set[32], uint8_t ch)
{
    set[ch >> 3] = (uint8_t)(set[ch >> 3] | (1u << (ch & 7u)));
}

static uint8_t *prepare(pr_u25_156d_cpu *cpu, const uint8_t *src, size_t n)
{
    uint8_t *ram = calloc(PR_U25_156D_RAM_SIZE, 1u);
    uint8_t set[32] = {0};
    unsigned c;
    assert(ram != NULL);
    memset(cpu, 0, sizeof *cpu);
    cpu->ax = 0xA5CDu; cpu->bp = 0x7777u; cpu->sp = SP; cpu->dx = 0x5555u;
    cpu->cs = CS; cpu->ip = 0x156Du; cpu->ds = DS; cpu->ss = SS;
    cpu->flags = 0x0202u;
    for (c = 'A'; c <= 'Z'; ++c) set_add(set, (uint8_t)c);
    memcpy(at(ram, CS, 0x154Du), set, sizeof set);
    put16(ram, SS, SP, 0x16C4u);
    put16(ram, SS, SP + 2u, CS);
    put16(ram, SS, SP + 4u, DST_OFF);
    put16(ram, SS, SP + 6u, DST_SEG);
    put16(ram, SS, SP + 8u, SRC_OFF);
    put16(ram, SS, SP + 10u, SRC_SEG);
    memset(at(ram, SRC_SEG, SRC_OFF), '.', 256u);
    *at(ram, SRC_SEG, SRC_OFF) = (uint8_t)n;
    memcpy(at(ram, SRC_SEG, SRC_OFF + 1u), src, n);
    *at(ram, DST_SEG, DST_OFF) = 0x55u;
    return ram;
}

/* Independent reference of the decoded loop over a 256-byte string. */
static void reference(uint8_t s[256], uint8_t d[256])
{
    unsigned len = s[0], i = 1u;
    d[0] = 0u;
    if (len < 1u) return;
    for (;;) {
        if (s[i] >= 'A' && s[i] <= 'Z') s[i] = (uint8_t)(s[i] + 0x20u);
        if (s[i] == '~') {
            uint8_t t[257], ch;
            unsigned n;
            if (i <= s[0]) {
                memmove(&s[i], &s[i + 1u], s[0] - i);
                s[0] = (uint8_t)(s[0] - 1u);
            }
            ch = s[i];
            if (ch >= 'a' && ch <= 'z') ch = (uint8_t)(ch - 0x20u);
            s[i] = ch;
            memcpy(t, d, (size_t)d[0] + 1u);
            if (t[0] < 255u) t[++t[0]] = ch;
            n = t[0] > 0x28u ? 0x28u : t[0];
            memmove(&d[1], &t[1], n);
            d[0] = (uint8_t)n;
        }
        if (i == len) break;
        i = (i + 1u) & 0xFFu;
    }
}

static pr_u25_156d_status run(uint8_t *ram, pr_u25_156d_cpu *cpu, fixture *f)
{
    return pr_u25_tilde_caps_156d(ram, PR_U25_156D_RAM_SIZE, cpu, external, f);
}

static void returned(const pr_u25_156d_cpu *cpu)
{
    assert(cpu->sp == SP + 12u && cpu->bp == 0x7777u);
    assert(cpu->ip == 0x16C4u && cpu->cs == CS);
}

static void test_example(void)
{
    static const char src[] = "~aBC ~dE~";
    pr_u25_156d_cpu cpu;
    fixture f;
    uint8_t s[256], d[256];
    uint8_t *ram = prepare(&cpu, (const uint8_t *)src, 9u);
    memset(&f, 0, sizeof f);
    memcpy(s, at(ram, SRC_SEG, SRC_OFF), sizeof s);
    reference(s, d);
    assert(run(ram, &cpu, &f) == PR_U25_156D_OK);
    returned(&cpu);
    /* '~' is removed and the next character upcased and appended; the
     * loop still runs to the original length 9, over stale tail bytes. */
    assert(memcmp(at(ram, SRC_SEG, SRC_OFF), s, sizeof s) == 0);
    assert(memcmp(at(ram, DST_SEG, DST_OFF), d, (size_t)d[0] + 1u) == 0);
    assert(d[0] >= 2u && d[1] == 'A' && d[2] == 'D');
    assert(*at(ram, SRC_SEG, SRC_OFF) < 9u);
    assert(memcmp(at(ram, SRC_SEG, SRC_OFF + 1u), "Abc De", 6u) == 0);
    /* The final CMP i,len is equal: ZF and PF set. */
    assert((cpu.flags & 0x08D5u) == 0x0044u);
    free(ram);
}

static void test_reference(void)
{
    static const char alphabet[] = "~~aZbYq ~.";
    uint32_t seed = 0x156Du;
    unsigned round;
    for (round = 0u; round < 300u; ++round) {
        uint8_t src[255], s[256], d[256];
        size_t n = round % 40u, i;
        pr_u25_156d_cpu cpu;
        fixture f;
        uint8_t *ram;
        for (i = 0u; i < n; ++i) {
            seed = seed * 1103515245u + 12345u;
            src[i] = (uint8_t)alphabet[(seed >> 16) % (sizeof alphabet - 1u)];
        }
        ram = prepare(&cpu, src, n);
        memcpy(s, at(ram, SRC_SEG, SRC_OFF), sizeof s);
        reference(s, d);
        memset(&f, 0, sizeof f);
        assert(run(ram, &cpu, &f) == PR_U25_156D_OK);
        returned(&cpu);
        assert(memcmp(at(ram, SRC_SEG, SRC_OFF), s, sizeof s) == 0);
        if (n == 0u) {
            assert(*at(ram, DST_SEG, DST_OFF) == 0u && f.calls == 0u);
            /* CMP 1,0 then JMP: result 1, all six flags clear. */
            assert((cpu.flags & 0x08D5u) == 0u);
        } else {
            assert(memcmp(at(ram, DST_SEG, DST_OFF), d, (size_t)d[0] + 1u) == 0);
        }
        free(ram);
    }
}

static void test_abort_and_arguments(void)
{
    static const char *const logs[] = {
        "", "M ", "M D ", "M D U ", "M D U L ", "M D U L C ", "M D U L C A "
    };
    static const uint16_t sites[] = {
        0x15A9u, 0x15F0u, 0x1603u, 0x1622u, 0x163Bu, 0x1640u, 0x164Eu
    };
    pr_u25_156d_cpu cpu;
    fixture f;
    uint8_t *ram;
    unsigned i;
    for (i = 0u; i < 7u; ++i) {
        memset(&f, 0, sizeof f);
        f.abort_at = i + 1u;
        ram = prepare(&cpu, (const uint8_t *)"~a", 2u);
        assert(run(ram, &cpu, &f) == PR_U25_156D_CALL_ABORT);
        assert(strcmp(f.log, logs[i]) == 0 && cpu.ip == sites[i]);
        free(ram);
    }
    ram = prepare(&cpu, (const uint8_t *)"a", 1u);
    assert(pr_u25_tilde_caps_156d(NULL, PR_U25_156D_RAM_SIZE, &cpu, external, &f)
           == PR_U25_156D_ARGUMENT);
    assert(pr_u25_tilde_caps_156d(ram, PR_U25_156D_RAM_SIZE - 1u, &cpu, external, &f)
           == PR_U25_156D_ARGUMENT);
    assert(pr_u25_tilde_caps_156d(ram, PR_U25_156D_RAM_SIZE, NULL, external, &f)
           == PR_U25_156D_ARGUMENT);
    assert(pr_u25_tilde_caps_156d(ram, PR_U25_156D_RAM_SIZE, &cpu, NULL, &f)
           == PR_U25_156D_ARGUMENT);
    assert(cpu.sp == SP && *at(ram, DST_SEG, DST_OFF) == 0x55u);
    free(ram);
}

int main(void)
{
    test_example();
    test_reference();
    test_abort_and_arguments();
    return 0;
}
