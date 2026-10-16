#include "poolrad/recovered/overlay_u25_entry13_1684.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define DS 0x3000u
#define SS 0xE000u
#define SP 0x8000u
#define BP (SP - 2u)
#define CS 0xF000u

typedef struct {
    unsigned calls, abort_at, key_calls;
    int redirect_156d;
    const uint8_t *keys, *flags;
    uint16_t u3b0_args[11];
    char log[256];
} fixture;

static size_t physical(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & (PR_U25_1684_RAM_SIZE - 1u);
}

static void put16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t value)
{
    size_t p = physical(seg, off);
    ram[p] = (uint8_t)value;
    ram[(p + 1u) & (PR_U25_1684_RAM_SIZE - 1u)] = (uint8_t)(value >> 8);
}

static uint16_t get16(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    size_t p = physical(seg, off);
    return (uint16_t)(ram[p] |
        ((uint16_t)ram[(p + 1u) & (PR_U25_1684_RAM_SIZE - 1u)] << 8));
}

static uint16_t arg(const uint8_t *ram, const pr_u25_1684_cpu *cpu, unsigned i)
{
    return get16(ram, cpu->ss, (uint16_t)(cpu->sp + 2u * i));
}

static uint8_t *at(uint8_t *ram, uint16_t seg, uint16_t off)
{
    return &ram[physical(seg, off)];
}

static void store(uint8_t *ram, const pr_u25_1684_cpu *cpu)
{
    uint8_t *d = at(ram, arg(ram, cpu, 2), arg(ram, cpu, 1));
    const uint8_t *s = at(ram, arg(ram, cpu, 4), arg(ram, cpu, 3));
    unsigned n = s[0], cap = arg(ram, cpu, 0) & 0xFFu;
    if (n > cap) n = cap;
    memmove(&d[1], &s[1], n);
    d[0] = (uint8_t)n;
}

/* Set membership from the 32-byte set at the pushed far pointer. */
static void in_set(uint8_t *ram, pr_u25_1684_cpu *cpu)
{
    uint8_t ch = (uint8_t)arg(ram, cpu, 2);
    uint8_t bits = *at(ram, arg(ram, cpu, 1), (uint16_t)(arg(ram, cpu, 0) + (ch >> 3)));
    int member = (bits >> (ch & 7u)) & 1u;
    cpu->flags = (uint16_t)((cpu->flags & ~0x0040u) | (member ? 0u : 0x0040u));
}

static int external(void *context, uint8_t *ram, size_t ram_size,
                    pr_u25_1684_cpu *cpu, unsigned call)
{
    fixture *f = context;
    static const char codes[] = "SKPRT" "MDULCAs";
    char note[3] = {codes[call], ' ', '\0'};
    unsigned i;
    assert(ram_size == PR_U25_1684_RAM_SIZE && call < 12u);
    ++f->calls;
    if (f->calls == f->abort_at) return 0;
    strcat(f->log, note);
    if (call == PR_U25_1684_U77_10)
        assert((cpu->flags & 0x0040u) == 0u); /* CMP [bp-0x156],0 nonzero */
    cpu->flags = 0x0A53u;
    switch (call) {
    case PR_U25_1684_STRSTORE:
    case PR_U25_1684_156D_BASE + PR_U25_156D_STRSTORE:
        store(ram, cpu);
        if (call != PR_U25_1684_STRSTORE && f->redirect_156d)
            put16(ram, cpu->ss, (uint16_t)(cpu->bp + 2u), 0x1234u);
        break;
    case PR_U25_1684_U3B0_2: {
        unsigned n = f->key_calls++;
        assert(cpu->ip == 0x16E6u);
        for (i = 0u; i < 11u; ++i) f->u3b0_args[i] = arg(ram, cpu, i);
        *at(ram, arg(ram, cpu, 1), arg(ram, cpu, 0)) = f->flags[n];
        cpu->ax = (uint16_t)(0x6600u | f->keys[n]);
        break;
    }
    case PR_U25_1684_U77_10:
        assert(cpu->ip == 0x16FBu);
        break;
    case PR_U25_1684_LOAD_PIECES:
        assert(arg(ram, cpu, 0) == 0x4321u && arg(ram, cpu, 1) == 0x8765u);
        break;
    case PR_U25_1684_IN_SET:
        assert(arg(ram, cpu, 0) == 0x1664u && arg(ram, cpu, 1) == CS);
        in_set(ram, cpu);
        break;
    case PR_U25_1684_156D_BASE + PR_U25_156D_IN_SET:
        assert(arg(ram, cpu, 0) == 0x154Du);
        in_set(ram, cpu);
        break;
    case PR_U25_1684_156D_BASE + PR_U25_156D_DELETE: {
        uint16_t index = arg(ram, cpu, 1);
        uint8_t *s = at(ram, arg(ram, cpu, 3), arg(ram, cpu, 2));
        if (index >= 1u && index <= s[0]) {
            memmove(&s[index], &s[index + 1u], (size_t)(s[0] - index));
            s[0] = (uint8_t)(s[0] - 1u);
        }
        break;
    }
    case PR_U25_1684_156D_BASE + PR_U25_156D_UPCASE: {
        uint8_t ch = (uint8_t)arg(ram, cpu, 0);
        if (ch >= 'a' && ch <= 'z') ch = (uint8_t)(ch - 0x20u);
        cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | ch);
        break;
    }
    case PR_U25_1684_156D_BASE + PR_U25_156D_STRLOAD: {
        uint8_t *d = at(ram, arg(ram, cpu, 3), arg(ram, cpu, 2));
        const uint8_t *s = at(ram, arg(ram, cpu, 1), arg(ram, cpu, 0));
        memmove(d, s, (size_t)s[0] + 1u);
        break;
    }
    case PR_U25_1684_156D_BASE + PR_U25_156D_CHARSTR: {
        uint8_t *d = at(ram, arg(ram, cpu, 2), arg(ram, cpu, 1));
        d[0] = 1u;
        d[1] = (uint8_t)arg(ram, cpu, 0);
        break;
    }
    case PR_U25_1684_156D_BASE + PR_U25_156D_CONCAT: {
        uint8_t *d = at(ram, arg(ram, cpu, 3), arg(ram, cpu, 2));
        const uint8_t *s = at(ram, arg(ram, cpu, 1), arg(ram, cpu, 0));
        memcpy(&d[d[0] + 1u], &s[1], s[0]);
        d[0] = (uint8_t)(d[0] + s[0]);
        break;
    }
    }
    return 1;
}

static void set_add(uint8_t *set, uint8_t ch)
{
    set[ch >> 3] = (uint8_t)(set[ch >> 3] | (1u << (ch & 7u)));
}

static uint8_t *prepare(pr_u25_1684_cpu *cpu, const char *text, uint8_t enter)
{
    uint8_t *ram = calloc(PR_U25_1684_RAM_SIZE, 1u);
    unsigned c;
    assert(ram != NULL);
    memset(cpu, 0, sizeof *cpu);
    cpu->ax = 0xA5CDu; cpu->bp = 0x7777u; cpu->sp = SP;
    cpu->cs = CS; cpu->ip = 0x1684u; cpu->ds = DS; cpu->ss = SS;
    cpu->flags = 0x0202u;
    for (c = 'A'; c <= 'Z'; ++c) set_add(at(ram, CS, 0x154Du), (uint8_t)c);
    set_add(at(ram, CS, 0x1664u), 'Y');
    set_add(at(ram, CS, 0x1664u), 'N');
    set_add(at(ram, CS, 0x1664u), 'Q');
    put16(ram, SS, SP, 0x1236u);
    put16(ram, SS, SP + 2u, 0xD000u);
    put16(ram, SS, SP + 4u, 0x1101u);           /* [bp+6]  a */
    put16(ram, SS, SP + 6u, (uint16_t)(0x2200u | enter)); /* [bp+8] */
    put16(ram, SS, SP + 8u, 0x3303u);           /* [bp+0xA] c */
    put16(ram, SS, SP + 10u, 0x4404u);          /* [bp+0xC] d */
    put16(ram, SS, SP + 12u, 0x5505u);          /* [bp+0xE] e */
    put16(ram, SS, SP + 14u, 0x0300u);          /* [bp+0x10] S2 */
    put16(ram, SS, SP + 16u, 0x2000u);
    put16(ram, SS, SP + 18u, 0x0200u);          /* [bp+0x14] S1 */
    put16(ram, SS, SP + 20u, 0x2000u);
    memcpy(at(ram, 0x2000u, 0x0200u), "\x05Title", 6u);
    *at(ram, 0x2000u, 0x0300u) = (uint8_t)strlen(text);
    memcpy(at(ram, 0x2000u, 0x0301u), text, strlen(text));
    put16(ram, DS, 0x5D92u, 0x4321u);
    put16(ram, DS, 0x5D94u, 0x8765u);
    return ram;
}

static pr_u25_1684_status run(uint8_t *ram, pr_u25_1684_cpu *cpu, fixture *f)
{
    return pr_u25_entry13_1684(ram, PR_U25_1684_RAM_SIZE, cpu, external, f);
}

static void returned(const pr_u25_1684_cpu *cpu, uint8_t al, uint16_t flags)
{
    assert((cpu->ax & 0xFFu) == al && (cpu->flags & 0x08D5u) == flags);
    assert(cpu->sp == SP + 4u + 0x12u && cpu->bp == 0x7777u);
    assert(cpu->ip == 0x1236u && cpu->cs == 0xD000u);
}

static void test_hotkeys(void)
{
    static const uint8_t keys1[] = {'x', 'N'}, zero[4] = {0};
    static const uint8_t keys2[] = {'Y'}, keys3[] = {'Q'};
    pr_u25_1684_cpu cpu;
    fixture f;
    uint8_t *ram = prepare(&cpu, "~Yes ~No", 0u);
    memset(&f, 0, sizeof f); f.keys = keys1; f.flags = zero;
    assert(run(ram, &cpu, &f) == PR_U25_1684_OK);
    /* Hotkeys "YN"; 'x' loops, 'N' is the second hotkey: DEC 2 -> 1. */
    assert(*at(ram, SS, (uint16_t)(BP - 0x153u)) == 2u);
    assert(memcmp(at(ram, SS, (uint16_t)(BP - 0x152u)), "YN", 2u) == 0);
    assert(f.key_calls == 2u);
    returned(&cpu, 1u, 0u);
    /* U3B0 sees eleven words: far flag cell, a, 1, c, d, e, two locals. */
    assert(f.u3b0_args[0] == (uint16_t)(BP - 0x156u) && f.u3b0_args[1] == SS);
    assert((f.u3b0_args[2] & 0xFFu) == 0x01u && (f.u3b0_args[3] & 0xFFu) == 1u);
    assert((f.u3b0_args[4] & 0xFFu) == 0x03u && (f.u3b0_args[5] & 0xFFu) == 0x04u);
    assert((f.u3b0_args[6] & 0xFFu) == 0x05u);
    assert(f.u3b0_args[7] == (uint16_t)(BP - 0x12Au) && f.u3b0_args[9] == (uint16_t)(BP - 0x2Au));
    free(ram);

    ram = prepare(&cpu, "~Yes ~No", 0u);
    memset(&f, 0, sizeof f); f.keys = keys2; f.flags = zero;
    assert(run(ram, &cpu, &f) == PR_U25_1684_OK);
    returned(&cpu, 0u, 0x0044u); /* DEC 1 -> 0: ZF PF */
    free(ram);

    ram = prepare(&cpu, "~Yes ~No", 0u);
    memset(&f, 0, sizeof f); f.keys = keys3; f.flags = zero;
    assert(run(ram, &cpu, &f) == PR_U25_1684_OK);
    returned(&cpu, 0xFFu, 0u);   /* in the set, not a hotkey: CMP 3,2 */
    free(ram);
}

static void test_enter_and_flag(void)
{
    static const uint8_t keys[] = {0x0Du, 'Y'}, zero[2] = {0};
    static const uint8_t keys2[] = {'N', 0x0Du}, flags2[] = {1u, 0u};
    pr_u25_1684_cpu cpu;
    fixture f;
    uint8_t *ram = prepare(&cpu, "~Yes ~No", 1u);
    memset(&f, 0, sizeof f); f.keys = keys; f.flags = zero;
    assert(run(ram, &cpu, &f) == PR_U25_1684_OK && f.key_calls == 1u);
    returned(&cpu, 0u, 0x0044u); /* Enter accepted: CMP 0Dh,0Dh */
    free(ram);

    ram = prepare(&cpu, "~Yes ~No", 0u);
    memset(&f, 0, sizeof f); f.keys = keys; f.flags = zero;
    assert(run(ram, &cpu, &f) == PR_U25_1684_OK && f.key_calls == 2u);
    returned(&cpu, 0u, 0x0044u); /* Enter ignored, then 'Y' */
    free(ram);

    /* A set flag byte runs U77 and load-pieces and clears the key. */
    ram = prepare(&cpu, "~Yes ~No", 1u);
    memset(&f, 0, sizeof f); f.keys = keys2; f.flags = flags2;
    assert(run(ram, &cpu, &f) == PR_U25_1684_OK && f.key_calls == 2u);
    assert(strstr(f.log, "K P R T K T ") != NULL);
    returned(&cpu, 0u, 0x0044u);
    free(ram);
}

static void test_abort_transfer_arguments(void)
{
    static const uint8_t keys[] = {'N'}, flags1[] = {1u};
    pr_u25_1684_cpu cpu;
    fixture f;
    uint8_t *ram;
    unsigned i;
    for (i = 1u; i <= 15u; ++i) {
        memset(&f, 0, sizeof f); f.keys = keys; f.flags = flags1; f.abort_at = i;
        ram = prepare(&cpu, "~Yes", 0u);
        assert(run(ram, &cpu, &f) == PR_U25_1684_CALL_ABORT);
        free(ram);
    }
    memset(&f, 0, sizeof f); f.keys = keys; f.flags = flags1; f.redirect_156d = 1;
    ram = prepare(&cpu, "~N", 0u);
    assert(run(ram, &cpu, &f) == PR_U25_1684_TRANSFER);
    assert(cpu.ip == 0x1234u && f.key_calls == 0u);
    free(ram);
    ram = prepare(&cpu, "a", 0u);
    assert(pr_u25_entry13_1684(NULL, PR_U25_1684_RAM_SIZE, &cpu, external, &f)
           == PR_U25_1684_ARGUMENT);
    assert(pr_u25_entry13_1684(ram, PR_U25_1684_RAM_SIZE - 1u, &cpu, external, &f)
           == PR_U25_1684_ARGUMENT);
    assert(pr_u25_entry13_1684(ram, PR_U25_1684_RAM_SIZE, NULL, external, &f)
           == PR_U25_1684_ARGUMENT);
    assert(pr_u25_entry13_1684(ram, PR_U25_1684_RAM_SIZE, &cpu, NULL, &f)
           == PR_U25_1684_ARGUMENT);
    assert(cpu.sp == SP);
    free(ram);
}

int main(void)
{
    test_hotkeys();
    test_enter_and_flag();
    test_abort_transfer_arguments();
    return 0;
}
