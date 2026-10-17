/* U3C6 +04BE CPU/RAM model: in/out-of-range lookups, live alias re-reads, the
 * pinned LES FFFD seam, code-alias stops, the declared RETF policy and argument
 * negatives, on synthetic state (no game bytes). */
#include "poolrad/recovered/overlay_u3c6_04be_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CSEG 0x2000u
#define DSEG 0x1000u
#define SSEG 0x3000u
#define OSEG 0x4000u
#define MSEG 0x5000u
#define RET_CS 0x0BADu
#define RET_IP 0x1234u
#define ENTRY_SP 0x7FF0u /* SP on the return IP; 12 argument bytes above */
#define A_OFF 0x0010u
#define B_OFF 0x0020u
#define R_OFF 0x0100u    /* far DS:[0x6716] = MSEG:R_OFF */

static uint8_t ram[PR_U3C6_04BE_CPU_RAM_SIZE], before[PR_U3C6_04BE_CPU_RAM_SIZE];
static unsigned failures, checks;

static size_t at(uint16_t seg, uint16_t off) { return (((size_t)seg << 4) + off) & (sizeof ram - 1u); }
static uint8_t r8(uint16_t seg, uint16_t off) { return ram[at(seg, off)]; }
static uint16_t r16(uint16_t seg, uint16_t off)
{
    return (uint16_t)(r8(seg, off) | ((uint16_t)ram[(at(seg, off) + 1u) & (sizeof ram - 1u)] << 8));
}
static void w8(uint16_t seg, uint16_t off, uint8_t v) { ram[at(seg, off)] = v; }
static void w16(uint16_t seg, uint16_t off, uint16_t v)
{
    ram[at(seg, off)] = (uint8_t)v;
    ram[(at(seg, off) + 1u) & (sizeof ram - 1u)] = (uint8_t)(v >> 8);
}
static void check(int ok, const char *what, unsigned line, unsigned tag)
{
    ++checks;
    if (!ok) {
        fprintf(stderr, "line %u (case %u): %s\n", line, tag, what);
        ++failures;
    }
}
#define CHECK(tag, e) check((e) != 0, #e, __LINE__, (tag))

static uint8_t map_byte(unsigned i) { return (uint8_t)(i * 7u + 3u); }
static uint8_t grid_byte(unsigned i) { return (uint8_t)(i * 11u + 5u); }

static uint16_t pf(uint8_t v)
{
    unsigned i, ones = 0;
    for (i = 0; i < 8; i++) ones += (v >> i) & 1u;
    return (ones % 2u == 0u) ? 0x0004u : 0u;
}
/* independent flag formulas: CMP r/m8,imm8 and ADD r16,r16 */
static uint16_t cmp8_flags(uint8_t a, uint8_t b)
{
    uint8_t r = (uint8_t)(a - b);
    uint16_t f = pf(r);
    if (a < b) f |= 0x0001u;
    if ((a ^ b ^ r) & 0x10u) f |= 0x0010u;
    if (r == 0u) f |= 0x0040u;
    if (r & 0x80u) f |= 0x0080u;
    if ((a ^ b) & (a ^ r) & 0x80u) f |= 0x0800u;
    return f;
}
static uint16_t add16_flags(uint16_t a, uint16_t b)
{
    uint32_t w = (uint32_t)a + b;
    uint16_t r = (uint16_t)w, f = pf((uint8_t)r);
    if (w > 0xFFFFu) f |= 0x0001u;
    if ((a ^ b ^ r) & 0x10u) f |= 0x0010u;
    if (r == 0u) f |= 0x0040u;
    if (r & 0x8000u) f |= 0x0080u;
    if ((a ^ r) & (b ^ r) & 0x8000u) f |= 0x0800u;
    return f;
}

static void fill(void)
{
    unsigned i;
    memset(ram, 0, sizeof ram);
    for (i = 0; i < 50u * 25u; i++) {
        w8(MSEG, (uint16_t)(R_OFF + 7u + i), map_byte(i));
        w8(DSEG, (uint16_t)(0x60DBu + i), grid_byte(i));
    }
    w16(DSEG, 0x6716u, R_OFF);
    w16(DSEG, 0x6718u, MSEG);
    w8(OSEG, A_OFF, 0x55u);
    w8(OSEG, B_OFF, 0x55u);
}

/* caller frame pushed word by word (byte-contiguous words, as PUSH does):
 * x, y, B (seg, off), A (seg, off), CS, IP; SS:SP = entry_sp on the IP */
static void enter_at(pr_u3c6_04be_cpu *c, uint16_t ss, uint16_t entry_sp, uint8_t x, uint8_t y,
                     uint16_t a_seg, uint16_t a_off, uint16_t b_seg, uint16_t b_off)
{
    const uint16_t words[8] = {(uint16_t)(0xA500u | x), (uint16_t)(0x5A00u | y), b_seg, b_off,
                               a_seg, a_off, RET_CS, RET_IP};
    uint16_t sp = (uint16_t)(entry_sp + 16u);
    unsigned i;
    for (i = 0; i < 8; i++) {
        sp = (uint16_t)(sp - 2u);
        w16(ss, sp, words[i]);
    }
    memset(c, 0, sizeof *c);
    c->ax = 0x1111u; c->bx = 0x2222u; c->cx = 0x3333u; c->dx = 0x4444u;
    c->si = 0x5555u; c->di = 0x6666u; c->bp = 0x7777u; c->sp = sp;
    c->cs = CSEG; c->ip = 0x04BEu; c->ds = DSEG; c->es = 0x9999u; c->ss = ss;
    c->flags = 0x0002u;
}
static void enter(pr_u3c6_04be_cpu *c, uint8_t x, uint8_t y)
{
    enter_at(c, SSEG, ENTRY_SP, x, y, OSEG, A_OFF, OSEG, B_OFF);
}

/* T1: in range (corners and middles), both synthetic RETF profiles */
static void in_range(void)
{
    static const uint8_t xs[3] = {0x00u, 0x31u, 0x10u}, ys[3] = {0x00u, 0x18u, 0x0Cu};
    unsigned ix, iy, p;
    for (p = 0; p < 2u; p++)
        for (ix = 0; ix < 3u; ix++)
            for (iy = 0; iy < 3u; iy++) {
                pr_u3c6_04be_cpu c;
                uint8_t x = xs[ix], y = ys[iy];
                unsigned i = 50u * y + x, tag = 100u + 10u * p + 3u * ix + iy;
                uint16_t prod = (uint16_t)(y * 0x32u);
                fill();
                enter(&c, x, y);
                CHECK(tag, pr_u3c6_04be_cpu_run(ram, sizeof ram, &c,
                                                p ? PR_U3C6_04BE_RETF_CONTIGUOUS_PHYSICAL : PR_U3C6_04BE_RETF_WRAPPED_SS) ==
                               PR_U3C6_04BE_CPU_RETURNED);
                CHECK(tag, r8(OSEG, A_OFF) == map_byte(i) && r8(OSEG, B_OFF) == grid_byte(i));
                CHECK(tag, c.cs == RET_CS && c.ip == RET_IP && c.sp == (uint16_t)(ENTRY_SP + 0x10u) && c.bp == 0x7777u);
                CHECK(tag, c.ax == (uint16_t)((prod & 0xFF00u) | grid_byte(i)) && c.cx == x && c.dx == 0u);
                CHECK(tag, c.es == OSEG && c.di == B_OFF && c.ss == SSEG && c.ds == DSEG);
                CHECK(tag, c.bx == 0x2222u && c.si == 0x5555u);
                CHECK(tag, c.flags == (uint16_t)(0x0002u | add16_flags(prod, x)));   /* +050B add di,cx */
                CHECK(tag, r16(SSEG, (uint16_t)(ENTRY_SP - 2u)) == 0x7777u);         /* saved BP */
            }
}

/* T2: out of range -> *B := 0 then *A := 0; AX/CX/DX unchanged; flags of the failing CMP */
static void out_of_range(void)
{
    static const struct { uint8_t x, y, cmp_val, cmp_imm; } cases[] = {
        {0xFFu, 0x05u, 0xFFu, 0x00u}, /* x = -1: jl at +04C5 */
        {0x32u, 0x05u, 0x32u, 0x31u}, /* x = 0x32: jg at +04CB */
        {0x05u, 0xFFu, 0xFFu, 0x00u}, /* y = -1: jl at +04D1 */
        {0x05u, 0x19u, 0x19u, 0x18u}, /* y = 0x19: jg at +04D7 */
        {0x80u, 0x05u, 0x80u, 0x00u}, /* x = 0x80 (-128) */
    };
    unsigned n;
    for (n = 0; n < sizeof cases / sizeof cases[0]; n++) {
        pr_u3c6_04be_cpu c;
        unsigned tag = 200u + n;
        fill();
        enter(&c, cases[n].x, cases[n].y);
        CHECK(tag, pr_u3c6_04be_cpu_run(ram, sizeof ram, &c, PR_U3C6_04BE_RETF_WRAPPED_SS) == PR_U3C6_04BE_CPU_RETURNED);
        CHECK(tag, r8(OSEG, A_OFF) == 0u && r8(OSEG, B_OFF) == 0u);
        CHECK(tag, c.ax == 0x1111u && c.cx == 0x3333u && c.dx == 0x4444u);
        CHECK(tag, c.es == OSEG && c.di == A_OFF);                              /* last LES +0520 */
        CHECK(tag, c.flags == (uint16_t)(0x0002u | cmp8_flags(cases[n].cmp_val, cases[n].cmp_imm)));
        CHECK(tag, c.cs == RET_CS && c.ip == RET_IP && c.sp == (uint16_t)(ENTRY_SP + 0x10u));
    }
}

/* T3: *A aliases the y byte [bp+0xE]; the second CBW/MUL reads the new value
 * (map byte 0x80 -> cbw 0xFF80 * 0x32 = DX:AX 0x0031:0xE700) */
static void alias_row(void)
{
    pr_u3c6_04be_cpu c;
    const uint8_t x = 0x05u, y = 0x02u;
    const unsigned i = 50u * y + x;
    const uint16_t bp = (uint16_t)(ENTRY_SP - 2u);
    fill();
    w8(MSEG, (uint16_t)(R_OFF + 7u + i), 0x80u);
    w8(DSEG, (uint16_t)(0x60DBu + 0xE700u + x), 0xC7u);                        /* 16-bit wrapped index */
    enter_at(&c, SSEG, ENTRY_SP, x, y, SSEG, (uint16_t)(bp + 0x0Eu), OSEG, B_OFF);
    CHECK(300u, pr_u3c6_04be_cpu_run(ram, sizeof ram, &c, PR_U3C6_04BE_RETF_WRAPPED_SS) == PR_U3C6_04BE_CPU_RETURNED);
    CHECK(300u, r8(SSEG, (uint16_t)(bp + 0x0Eu)) == 0x80u);                    /* the +04F7 store */
    CHECK(300u, c.dx == 0x0031u && r8(OSEG, B_OFF) == 0xC7u);
}

/* T4: *A aliases the low byte of B's offset word [bp+0xA]; +0511 LES re-reads it */
static void alias_pointer(void)
{
    pr_u3c6_04be_cpu c;
    const uint8_t x = 0x07u, y = 0x03u;
    const unsigned i = 50u * y + x;
    const uint16_t bp = (uint16_t)(ENTRY_SP - 2u);
    const uint16_t new_b = (uint16_t)((B_OFF & 0xFF00u) | map_byte(i));
    fill();
    enter_at(&c, SSEG, ENTRY_SP, x, y, SSEG, (uint16_t)(bp + 0x0Au), OSEG, B_OFF);
    CHECK(400u, pr_u3c6_04be_cpu_run(ram, sizeof ram, &c, PR_U3C6_04BE_RETF_WRAPPED_SS) == PR_U3C6_04BE_CPU_RETURNED);
    CHECK(400u, r8(OSEG, new_b) == grid_byte(i) && r8(OSEG, B_OFF) == 0x55u);
    CHECK(400u, c.di == new_b && c.es == OSEG);
}

/* T5: unmeasured LES seam (PM i16879). A stack operand [bp+6]/[bp+0xA] at
 * SS:FFFD..FFFF stops before that LES with earlier effects kept; the
 * neighbour at FFFC runs. bp = entry SP - 2. */
static void les_seam(void)
{
    static const struct { uint16_t entry_sp; uint8_t x; uint16_t stop_ip; int a_done, b_zero; } cases[] = {
        {0xFFF9u, 0x04u, 0x04F4u, 0, 0}, /* in range, [bp+6] = FFFD: stop at +04F4, nothing stored */
        {0xFFFAu, 0x04u, 0x04F4u, 0, 0}, /* [bp+6] = FFFE */
        {0xFFFBu, 0x04u, 0x04F4u, 0, 0}, /* [bp+6] = FFFF */
        {0xFFF5u, 0x04u, 0x0511u, 1, 0}, /* in range, [bp+0xA] = FFFD: *A stored, stop at +0511 */
        {0xFFF5u, 0x40u, 0x0519u, 0, 0}, /* out of range, [bp+0xA] = FFFD: stop at +0519, nothing stored */
        {0xFFF9u, 0x40u, 0x0520u, 0, 1}, /* out of range, [bp+6] = FFFD: *B := 0 kept, stop at +0520 */
        {0xFFF8u, 0x04u, 0, 1, 0},       /* neighbour: [bp+6] = FFFC, [bp+0xA] = 0000: runs */
    };
    unsigned n;
    for (n = 0; n < sizeof cases / sizeof cases[0]; n++) {
        pr_u3c6_04be_cpu c;
        pr_u3c6_04be_cpu_status st;
        unsigned tag = 500u + n;
        uint16_t bp = (uint16_t)(cases[n].entry_sp - 2u);
        fill();
        enter_at(&c, SSEG, cases[n].entry_sp, cases[n].x, 0x01u, OSEG, A_OFF, OSEG, B_OFF);
        st = pr_u3c6_04be_cpu_run(ram, sizeof ram, &c, PR_U3C6_04BE_RETF_WRAPPED_SS);
        if (cases[n].stop_ip == 0u) {
            CHECK(tag, st == PR_U3C6_04BE_CPU_RETURNED && c.sp == (uint16_t)(cases[n].entry_sp + 0x10u));
            CHECK(tag, r8(OSEG, A_OFF) == map_byte(50u + cases[n].x) && r8(OSEG, B_OFF) == grid_byte(50u + cases[n].x));
            continue;
        }
        CHECK(tag, st == PR_U3C6_04BE_CPU_UNSUPPORTED_LES_SEAM && c.cs == CSEG && c.ip == cases[n].stop_ip);
        CHECK(tag, c.bp == bp && c.sp == bp);                                     /* after push bp; no locals */
        CHECK(tag, r8(OSEG, A_OFF) == (cases[n].a_done ? map_byte(50u + cases[n].x) : 0x55u));
        CHECK(tag, r8(OSEG, B_OFF) == (cases[n].b_zero ? 0u : 0x55u));
    }
}

/* SP edge (declared synthetic word rule): entry SP 0x0001 puts the BP push at
 * SS:FFFF with its high byte at linear SS*16 + 0x10000; POP BP reads the same
 * bytes back, and the frame words above wrap the 16-bit offset normally */
static void sp_edge(void)
{
    pr_u3c6_04be_cpu c;
    const uint8_t x = 0x05u, y = 0x03u;
    fill();
    enter_at(&c, SSEG, 0x0001u, x, y, OSEG, A_OFF, OSEG, B_OFF);
    CHECK(550u, pr_u3c6_04be_cpu_run(ram, sizeof ram, &c, PR_U3C6_04BE_RETF_WRAPPED_SS) == PR_U3C6_04BE_CPU_RETURNED);
    CHECK(550u, r8(SSEG, 0xFFFFu) == 0x77u && r8((uint16_t)(SSEG + 0x1000u), 0x0000u) == 0x77u);
    CHECK(550u, c.bp == 0x7777u && c.sp == 0x0011u && c.cs == RET_CS && c.ip == RET_IP);
    CHECK(550u, r8(OSEG, A_OFF) == map_byte(50u * y + x) && r8(OSEG, B_OFF) == grid_byte(50u * y + x));
}

/* T7: stores into the loaded body stop before the instruction; neighbours run */
static void code_alias(void)
{
    static const struct { uint8_t x; int on_b; uint16_t off, stop_ip; } cases[] = {
        {0x05u, 0, 0x04BEu, 0x04F7u}, {0x05u, 0, 0x052Au, 0x04F7u}, {0x05u, 0, 0x052Bu, 0},
        {0x05u, 0, 0x04BDu, 0},       {0x05u, 1, 0x04F0u, 0x0514u}, {0x40u, 1, 0x0500u, 0x051Cu},
        {0x40u, 0, 0x0500u, 0x0523u},
    };
    unsigned n;
    for (n = 0; n < sizeof cases / sizeof cases[0]; n++) {
        pr_u3c6_04be_cpu c;
        pr_u3c6_04be_cpu_status st;
        unsigned tag = 700u + n;
        fill();
        w8(CSEG, cases[n].off, 0xCCu);
        if (cases[n].on_b) enter_at(&c, SSEG, ENTRY_SP, cases[n].x, 0x03u, OSEG, A_OFF, CSEG, cases[n].off);
        else enter_at(&c, SSEG, ENTRY_SP, cases[n].x, 0x03u, CSEG, cases[n].off, OSEG, B_OFF);
        st = pr_u3c6_04be_cpu_run(ram, sizeof ram, &c, PR_U3C6_04BE_RETF_WRAPPED_SS);
        if (cases[n].stop_ip == 0u) {
            CHECK(tag, st == PR_U3C6_04BE_CPU_RETURNED);
            continue;
        }
        CHECK(tag, st == PR_U3C6_04BE_CPU_UNSCREENED_ALIAS && c.ip == cases[n].stop_ip && c.cs == CSEG);
        CHECK(tag, r8(CSEG, cases[n].off) == 0xCCu);
        if (cases[n].stop_ip == 0x0514u) CHECK(tag, r8(OSEG, A_OFF) != 0x55u);  /* +04F7 done */
        if (cases[n].stop_ip == 0x0523u) CHECK(tag, r8(OSEG, B_OFF) == 0u);     /* +051C done */
    }
}

/* T8: the BP push aliasing the body stops at +04BE with nothing changed */
static void push_alias(void)
{
    static const struct { uint16_t entry_sp; int stop; } cases[] = {
        {0x04C0u, 1}, {0x04BFu, 1}, {0x052Du, 0},
    };
    unsigned n;
    for (n = 0; n < sizeof cases / sizeof cases[0]; n++) {
        pr_u3c6_04be_cpu c, keep;
        pr_u3c6_04be_cpu_status st;
        unsigned tag = 800u + n;
        fill();
        enter_at(&c, CSEG, cases[n].entry_sp, 0x05u, 0x03u, OSEG, A_OFF, OSEG, B_OFF);
        keep = c;
        memcpy(before, ram, sizeof ram);
        st = pr_u3c6_04be_cpu_run(ram, sizeof ram, &c, PR_U3C6_04BE_RETF_WRAPPED_SS);
        if (cases[n].stop) {
            CHECK(tag, st == PR_U3C6_04BE_CPU_UNSCREENED_ALIAS);
            CHECK(tag, memcmp(&c, &keep, sizeof c) == 0 && memcmp(ram, before, sizeof ram) == 0);
        } else {
            CHECK(tag, st == PR_U3C6_04BE_CPU_RETURNED && c.sp == (uint16_t)(cases[n].entry_sp + 0x10u));
        }
    }
}

/* T9: UNMEASURED fails closed at +0528 after POP BP, both stores kept */
static void unmeasured(void)
{
    pr_u3c6_04be_cpu c;
    const uint8_t x = 0x06u, y = 0x02u;
    const unsigned i = 50u * y + x;
    fill();
    enter(&c, x, y);
    CHECK(900u, pr_u3c6_04be_cpu_run(ram, sizeof ram, &c, PR_U3C6_04BE_RETF_UNMEASURED) ==
                    PR_U3C6_04BE_CPU_RETURN_PENDING);
    CHECK(900u, c.cs == CSEG && c.ip == 0x0528u && c.sp == ENTRY_SP && c.bp == 0x7777u);
    CHECK(900u, r16(SSEG, ENTRY_SP) == RET_IP && r16(SSEG, (uint16_t)(ENTRY_SP + 2u)) == RET_CS);
    CHECK(900u, r8(OSEG, A_OFF) == map_byte(i) && r8(OSEG, B_OFF) == grid_byte(i));
}

/* T10: return frame at SS:FFFD/FFFE/FFFF; decoy at linear SS*16 + 0x10000 (FFFE)
 * or + 0x10001 (FFFF), clear of the contiguous IP byte */
static void retf_wrap(void)
{
    static const uint16_t sps[3] = {0xFFFDu, 0xFFFEu, 0xFFFFu};
    unsigned k, p;
    for (k = 0; k < 3u; k++)
        for (p = 0; p < 2u; p++) {
            pr_u3c6_04be_cpu c;
            unsigned tag = 1000u + 10u * k + p;
            uint16_t want_cs = RET_CS;
            fill();
            enter_at(&c, SSEG, sps[k], 0x05u, 0x03u, OSEG, A_OFF, OSEG, B_OFF);
            if (k == 1u) w16((uint16_t)(SSEG + 0x1000u), 0x0000u, 0xBEEFu);
            if (k == 2u) w16((uint16_t)(SSEG + 0x1000u), 0x0001u, 0xBEEFu);
            if (p == 1u && k != 0u) want_cs = 0xBEEFu;
            CHECK(tag, pr_u3c6_04be_cpu_run(ram, sizeof ram, &c,
                                            p ? PR_U3C6_04BE_RETF_CONTIGUOUS_PHYSICAL : PR_U3C6_04BE_RETF_WRAPPED_SS) ==
                           PR_U3C6_04BE_CPU_RETURNED);
            CHECK(tag, c.cs == want_cs && c.ip == RET_IP && c.sp == (uint16_t)(sps[k] + 0x10u));
            CHECK(tag, r8(OSEG, A_OFF) == map_byte(50u * 3u + 5u) && r8(OSEG, B_OFF) == grid_byte(50u * 3u + 5u));
        }
}

/* T11: argument negatives leave CPU and RAM unchanged */
static void arguments(void)
{
    pr_u3c6_04be_cpu c, keep;
    fill();
    enter(&c, 0x05u, 0x03u);
    keep = c;
    memcpy(before, ram, sizeof ram);
    CHECK(1100u, pr_u3c6_04be_cpu_run(NULL, sizeof ram, &c, PR_U3C6_04BE_RETF_WRAPPED_SS) == PR_U3C6_04BE_CPU_ARGUMENT);
    CHECK(1101u, pr_u3c6_04be_cpu_run(ram, sizeof ram - 1u, &c, PR_U3C6_04BE_RETF_WRAPPED_SS) == PR_U3C6_04BE_CPU_ARGUMENT);
    CHECK(1102u, pr_u3c6_04be_cpu_run(ram, sizeof ram, NULL, PR_U3C6_04BE_RETF_WRAPPED_SS) == PR_U3C6_04BE_CPU_ARGUMENT);
    CHECK(1103u, pr_u3c6_04be_cpu_run(ram, sizeof ram, &c, (pr_u3c6_04be_retf_policy)3) == PR_U3C6_04BE_CPU_ARGUMENT);
    c.ip = 0x04BFu;
    CHECK(1104u, pr_u3c6_04be_cpu_run(ram, sizeof ram, &c, PR_U3C6_04BE_RETF_WRAPPED_SS) == PR_U3C6_04BE_CPU_ARGUMENT);
    c.ip = keep.ip;
    CHECK(1105u, memcmp(&c, &keep, sizeof c) == 0 && memcmp(ram, before, sizeof ram) == 0);
}

int main(void)
{
    in_range();
    out_of_range();
    alias_row();
    alias_pointer();
    les_seam();
    sp_edge();
    code_alias();
    push_alias();
    unmeasured();
    retf_wrap();
    arguments();
    if (failures != 0u) {
        fprintf(stderr, "%u of %u checks failed\n", failures, checks);
        return 1;
    }
    printf("u3c6 +04BE cpu: %u checks passed\n", checks);
    return 0;
}
