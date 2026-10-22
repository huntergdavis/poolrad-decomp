#!/usr/bin/env python3
"""Regenerate src/recovered/resident_03f1_box_0000_cpu.c and its header with emit_raw_cpu_c.py.

python3 tools/overlay/emit_res03f1_box_0000.py [--check]   (--check compares instead of writing)
Needs the owner START.EXE under resources/. Prints the undefined-flag observability report."""
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
from resident_loader_map import unpack
from emit_raw_cpu_c import Emitter

ROOT = Path(__file__).resolve().parents[2]
img = bytes(unpack((ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes())[0])
START, END = 0x0000, 0x0137
e = Emitter(img[0x3F10 + START:0x3F10 + END], START, END, {0x002A: 'call_tile_stub'})
for a, mn, ops, und, vis in e.undefined_report():
    print(f'UNDEF {a:04X} {mn} {ops}: {und} -> {vis if vis else "never observed (overwritten on every path)"}')
cases = '\n'.join(e.cases())

header = '''#ifndef POOLRAD_RECOVERED_RESIDENT_03F1_BOX_0000_CPU_H
#define POOLRAD_RECOVERED_RESIDENT_03F1_BOX_0000_CPU_H
#include <stddef.h>
#include <stdint.h>
#include "poolrad/recovered/overlay_u40c_tile_015d_cpu.h"

/* Raw-CPU, full-state model of START.EXE resident 03F1:[0000,0137) (311 bytes, 145 instructions, RETF 0Ah): draw a
 * tile box. Arguments: [BP+6] style byte, [BP+8] bottom row, [BP+0Ah] right column, [BP+0Ch] top row, [BP+0Eh] left
 * column (the four coordinate bytes are first adjusted in place: left and top DEC, right and bottom INC). The style's
 * three tile numbers are style * 3 + 114h + {0 corner, 1 vertical edge, 2 horizontal edge} ([BP-3]). Four corners are
 * placed, then the horizontal edges for each interior column ([BP-1] from left + 1 to right - 1, signed) and the vertical
 * edges for each interior row, every tile through overlay stub 0428:002A (unit 40Ch entry 2, +015D) with the frame
 * (column, row, number, 0, 0); AH of each pushed byte word is what the preceding instruction left.
 * The stub is executed as on the CPU: a loaded overlay's entry is JMP FAR (EAh) to the unit's segment:015D, followed into
 * OVL_040C_015D's model; an unloaded entry (INT 3Fh, the overlay manager) stops with OVERLAY. The stub's segment word is
 * a loader relocation, read live at each call site. Entered after a far call: [SP] IP, [SP+2] CS, then the five argument
 * words; RETF 0Ah. See docs/re/RES_03F1_BOX_0000_CPU.md for the contract and evidence. */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_r03f1_box_regs;

typedef struct {
    pr_ovl040c_015d_env tile;   /* the environment of every 015D call */
    size_t max_loopbacks;       /* this body's backward jumps before the LIMIT stop */
} pr_r03f1_box_env;

typedef enum {
    PR_R03F1_BOX_OK = 0,        /* RETF 0Ah executed */
    PR_R03F1_BOX_ARGUMENT,      /* null pointers, RAM not exactly 1 MiB, IP not 0000h; nothing changed */
    PR_R03F1_BOX_CALLEE,        /* 015D stopped (CALLEE/TRANSFER/ARGUMENT; diagnostic: site and its status) */
    PR_R03F1_BOX_TRANSFER,      /* a call returned somewhere other than the instruction after it */
    PR_R03F1_BOX_LIMIT,         /* max_loopbacks backward jumps taken; stopped before the next */
    PR_R03F1_BOX_OVERLAY,       /* the stub entry is not a loaded JMP FAR to +015D; stopped at the stub with the frame pushed */
    PR_R03F1_BOX_CONTRACT,      /* 015D's CONTRACT stop (EGA attached), state as it left it */
    PR_R03F1_BOX_EXIT           /* 015D's EXIT (its error branch reached Halt, or ReadKey exited) */
} pr_r03f1_box_status;

typedef struct {
    uint16_t site;              /* the call site of the stopping tile call, 0 when none */
    int status;                 /* 015D's status there, -1 when none */
    unsigned calls;             /* tile calls made */
    pr_ovl040c_015d_diagnostic tile;   /* the last 015D call's diagnostic */
} pr_r03f1_box_diagnostic;

pr_r03f1_box_status pr_r03f1_box_0000_cpu(pr_bus *bus, size_t ram_size, pr_r03f1_box_regs *cpu, const pr_r03f1_box_env *env,
                                          pr_r03f1_box_diagnostic *diagnostic);
#endif
'''

source = '''/* START.EXE resident 03F1:0000 tile box, raw CPU (docs/re/RES_03F1_BOX_0000_CPU.md).
 * One case per original instruction; operands are reread from the bus in source order. */
#include "poolrad/recovered/resident_03f1_box_0000_cpu.h"
#include <string.h>
#define MASK (PR_BUS_RAM_SIZE - 1u)
#define STATUS(s) PR_R03F1_BOX_##s
typedef pr_r03f1_box_status pr_status_t;
typedef pr_r03f1_box_regs pr_cpu_t;
typedef struct {
    const pr_r03f1_box_env *env;
    pr_r03f1_box_diagnostic *d;
} pr_run;
_Static_assert(sizeof(pr_cpu_t) == sizeof(pr_ovl040c_015d_regs), "CPU layout");
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
 * Unicorn's rule otherwise); AF := 0 (Unicorn). The report in the doc shows where these are observed. */
static uint16_t shift(pr_cpu_t *c, uint16_t v, uint8_t n, unsigned bits, unsigned right)
{
    uint32_t mask = bits == 8u ? 0xFFu : 0xFFFFu, sign = bits == 8u ? 0x80u : 0x8000u, pre, res;
    n &= 0x1Fu;
    if (n == 0u) return v;
    v = (uint16_t)(v & mask);
    pre = right ? ((uint32_t)v >> (n - 1u)) : (((uint32_t)v << (n - 1u)) & mask);
    res = right ? (pre >> 1u) : ((pre << 1u) & mask);
    c->flags &= (uint16_t)~0x08D5u;
    if (right ? (pre & 1u) : (pre & sign)) c->flags |= 1u;
    if ((pre ^ res) & sign) c->flags |= 0x800u;
    szp(c, (uint16_t)res, bits); return (uint16_t)res;
}
/* Overlay stub 0428:002A, entered with the far frame pushed. A loaded entry is JMP FAR (EAh) to the unit's segment:015D;
 * it is executed, then OVL_040C_015D runs to its RETF 0Ah, which returns straight to this body. */
static int call_tile_stub(pr_bus *b, pr_cpu_t *c, pr_run *r)
{
    pr_ovl040c_015d_regs in; int s;
    uint16_t site = (uint16_t)(rd16(b, c->ss, c->sp) - 5u), ip, cs;
    r->d->calls += 1u;
    if (rd8(b, c->cs, c->ip) != 0xEAu) { r->d->site = site; return STATUS(OVERLAY); }
    ip = rd16(b, c->cs, (uint16_t)(c->ip + 1u)); cs = rd16(b, c->cs, (uint16_t)(c->ip + 3u));
    if (ip != 0x015Du) { r->d->site = site; return STATUS(OVERLAY); }
    c->ip = ip; c->cs = cs;
    memcpy(&in, c, sizeof in);
    s = (int)pr_ovl040c_015d_cpu(b, PR_BUS_RAM_SIZE, &in, &r->env->tile, &r->d->tile);
    memcpy(c, &in, sizeof in);
    if (s == PR_OVL040C_015D_OK) return 0;
    r->d->site = site; r->d->status = s;
    return s == PR_OVL040C_015D_EXIT ? STATUS(EXIT) : s == PR_OVL040C_015D_CONTRACT ? STATUS(CONTRACT) : STATUS(CALLEE);
}

pr_r03f1_box_status pr_r03f1_box_0000_cpu(pr_bus *b, size_t ram_size, pr_r03f1_box_regs *c, const pr_r03f1_box_env *env,
                                          pr_r03f1_box_diagnostic *d)
{
    pr_r03f1_box_diagnostic fallback;
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
targets = {ROOT / 'include/poolrad/recovered/resident_03f1_box_0000_cpu.h': header,
           ROOT / 'src/recovered/resident_03f1_box_0000_cpu.c': source}
if '--check' in sys.argv:
    bad = [str(p) for p, text in targets.items() if p.read_text() != text]
    print('EMIT_CHECK', 'MISMATCH ' + ', '.join(bad) if bad else 'OK'); sys.exit(1 if bad else 0)
for p, text in targets.items():
    p.write_text(text)
print('cases', len(e.ins))
