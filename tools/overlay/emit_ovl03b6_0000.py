#!/usr/bin/env python3
"""Regenerate src/recovered/overlay_u3b6_window_0000_cpu.c and its header with emit_raw_cpu_c.py.

python3 tools/overlay/emit_ovl03b6_0000.py [--check]   (--check compares instead of writing)
Needs the owner START.EXE and GAME.OVR under resources/. Prints the undefined-flag observability report."""
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
from overlay_inventory import find_units, parse_mz
from emit_raw_cpu_c import Emitter

ROOT = Path(__file__).resolve().parents[2]
exe = (ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes()
ovr = (ROOT / 'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
unit = {u['stub_para']: u for u in find_units(exe[parse_mz(exe)['header_bytes']:], ovr)}[0x3B6]
code = ovr[unit['ovr_offset']:unit['ovr_offset'] + unit['code_size']]
START, END = 0x0000, 0x0206
e = Emitter(code[START:END], START, END, {0x002A: 'call_tile_stub'})
for a, mn, ops, und, vis in e.undefined_report():
    print(f'UNDEF {a:04X} {mn} {ops}: {und} -> {vis if vis else "never observed (overwritten on every path)"}')
cases = '\n'.join(e.cases())

header = '''#ifndef POOLRAD_RECOVERED_OVERLAY_U3B6_WINDOW_0000_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U3B6_WINDOW_0000_CPU_H
#include <stddef.h>
#include <stdint.h>
#include "poolrad/recovered/overlay_u3ce_tile_00f1_cpu.h"

/* Raw-CPU, full-state model of GAME.OVR unit 3B6h +[0000,0206) (518 bytes, 211 instructions, RETF 4): draw a 5 x 5
 * window of the 44-column tile map DS:3648. For row r ([BP-8], 0..4) and column k ([BP-7], 0..4):
 *   x := low byte of (k + word [BP+8]) ([BP-4]); y := low byte of (r + word [BP+6]) ([BP-5]);
 *   tile := DS:[3648 + y * 2Ch + x] - 1 ([BP-6]; x and y sign-extended, the product and sum wrap at 16 bits);
 *   for each record i = 1..9 ([BP-3]) whose words DS:0C30 + 6i and DS:0C32 + 6i equal x and y (sign-extended to 32 bits
 *   against the zero-extended word): records 3..9 replace the tile with low byte of (DS:0C34 + 6i) - 1, subject to the
 *   window-0 words at far DS:49D2: i = 3 when +318h is not 00FFh; i = 4 unless +3A4h = 0, (+2F8h AND 8) = 0 and
 *   +33Eh is non-zero; i = 5 when (+340h AND 2) = 0; i = 6 always; i = 7 when (+340h AND 1) = 0; i = 8 and 9 when +366h
 *   is above 00FDh (unsigned). Records 1 and 2 never replace it. Later records override earlier ones.
 * Then the tile goes through overlay stub 03E9:002A (unit 3CEh entry 2, +00F1) with the frame (1 in AL, tile, column
 * [BP-2], row [BP-1]); [BP-1] starts at 0 for each row and steps by 3 per column, [BP-2] starts at 0 and steps by 3 per
 * row. AH of each pushed byte word is what the preceding instruction left.
 * The stub is executed as on the CPU: a loaded overlay's entry is JMP FAR (EAh) to the unit's segment:00F1, followed into
 * OVL_03CE_00F1's model; an unloaded entry (INT 3Fh, the overlay manager) stops with OVERLAY. The stub's segment word
 * (CS:01E4) is a loader fixup, read live. Entered after a far call: [SP] IP, [SP+2] CS, then the two argument words;
 * RETF 4. See docs/re/OVL_03B6_0000_CPU.md for the contract and evidence. */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_ovl03b6_0000_regs;

typedef struct {
    pr_ovl03ce_00f1_env tile;   /* the environment of every 00F1 call */
    size_t max_loopbacks;       /* this body's backward jumps before the LIMIT stop */
} pr_ovl03b6_0000_env;

typedef enum {
    PR_OVL03B6_0000_OK = 0,     /* RETF 4 executed */
    PR_OVL03B6_0000_ARGUMENT,   /* null pointers, RAM not exactly 1 MiB, IP not 0000h; nothing changed */
    PR_OVL03B6_0000_CALLEE,     /* 00F1 stopped (CALLEE/TRANSFER/ARGUMENT; diagnostic: site and its status) */
    PR_OVL03B6_0000_TRANSFER,   /* a call returned somewhere other than the instruction after it */
    PR_OVL03B6_0000_LIMIT,      /* max_loopbacks backward jumps taken; stopped before the next */
    PR_OVL03B6_0000_OVERLAY     /* the stub entry is not a loaded JMP FAR to +00F1; stopped at the stub with the frame pushed */
} pr_ovl03b6_0000_status;

typedef struct {
    uint16_t site;              /* the call site of the stopping tile call, 0 when none */
    int status;                 /* 00F1's status there, -1 when none */
    unsigned calls;             /* tile calls made */
    pr_ovl03ce_00f1_diagnostic tile;   /* the last 00F1 call's diagnostic */
} pr_ovl03b6_0000_diagnostic;

pr_ovl03b6_0000_status pr_ovl03b6_0000_cpu(pr_bus *bus, size_t ram_size, pr_ovl03b6_0000_regs *cpu,
                                           const pr_ovl03b6_0000_env *env, pr_ovl03b6_0000_diagnostic *diagnostic);
#endif
'''

source = '''/* GAME.OVR unit 3B6h +0000 map window, raw CPU (docs/re/OVL_03B6_0000_CPU.md).
 * One case per original instruction; operands are reread from the bus in source order. */
#include "poolrad/recovered/overlay_u3b6_window_0000_cpu.h"
#include <string.h>
#define MASK (PR_BUS_RAM_SIZE - 1u)
#define STATUS(s) PR_OVL03B6_0000_##s
typedef pr_ovl03b6_0000_status pr_status_t;
typedef pr_ovl03b6_0000_regs pr_cpu_t;
typedef struct {
    const pr_ovl03b6_0000_env *env;
    pr_ovl03b6_0000_diagnostic *d;
} pr_run;
_Static_assert(sizeof(pr_cpu_t) == sizeof(pr_ovl03ce_00f1_regs), "CPU layout");
static size_t physical(uint16_t seg, uint16_t off) { return (((size_t)seg << 4u) + off) & MASK; }
static uint16_t word_at(pr_bus *b, size_t at)
{
    return (uint16_t)(pr_bus_read8(b, at) | ((uint16_t)pr_bus_read8(b, (at + 1u) & MASK) << 8u));
}
static uint8_t rd8(pr_bus *b, uint16_t seg, uint16_t off) { return pr_bus_read8(b, physical(seg, off)); }
static uint16_t rd16(pr_bus *b, uint16_t seg, uint16_t off) { return word_at(b, physical(seg, off)); }
static void wr8(pr_bus *b, uint16_t seg, uint16_t off, uint8_t v) { pr_bus_write8(b, physical(seg, off), v); }
static void wr16(pr_bus *b, uint16_t seg, uint16_t off, uint16_t v)
{
    size_t at = physical(seg, off);
    pr_bus_write8(b, at, (uint8_t)v); pr_bus_write8(b, (at + 1u) & MASK, (uint8_t)(v >> 8u));
}
static void push(pr_bus *b, pr_cpu_t *c, uint16_t v) { c->sp = (uint16_t)(c->sp - 2u); wr16(b, c->ss, c->sp, v); }
static uint16_t pop(pr_bus *b, pr_cpu_t *c) { uint16_t v = rd16(b, c->ss, c->sp); c->sp = (uint16_t)(c->sp + 2u); return v; }
static void szp(pr_cpu_t *c, uint16_t r, unsigned bits)
{
    uint8_t p = (uint8_t)r;
    p ^= (uint8_t)(p >> 4u); p ^= (uint8_t)(p >> 2u); p ^= (uint8_t)(p >> 1u);
    if ((p & 1u) == 0u) c->flags |= 0x04u;
    if ((r & (bits == 8u ? 0xFFu : 0xFFFFu)) == 0u) c->flags |= 0x40u;
    if (r & (bits == 8u ? 0x80u : 0x8000u)) c->flags |= 0x80u;
}
static uint16_t arith(pr_cpu_t *c, uint16_t l, uint16_t r, unsigned carry, unsigned bits, unsigned sub)
{
    uint32_t mask = bits == 8u ? 0xFFu : 0xFFFFu, sign = bits == 8u ? 0x80u : 0x8000u;
    uint32_t wide = sub ? (uint32_t)l - r - carry : (uint32_t)l + r + carry;
    uint16_t res = (uint16_t)(wide & mask);
    c->flags &= (uint16_t)~0x08D5u;
    if (sub ? (uint32_t)l < (uint32_t)r + carry : wide > mask) c->flags |= 1u;
    if ((l ^ r ^ res) & 0x10u) c->flags |= 0x10u;
    if ((sub ? (l ^ r) & (l ^ res) : (uint16_t)~(l ^ r) & (l ^ res)) & sign) c->flags |= 0x800u;
    szp(c, res, bits); return res;
}
static uint16_t logic(pr_cpu_t *c, uint16_t v, unsigned bits)
{
    v = (uint16_t)(v & (bits == 8u ? 0xFFu : 0xFFFFu)); c->flags &= (uint16_t)~0x08D5u; szp(c, v, bits); return v;
}
static uint16_t incdec(pr_cpu_t *c, uint16_t v, unsigned bits, unsigned dec)
{
    uint16_t cf = c->flags & 1u; v = arith(c, v, 1u, 0u, bits, dec);
    c->flags = (uint16_t)((c->flags & ~1u) | cf); return v;
}
/* SHL/SHR, count masked to 5 bits; a zero count changes nothing. CF is the last bit out; OF is
 * MSB(v<<(n-1)) ^ MSB(result) for SHL and MSB(v>>(n-1)) ^ MSB(result) for SHR (architectural for n=1,
 * Unicorn's rule otherwise); AF := 1 (pinned DOSBox-X; Unicorn clears it). The emitter's report shows neither is observed. */
static uint16_t shift(pr_cpu_t *c, uint16_t v, uint8_t n, unsigned bits, unsigned right)
{
    uint32_t mask = bits == 8u ? 0xFFu : 0xFFFFu, sign = bits == 8u ? 0x80u : 0x8000u, pre, res;
    n &= 0x1Fu;
    if (n == 0u) return v;
    v = (uint16_t)(v & mask);
    pre = right ? ((uint32_t)v >> (n - 1u)) : (((uint32_t)v << (n - 1u)) & mask);
    res = right ? (pre >> 1u) : ((pre << 1u) & mask);
    c->flags &= (uint16_t)~0x08D5u;
    c->flags |= 0x10u; /* pinned: AF = (count & 1Fh) != 0 */
    if (right ? (pre & 1u) : (pre & sign)) c->flags |= 1u;
    if ((pre ^ res) & sign) c->flags |= 0x800u;
    szp(c, (uint16_t)res, bits); return (uint16_t)res;
}
/* MUL r/m16: DX:AX := AX * src; CF = OF = (DX != 0); SF/ZF/PF from AX (PF from its low byte); AF := 0. */
static void mul16(pr_cpu_t *c, uint16_t src)
{
    uint32_t p = (uint32_t)c->ax * src;
    c->ax = (uint16_t)p; c->dx = (uint16_t)(p >> 16u);
    /* Pinned MULW: SF/AF kept, ZF from AX, PF is the even parity of all four bytes of DX:AX. */
    {
        uint32_t fold = p;
        fold ^= fold >> 16; fold ^= fold >> 8; fold ^= fold >> 4; fold ^= fold >> 2; fold ^= fold >> 1;
        c->flags = (uint16_t)((c->flags & ~0x0845u) | (c->dx != 0u ? 0x0801u : 0u) |
                                (c->ax == 0u ? 0x0040u : 0u) | ((fold & 1u) ? 0u : 0x0004u));
    }
}
/* Overlay stub 03E9:002A, entered with the far frame pushed. A loaded entry is JMP FAR (EAh) to the unit's segment:00F1;
 * it is executed, then OVL_03CE_00F1 runs to its RETF 8, which returns straight to this body. */
static int call_tile_stub(pr_bus *b, pr_cpu_t *c, pr_run *r)
{
    pr_ovl03ce_00f1_regs in; int s;
    uint16_t site = (uint16_t)(rd16(b, c->ss, c->sp) - 5u), ip, cs;
    r->d->calls += 1u;
    if (rd8(b, c->cs, c->ip) != 0xEAu) { r->d->site = site; return STATUS(OVERLAY); }
    ip = rd16(b, c->cs, (uint16_t)(c->ip + 1u)); cs = rd16(b, c->cs, (uint16_t)(c->ip + 3u));
    if (ip != 0x00F1u) { r->d->site = site; return STATUS(OVERLAY); }
    c->ip = ip; c->cs = cs;
    memcpy(&in, c, sizeof in);
    s = (int)pr_ovl03ce_00f1_cpu(b, PR_BUS_RAM_SIZE, &in, &r->env->tile, &r->d->tile);
    memcpy(c, &in, sizeof in);
    if (s == PR_OVL03CE_00F1_OK) return 0;
    r->d->site = site; r->d->status = s; return STATUS(CALLEE);
}

pr_ovl03b6_0000_status pr_ovl03b6_0000_cpu(pr_bus *b, size_t ram_size, pr_ovl03b6_0000_regs *c,
                                           const pr_ovl03b6_0000_env *env, pr_ovl03b6_0000_diagnostic *d)
{
    pr_ovl03b6_0000_diagnostic fallback;
    pr_run run, *dg = &run;
    uint16_t caller; size_t loops = 0, max_loopbacks;
    if (b == NULL || b->ram == NULL || c == NULL || env == NULL || ram_size != PR_BUS_RAM_SIZE || c->ip != 0x0000u)
        return STATUS(ARGUMENT);
    run.env = env; run.d = d != NULL ? d : &fallback;
    memset(run.d, 0, sizeof *run.d);
    run.d->status = -1;
    max_loopbacks = env->max_loopbacks;
    caller = c->cs;
    for (;;) {
        switch (c->ip) {
CASES
        default: return STATUS(TRANSFER);
        }
    }
}
'''.replace('CASES', cases)
import re
for name in ('mul16', 'shift', 'incdec', 'logic'):
    if name + '(c,' not in cases:
        source = re.sub(r'(/\*[^/]*?\*/\n)?static [a-z0-9_ ]+\*?' + name + r'\(.*?\n}\n', '', source, count=1, flags=re.S)
targets = {ROOT / 'include/poolrad/recovered/overlay_u3b6_window_0000_cpu.h': header,
           ROOT / 'src/recovered/overlay_u3b6_window_0000_cpu.c': source}
if '--check' in sys.argv:
    bad = [str(p) for p, text in targets.items() if p.read_text() != text]
    print('EMIT_CHECK', 'MISMATCH ' + ', '.join(bad) if bad else 'OK'); sys.exit(1 if bad else 0)
for p, text in targets.items():
    p.write_text(text)
print('cases', len(e.ins))
