/* START.EXE resident 0709:0768, the text-window printer, raw CPU (docs/re/RES_0709_PRINTER_0768_CPU.md). */
#include "poolrad/recovered/resident_0709_printer_0768_cpu.h"
#include "poolrad/recovered/resident_0709_clear_0dd3_cpu.h"
#include "poolrad/recovered/resident_0709_skip_spaces_0714_cpu.h"
#include "poolrad/recovered/resident_0709_text_06a7_cpu.h"
#include "poolrad/recovered/resident_0899_idle_0104_cpu.h"
#include "poolrad/recovered/system_runtime_leaves_cpu.h"
#include "poolrad/recovered/system_set_expand_0d12_cpu.h"
#include "poolrad/recovered/system_string_leaves_cpu.h"
#include "poolrad/recovered/system_strstore_fullstate_0b25.h"

#define MASK (PR_BUS_RAM_SIZE - 1u)
#define CF 0x0001u
#define PF 0x0004u
#define AF 0x0010u
#define ZF 0x0040u
#define SF 0x0080u
#define OF 0x0800u
#define ARITH (CF | PF | AF | ZF | SF | OF)
#define CONTINUE (-1)
#define COPY_CPU(d, s) do { (d)->ax = (s)->ax; (d)->bx = (s)->bx; (d)->cx = (s)->cx; (d)->dx = (s)->dx; \
    (d)->si = (s)->si; (d)->di = (s)->di; (d)->bp = (s)->bp; (d)->sp = (s)->sp; (d)->cs = (s)->cs; (d)->ip = (s)->ip; \
    (d)->ds = (s)->ds; (d)->es = (s)->es; (d)->ss = (s)->ss; (d)->flags = (s)->flags; } while (0)

typedef pr_r0709_printer_0768_regs cpu_t;

static size_t physical(uint16_t seg, uint16_t off) { return (((size_t)seg << 4u) + off) & MASK; }
static uint8_t rd8(pr_bus *b, uint16_t seg, uint16_t off) { return pr_bus_read8(b, physical(seg, off)); }
static void wr8(pr_bus *b, uint16_t seg, uint16_t off, uint8_t v) { pr_bus_write8(b, physical(seg, off), v); }
static uint16_t rd16(pr_bus *b, uint16_t seg, uint16_t off)
{
    size_t at = physical(seg, off);
    return (uint16_t)(pr_bus_read8(b, at) | ((uint16_t)pr_bus_read8(b, (at + 1u) & MASK) << 8u));
}
static void wr16(pr_bus *b, uint16_t seg, uint16_t off, uint16_t v)
{
    size_t at = physical(seg, off);
    pr_bus_write8(b, at, (uint8_t)v); pr_bus_write8(b, (at + 1u) & MASK, (uint8_t)(v >> 8u));
}
static void push(pr_bus *b, cpu_t *c, uint16_t v) { c->sp = (uint16_t)(c->sp - 2u); wr16(b, c->ss, c->sp, v); }
static uint16_t pop(pr_bus *b, cpu_t *c)
{
    uint16_t v = rd16(b, c->ss, c->sp);
    c->sp = (uint16_t)(c->sp + 2u);
    return v;
}
static unsigned parity(unsigned v)
{
    v &= 0xFFu; v ^= v >> 4u; v ^= v >> 2u; v ^= v >> 1u;
    return (v & 1u) ? 0u : PF;
}
/* SUB/CMP of width bits; all six arithmetic flags. Returns the result. */
static unsigned sub_flags(cpu_t *c, unsigned a, unsigned b, unsigned bits)
{
    unsigned mask = (1u << bits) - 1u, top = 1u << (bits - 1u), r = (a - b) & mask;
    c->flags = (uint16_t)((c->flags & ~ARITH) | (r == 0u ? ZF : 0u) | (r & top ? SF : 0u) | parity(r) | (a < b ? CF : 0u) |
                          ((a ^ b ^ r) & AF) | (((a ^ b) & (a ^ r) & top) ? OF : 0u));
    return r;
}
static unsigned add_flags(cpu_t *c, unsigned a, unsigned b, unsigned bits)
{
    unsigned mask = (1u << bits) - 1u, top = 1u << (bits - 1u), full = a + b, r = full & mask;
    c->flags = (uint16_t)((c->flags & ~ARITH) | (r == 0u ? ZF : 0u) | (r & top ? SF : 0u) | parity(r) | (full > mask ? CF : 0u) |
                          ((a ^ b ^ r) & AF) | ((~(a ^ b) & (a ^ r) & top) ? OF : 0u));
    return r;
}
/* INC / DEC: as ADD / SUB 1 with CF kept. */
static unsigned inc_flags(cpu_t *c, unsigned a, unsigned bits)
{
    uint16_t keep = (uint16_t)(c->flags & CF);
    unsigned r = add_flags(c, a, 1u, bits);
    c->flags = (uint16_t)((c->flags & ~CF) | keep);
    return r;
}
static unsigned dec_flags(cpu_t *c, unsigned a, unsigned bits)
{
    uint16_t keep = (uint16_t)(c->flags & CF);
    unsigned r = sub_flags(c, a, 1u, bits);
    c->flags = (uint16_t)((c->flags & ~CF) | keep);
    return r;
}
static void xor_ah(cpu_t *c)                                          /* XOR AH,AH: CF = OF = AF = 0, ZF = PF = 1, SF = 0 */
{
    c->ax &= 0x00FFu;
    c->flags = (uint16_t)((c->flags & ~ARITH) | ZF | PF);
}
static void set_al(cpu_t *c, uint8_t v) { c->ax = (uint16_t)((c->ax & 0xFF00u) | v); }
static uint8_t bpb(pr_bus *b, const cpu_t *c, int d) { return rd8(b, c->ss, (uint16_t)(c->bp + d)); }
static void bpw(pr_bus *b, const cpu_t *c, int d, uint8_t v) { wr8(b, c->ss, (uint16_t)(c->bp + d), v); }
static uint8_t dsb(pr_bus *b, const cpu_t *c, uint16_t off) { return rd8(b, c->ds, off); }
static void dsw(pr_bus *b, const cpu_t *c, uint16_t off, uint8_t v) { wr8(b, c->ds, off, v); }
static int jb(const cpu_t *c) { return (c->flags & CF) != 0u; }
static int ja(const cpu_t *c) { return (c->flags & (CF | ZF)) == 0u; }
static int jz(const cpu_t *c) { return (c->flags & ZF) != 0u; }
static int jl(const cpu_t *c) { return ((c->flags & SF) != 0u) != ((c->flags & OF) != 0u); }
static int jg(const cpu_t *c) { return !jz(c) && !jl(c); }

static int stop(pr_r0709_printer_diagnostic *d, uint16_t site, int status, pr_r0709_printer_status s)
{
    d->site = site; d->callee_status = status;
    return (int)s;
}

/* LCALL at `at` into a SYSTEM leaf (0B25, 0B0B, 0D12, 0DC0) or 0899:0104: live target and segment; back at at+5. */
static int far_call(pr_bus *b, size_t size, cpu_t *c, uint16_t at, pr_r0709_printer_diagnostic *d)
{
    uint16_t home = c->cs, target = rd16(b, c->cs, (uint16_t)(at + 1u)), seg = rd16(b, c->cs, (uint16_t)(at + 3u));
    int status = -1, ok = 0;
    push(b, c, c->cs); push(b, c, (uint16_t)(at + 5u));
    c->cs = seg; c->ip = target;
    if (target == 0x0B25u) {
        pr_system_0b25_cpu m; COPY_CPU(&m, c); status = pr_system_strstore_0b25(b->ram, size, &m); COPY_CPU(c, &m);
        ok = status == PR_SYSTEM_0B25_RETURNED;
    } else if (target == 0x0B0Bu || target == 0x0D12u || target == 0x0DC0u) {
        pr_system_leaf_cpu m; COPY_CPU(&m, c);
        status = target == 0x0B0Bu ? pr_system_strload_0b0b(b->ram, size, &m) : target == 0x0D12u ? pr_system_set_expand_0d12(b->ram, size, &m)
               : pr_system_set_in_0dc0(b->ram, size, &m);
        COPY_CPU(c, &m); ok = status == PR_SYSTEM_LEAF_RETURNED;
    } else if (target == 0x0104u) {
        pr_r0899_idle_cpu m; COPY_CPU(&m, c); status = pr_r0899_idle_0104_cpu(b->ram, size, &m); COPY_CPU(c, &m);
        ok = status == PR_R0899_IDLE_OK;
    }
    if (!ok || c->cs != home || c->ip != (uint16_t)(at + 5u))
        return stop(d, at, status, PR_R0709_PRINTER_CALLEE);
    return CONTINUE;
}
#define FAR(at) do { int s_ = far_call(bus, ram_size, c, (at), d); if (s_ != CONTINUE) return (pr_r0709_printer_status)s_; } while (0)

/* PUSH CS; CALL near at `at` (3 bytes) into 0DD3, 06A7 or 0714; back at at+3. */
static int near_call(pr_bus *b, size_t size, cpu_t *c, uint16_t at, uint16_t target, const pr_r0709_printer_budget *k,
                     pr_r0709_printer_diagnostic *d)
{
    uint16_t home = c->cs;
    int status, ok;
    push(b, c, c->cs); push(b, c, (uint16_t)(at + 3u)); c->ip = target;
    if (target == 0x0DD3u) {
        pr_r0709_clear_cpu m; pr_r0709_clear_cpu_diagnostic cd;
        COPY_CPU(&m, c); status = pr_r0709_clear_0dd3_cpu(b, size, &m, k->clear_loopbacks, k->clear_rect_loopbacks, &cd); COPY_CPU(c, &m);
        d->nested[0] = cd.callee; d->nested[1] = cd.status; d->nested[2] = d->nested[3] = -1;
        if (status == PR_R0709_CLEAR_CPU_LIMIT) return stop(d, at, status, PR_R0709_PRINTER_LIMIT);
        ok = status == PR_R0709_CLEAR_CPU_OK;
    } else if (target == 0x06A7u) {
        pr_r0709_text_cpu m; pr_r0709_text_cpu_diagnostic td;
        COPY_CPU(&m, c);
        status = pr_r0709_text_06a7_cpu(b, size, &m, k->text_loopbacks, k->text_glyph_loopbacks, k->text_delay_loopbacks, &td);
        COPY_CPU(c, &m);
        d->nested[0] = td.callee; d->nested[1] = td.status; d->nested[2] = td.glyph_callee; d->nested[3] = td.glyph_status;
        if (status == PR_R0709_TEXT_CPU_LIMIT) return stop(d, at, status, PR_R0709_PRINTER_LIMIT);
        ok = status == PR_R0709_TEXT_CPU_OK;
    } else {
        pr_r0709_skip_cpu m;
        COPY_CPU(&m, c); status = pr_r0709_skip_spaces_0714_cpu(b->ram, size, &m, k->skip_loopbacks); COPY_CPU(c, &m);
        if (status == PR_R0709_SKIP_LIMIT) return stop(d, at, status, PR_R0709_PRINTER_LIMIT);
        ok = status == PR_R0709_SKIP_OK;
    }
    if (!ok || c->cs != home || c->ip != (uint16_t)(at + 3u))
        return stop(d, at, status, PR_R0709_PRINTER_CALLEE);
    d->nested[0] = d->nested[1] = d->nested[2] = d->nested[3] = -1;
    return CONTINUE;
}
#define NEAR(at, target) do { int s_ = near_call(bus, ram_size, c, (at), (target), budget, d); \
    if (s_ != CONTINUE) return (pr_r0709_printer_status)s_; } while (0)

/* PUSH BP (the parent frame for 06A7 / 0714); PUSH CS; CALL. */
#define NESTED(at, target) do { push(bus, c, c->bp); NEAR(at, target); } while (0)

/* MOV AL,[BP-x]; XOR AH,AH; MOV DI,AX: index a byte of the text buffer [BP+DI-100h]. */
static uint8_t text_at(pr_bus *b, cpu_t *c, int index_off, int plus)
{
    set_al(c, bpb(b, c, index_off)); xor_ah(c);
    if (plus) c->ax = (uint16_t)inc_flags(c, c->ax, 16u);
    c->di = c->ax;
    return rd8(b, c->ss, (uint16_t)(c->bp + c->di - 0x100u));
}
/* PUSH AX (the byte); set expansion of DS:0D16 (0404h) into SS:[BP-125h]; then the membership test. */
static int in_set(pr_bus *bus, size_t ram_size, cpu_t *c, uint16_t at_expand, pr_r0709_printer_diagnostic *d)
{
    int s;
    push(bus, c, c->ax);
    c->di = (uint16_t)(c->bp - 0x125u); push(bus, c, c->ss); push(bus, c, c->di);
    c->di = 0x0D16u; push(bus, c, c->ds); push(bus, c, c->di);
    c->ax = 0x0404u; push(bus, c, c->ax);
    if ((s = far_call(bus, ram_size, c, at_expand, d)) != CONTINUE) return s;
    return far_call(bus, ram_size, c, (uint16_t)(at_expand + 5u), d);
}
#define SETTEST(at) do { int s_ = in_set(bus, ram_size, c, (at), d); if (s_ != CONTINUE) return (pr_r0709_printer_status)s_; } while (0)

/* A loop-back JMP at `at` about to run past its declared budget: LIMIT with IP at the JMP, nothing of it executed. */
#define LOOPBACK(at, used, cap) do { if ((used)++ >= (cap)) { c->ip = (at); \
    return (pr_r0709_printer_status)stop(d, (at), -1, PR_R0709_PRINTER_LIMIT); } } while (0)

/* 08F2 / 0919: AX := j - i + cursor column; CMP AX, x2 (16-bit). */
static void fits(pr_bus *b, cpu_t *c)
{
    set_al(c, bpb(b, c, 0x10)); xor_ah(c); c->bx = c->ax;
    set_al(c, dsb(b, c, 0x5E24u)); xor_ah(c); c->cx = c->ax;
    set_al(c, bpb(b, c, -0x103)); xor_ah(c); c->dx = c->ax;
    set_al(c, bpb(b, c, -0x104)); xor_ah(c);
    c->ax = (uint16_t)sub_flags(c, c->ax, c->dx, 16u);
    c->ax = (uint16_t)add_flags(c, c->ax, c->cx, 16u);
    sub_flags(c, c->ax, c->bx, 16u);
}
static void push_window(pr_bus *b, cpu_t *c)                         /* x1, y1, x2, y2 pushed as AL words */
{
    set_al(c, bpb(b, c, 0x14)); push(b, c, c->ax);
    set_al(c, bpb(b, c, 0x12)); push(b, c, c->ax);
    set_al(c, bpb(b, c, 0x10)); push(b, c, c->ax);
    set_al(c, bpb(b, c, 0x0E)); push(b, c, c->ax);
}

pr_r0709_printer_status pr_r0709_printer_0768_cpu(pr_bus *bus, size_t ram_size, cpu_t *c, const pr_r0709_printer_budget *budget,
                                                  pr_r0709_printer_diagnostic *diagnostic)
{
    pr_r0709_printer_diagnostic local, *d = diagnostic ? diagnostic : &local;
    size_t words = 0, scans = 0;
    if (bus == NULL || bus->ram == NULL || c == NULL || budget == NULL || ram_size != PR_BUS_RAM_SIZE)
        return PR_R0709_PRINTER_ARGUMENT;
    if (c->ip == 0x0768u) {
        if (c->sp < 0x0300u || c->sp > 0xFFECu) return PR_R0709_PRINTER_ARGUMENT;
    } else if (c->ip != 0x0999u) {
        return PR_R0709_PRINTER_ARGUMENT;
    }
    d->site = 0; d->callee_status = -1; d->nested[0] = d->nested[1] = d->nested[2] = d->nested[3] = -1;
    if (c->ip == 0x0999u)
        goto after_bf5;

    push(bus, c, c->bp); c->bp = c->sp;                               /* 0768 PUSH BP; MOV BP,SP */
    { unsigned a = c->sp; c->sp = (uint16_t)sub_flags(c, a, 0x126u, 16u); }                  /* 076B SUB SP,126h */
    c->di = rd16(bus, c->ss, (uint16_t)(c->bp + 6u)); c->es = rd16(bus, c->ss, (uint16_t)(c->bp + 8u));   /* 076F LES DI,[BP+6] */
    push(bus, c, c->es); push(bus, c, c->di);
    c->di = (uint16_t)(c->bp - 0x100u); push(bus, c, c->ss); push(bus, c, c->di);
    c->ax = 0x00FFu; push(bus, c, c->ax);
    FAR(0x077E);                                                      /* StrStore */
    sub_flags(c, bpb(bus, c, 0x14), 0x27u, 8u); if (ja(c)) goto done;                        /* 0783 */
    sub_flags(c, bpb(bus, c, 0x12), 0x18u, 8u); if (ja(c)) goto done;                        /* 0789 */
    sub_flags(c, bpb(bus, c, 0x10), 0x27u, 8u); if (ja(c)) goto done;                        /* 078F */
    sub_flags(c, bpb(bus, c, 0x0E), 0x27u, 8u); if (ja(c)) goto done;                        /* 0795 JBE 079E; 079B JMP 09DD */
    set_al(c, dsb(bus, c, 0x5E24u)); sub_flags(c, (uint8_t)c->ax, bpb(bus, c, 0x14), 8u);     /* 079E */
    if (jb(c)) goto home;
    set_al(c, dsb(bus, c, 0x5E24u)); sub_flags(c, (uint8_t)c->ax, bpb(bus, c, 0x10), 8u);     /* 07A6 */
    if (ja(c)) goto home;
    set_al(c, dsb(bus, c, 0x5E25u)); sub_flags(c, (uint8_t)c->ax, bpb(bus, c, 0x12), 8u);     /* 07AE */
    if (jb(c)) goto home;
    set_al(c, dsb(bus, c, 0x5E25u)); sub_flags(c, (uint8_t)c->ax, bpb(bus, c, 0x0E), 8u);     /* 07B6 JBE 07CA */
    if (!ja(c)) goto clear;
home:
    set_al(c, bpb(bus, c, 0x14)); dsw(bus, c, 0x5E24u, (uint8_t)c->ax);                     /* 07BE */
    set_al(c, bpb(bus, c, 0x12)); dsw(bus, c, 0x5E25u, (uint8_t)c->ax);
clear:
    sub_flags(c, bpb(bus, c, 0x0A), 0u, 8u);                          /* 07CA CMP BYTE [BP+0Ah],0; JE 07F0 */
    if (!jz(c)) {
        push_window(bus, c);
        NEAR(0x07E1, 0x0DD3);                                         /* 07E0 PUSH CS; CALL 0DD3 */
        set_al(c, bpb(bus, c, 0x14)); dsw(bus, c, 0x5E24u, (uint8_t)c->ax);                 /* 07E4 */
        set_al(c, bpb(bus, c, 0x12)); dsw(bus, c, 0x5E25u, (uint8_t)c->ax);
    }
    bpw(bus, c, -0x103, 1u);                                          /* 07F0 MOV BYTE [BP-103h],1 */
    set_al(c, bpb(bus, c, -0x100)); bpw(bus, c, -0x101, (uint8_t)c->ax);                     /* 07F5 */
    sub_flags(c, bpb(bus, c, -0x101), 0u, 8u);                        /* 07FD CMP; JNE 0807; JMP 09DD */
    if (jz(c)) goto done;
    set_al(c, bpb(bus, c, 0x14)); xor_ah(c); c->dx = c->ax;           /* 0807 */
    set_al(c, bpb(bus, c, 0x10)); xor_ah(c);
    c->ax = (uint16_t)sub_flags(c, c->ax, c->dx, 16u);
    c->ax = (uint16_t)inc_flags(c, c->ax, 16u);
    bpw(bus, c, -0x102, (uint8_t)c->ax);                              /* 0816 MOV [BP-102h],AL */
    for (;;) {
        set_al(c, bpb(bus, c, -0x103)); bpw(bus, c, -0x104, (uint8_t)c->ax);                /* 081A j := i */
        for (;;) {                                                    /* 0822: skip set members */
            set_al(c, bpb(bus, c, -0x104)); sub_flags(c, (uint8_t)c->ax, bpb(bus, c, -0x101), 8u);
            if (!jb(c)) break;                                        /* JAE 085A */
            set_al(c, text_at(bus, c, -0x104, 0));                    /* 082C */
            SETTEST(0x0848);
            if (jz(c)) break;                                         /* 0852 JE 085A */
            bpw(bus, c, -0x104, (uint8_t)inc_flags(c, bpb(bus, c, -0x104), 8u));              /* 0854 INC; JMP 0822 */
            LOOPBACK(0x0858u, scans, budget->scan_loopbacks);
        }
        for (;;) {                                                    /* 085A: up to a set member or a space */
            set_al(c, bpb(bus, c, -0x104)); sub_flags(c, (uint8_t)c->ax, bpb(bus, c, -0x101), 8u);
            if (!jb(c)) break;                                        /* JAE 08A1 */
            set_al(c, text_at(bus, c, -0x104, 0));                    /* 0864 */
            SETTEST(0x0880);
            if (!jz(c)) break;                                        /* 088A JNE 08A1 */
            sub_flags(c, text_at(bus, c, -0x104, 0), 0x20u, 8u);      /* 088C CMP [BP+DI-100h],20h; JE 08A1 */
            if (jz(c)) break;
            bpw(bus, c, -0x104, (uint8_t)inc_flags(c, bpb(bus, c, -0x104), 8u));              /* 089B INC; JMP 085A */
            LOOPBACK(0x089Fu, scans, budget->scan_loopbacks);
        }
        sub_flags(c, text_at(bus, c, -0x104, 0), 0x20u, 8u);          /* 08A1 CMP [BP+DI-100h],20h; JE 08F2 */
        if (!jz(c)) {
            for (;;) {                                                /* 08B0: while j + 1 < n and text[j+1] is a set member */
                set_al(c, bpb(bus, c, -0x101)); xor_ah(c); c->dx = c->ax;
                set_al(c, bpb(bus, c, -0x104)); xor_ah(c);
                c->ax = (uint16_t)inc_flags(c, c->ax, 16u);
                sub_flags(c, c->ax, c->dx, 16u);                      /* 08BF CMP AX,DX; JGE 08F2 */
                if (!jl(c)) break;
                set_al(c, text_at(bus, c, -0x104, 1));                /* 08C3 */
                SETTEST(0x08E0);
                if (jz(c)) break;                                     /* 08EA JE 08F2 */
                bpw(bus, c, -0x104, (uint8_t)inc_flags(c, bpb(bus, c, -0x104), 8u));          /* 08EC INC; JMP 08B0 */
                LOOPBACK(0x08F0u, scans, budget->scan_loopbacks);
            }
        }
        fits(bus, c);                                                 /* 08F2: JG 0919, else JMP 09B9 */
        if (jg(c)) {
            fits(bus, c);                                             /* 0919: JNE 0955 */
            if (jz(c)) {
                sub_flags(c, text_at(bus, c, -0x104, 0), 0x20u, 8u);  /* 093D: JNE 0955 */
                if (jz(c)) {
                    bpw(bus, c, -0x104, (uint8_t)dec_flags(c, bpb(bus, c, -0x104), 8u));      /* 094C DEC */
                    NESTED(0x0952, 0x06A7);                           /* 0950 PUSH BP; PUSH CS; CALL 06A7 */
                }
            }
            set_al(c, bpb(bus, c, 0x14)); dsw(bus, c, 0x5E24u, (uint8_t)c->ax);             /* 0955 */
            dsw(bus, c, 0x5E25u, (uint8_t)inc_flags(c, dsb(bus, c, 0x5E25u), 8u));           /* 095B INC BYTE [5E25] */
            NESTED(0x0961, 0x0714);                                   /* 095F PUSH BP; PUSH CS; CALL 0714 */
            set_al(c, dsb(bus, c, 0x5E25u)); sub_flags(c, (uint8_t)c->ax, bpb(bus, c, 0x0E), 8u);   /* 0964 JBE 09B7 */
            if (ja(c)) {
                set_al(c, bpb(bus, c, -0x103)); sub_flags(c, (uint8_t)c->ax, bpb(bus, c, -0x101), 8u);   /* 096C JAE 09B7 */
                if (jb(c)) {
                    set_al(c, bpb(bus, c, 0x14)); dsw(bus, c, 0x5E24u, (uint8_t)c->ax);     /* 0976 */
                    set_al(c, bpb(bus, c, 0x12)); dsw(bus, c, 0x5E25u, (uint8_t)c->ax);
                    set_al(c, 0x0Du); push(bus, c, c->ax);            /* 0982 MOV AL,0Dh; PUSH AX */
                    c->di = (uint16_t)(c->bp - 0x11Fu); push(bus, c, c->ss); push(bus, c, c->di);
                    c->di = 0x074Eu; push(bus, c, c->cs); push(bus, c, c->di);
                    FAR(0x0990);                                      /* string load CS:074E -> [BP-11Fh] */
                    push(bus, c, c->cs); push(bus, c, 0x0999u); c->ip = 0x0BF5u;   /* 0995 PUSH CS; CALL 0BF5 */
                    return (pr_r0709_printer_status)stop(d, 0x0996u, -1, PR_R0709_PRINTER_STOP_BF5);
after_bf5:
                    FAR(0x0999);                                      /* 0899:0104 */
                    push_window(bus, c);                              /* 099E */
                    NEAR(0x09AF, 0x0DD3);                             /* 09AE PUSH CS; CALL 0DD3 */
                    NESTED(0x09B4, 0x06A7);                           /* 09B2 PUSH BP; PUSH CS; CALL 06A7 */
                }
            }
        } else {
            NESTED(0x09BB, 0x06A7);                                   /* 09B9 PUSH BP; PUSH CS; CALL 06A7 */
        }
        set_al(c, bpb(bus, c, -0x103)); sub_flags(c, (uint8_t)c->ax, bpb(bus, c, -0x101), 8u);   /* 09BE JA 09CB; JMP 081A */
        if (ja(c)) break;
        LOOPBACK(0x09C8u, words, budget->word_loopbacks);
    }
    set_al(c, dsb(bus, c, 0x5E24u)); sub_flags(c, (uint8_t)c->ax, bpb(bus, c, 0x10), 8u);   /* 09CB JBE 09DD */
    if (ja(c)) {
        set_al(c, bpb(bus, c, 0x14)); dsw(bus, c, 0x5E24u, (uint8_t)c->ax);                 /* 09D3 */
        dsw(bus, c, 0x5E25u, (uint8_t)inc_flags(c, dsb(bus, c, 0x5E25u), 8u));
    }
done:
    c->sp = c->bp; c->bp = pop(bus, c);                               /* 09DD MOV SP,BP; POP BP */
    c->ip = pop(bus, c); c->cs = pop(bus, c);                         /* 09E0 RETF 10h */
    c->sp = (uint16_t)(c->sp + 0x10u);
    return PR_R0709_PRINTER_RETURNED;
}
