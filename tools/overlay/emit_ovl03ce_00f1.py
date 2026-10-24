#!/usr/bin/env python3
"""Regenerate src/recovered/overlay_u3ce_tile_00f1_cpu.c and its header with emit_raw_cpu_c.py.

python3 tools/overlay/emit_ovl03ce_00f1.py [--check]   (--check compares instead of writing)
Needs the owner START.EXE and GAME.OVR under resources/. Prints the undefined-flag observability report."""
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
from overlay_inventory import find_units, parse_mz
from emit_raw_cpu_c import Emitter

ROOT = Path(__file__).resolve().parents[2]
exe = (ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes()
ovr = (ROOT / 'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
unit = {u['stub_para']: u for u in find_units(exe[parse_mz(exe)['header_bytes']:], ovr)}[0x3CE]
code = ovr[unit['ovr_offset']:unit['ovr_offset'] + unit['code_size']]
START, END = 0x00F1, 0x014E
e = Emitter(code[START:END], START, END, {0x08D4: 'call_compose_08d4'})
for a, mn, ops, und, vis in e.undefined_report():
    print(f'UNDEF {a:04X} {mn} {ops}: {und} -> {vis if vis else "never observed (overwritten on every path)"}')
cases = '\n'.join(e.cases())

header = '''#ifndef POOLRAD_RECOVERED_OVERLAY_U3CE_TILE_00F1_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U3CE_TILE_00F1_CPU_H
#include <stddef.h>
#include <stdint.h>
#include "poolrad/recovered/memory_bus.h"
#include "poolrad/recovered/resident_0464_compose_08d4_cpu.h"

/* Raw-CPU, full-state model of GAME.OVR unit 3CEh +[00F1,014E) (93 bytes, 38 instructions, RETF 8): place one tile in
 * the off-screen object DS:4A20 through 0464:08D4 (the tile compositor). Tile byte [BP+8] above 7Fh selects the tile set
 * at far DS:5E56 with the tile [BP+8] AND 7Fh; 00h..7Fh selects the tile set at far DS:5E52 with the tile unchanged. The
 * 08D4 frame is (object DS:4A20/4A22, tile set, flag 0 in AL, tile, sign-extended byte [BP+0Ah], sign-extended byte
 * [BP+0Ch]); AH of each pushed byte word is what the preceding instruction left. The word [BP+6] is not read.
 * Entered after a far call: [SP] IP, [SP+2] CS, then the four argument words; RETF 8. CS is the unit's load segment;
 * the two far-call segment words (CS:0120, CS:0148) are loader fixups, read live. 08D4 runs through its merged model on
 * the same RAM. Operands are reread in source order; offsets wrap at 16 bits and physical addresses at 1 MiB.
 * See docs/re/OVL_03CE_00F1_CPU.md for the contract and evidence. */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_ovl03ce_00f1_regs;

typedef struct {
    size_t max_loopbacks;       /* passed to 08D4 */
} pr_ovl03ce_00f1_env;

typedef enum {
    PR_OVL03CE_00F1_OK = 0,     /* RETF 8 executed */
    PR_OVL03CE_00F1_ARGUMENT,   /* null pointers, RAM not exactly 1 MiB, IP not 00F1h; nothing changed */
    PR_OVL03CE_00F1_CALLEE,     /* 08D4 stopped (diagnostic: site and its status); state as it left it */
    PR_OVL03CE_00F1_TRANSFER    /* 08D4 returned somewhere other than the instruction after its call */
} pr_ovl03ce_00f1_status;

typedef struct {
    uint16_t site;              /* the far-call site of the stopping 08D4 call, 0 when none */
    int status;                 /* 08D4's status there, -1 when none */
    unsigned calls;             /* 08D4 calls entered */
    pr_r0464_compose_cpu_diagnostic compose;
} pr_ovl03ce_00f1_diagnostic;

pr_ovl03ce_00f1_status pr_ovl03ce_00f1_cpu(pr_bus *bus, size_t ram_size, pr_ovl03ce_00f1_regs *cpu,
                                           const pr_ovl03ce_00f1_env *env, pr_ovl03ce_00f1_diagnostic *diagnostic);
#endif
'''

source = '''/* GAME.OVR unit 3CEh +00F1 tile placement, raw CPU (docs/re/OVL_03CE_00F1_CPU.md).
 * One case per original instruction; operands are reread from the bus in source order. */
#include "poolrad/recovered/overlay_u3ce_tile_00f1_cpu.h"
#include <string.h>
#define MASK (PR_BUS_RAM_SIZE - 1u)
#define STATUS(s) PR_OVL03CE_00F1_##s
typedef pr_ovl03ce_00f1_status pr_status_t;
typedef pr_ovl03ce_00f1_regs pr_cpu_t;
typedef struct {
    const pr_ovl03ce_00f1_env *env;
    pr_ovl03ce_00f1_diagnostic *d;
} pr_run;
_Static_assert(sizeof(pr_cpu_t) == sizeof(pr_r0464_compose_cpu), "CPU layout");
static size_t physical(uint16_t seg, uint16_t off) { return (((size_t)seg << 4u) + off) & MASK; }
static uint16_t word_at(pr_bus *b, size_t at)
{
    return (uint16_t)(pr_bus_read8(b, at) | ((uint16_t)pr_bus_read8(b, (at + 1u) & MASK) << 8u));
}
static uint8_t rd8(pr_bus *b, uint16_t seg, uint16_t off) { return pr_bus_read8(b, physical(seg, off)); }
static uint16_t rd16(pr_bus *b, uint16_t seg, uint16_t off) { return word_at(b, physical(seg, off)); }
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
/* 0464:08D4 on the same bus (its accepted raw model); a non-OK status stops here with the site recorded. */
static int call_compose_08d4(pr_bus *b, pr_cpu_t *c, pr_run *r)
{
    pr_r0464_compose_cpu in; int s;
    uint16_t site = (uint16_t)(rd16(b, c->ss, c->sp) - 5u);
    r->d->calls += 1u;
    memcpy(&in, c, sizeof in);
    s = (int)pr_r0464_compose_08d4_cpu(b, PR_BUS_RAM_SIZE, &in, r->env->max_loopbacks, &r->d->compose);
    memcpy(c, &in, sizeof in);
    if (s == PR_R0464_COMPOSE_CPU_OK) return 0;
    r->d->site = site; r->d->status = s; return STATUS(CALLEE);
}

pr_ovl03ce_00f1_status pr_ovl03ce_00f1_cpu(pr_bus *b, size_t ram_size, pr_ovl03ce_00f1_regs *c,
                                           const pr_ovl03ce_00f1_env *env, pr_ovl03ce_00f1_diagnostic *d)
{
    pr_ovl03ce_00f1_diagnostic fallback;
    pr_run run, *dg = &run;
    uint16_t caller;
    if (b == NULL || b->ram == NULL || c == NULL || env == NULL || ram_size != PR_BUS_RAM_SIZE || c->ip != 0x00F1u)
        return STATUS(ARGUMENT);
    run.env = env; run.d = d != NULL ? d : &fallback;
    memset(run.d, 0, sizeof *run.d);
    run.d->status = -1; run.d->compose.move_status = -1;
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
targets = {ROOT / 'include/poolrad/recovered/overlay_u3ce_tile_00f1_cpu.h': header,
           ROOT / 'src/recovered/overlay_u3ce_tile_00f1_cpu.c': source}
if '--check' in sys.argv:
    bad = [str(p) for p, text in targets.items() if p.read_text() != text]
    print('EMIT_CHECK', 'MISMATCH ' + ', '.join(bad) if bad else 'OK'); sys.exit(1 if bad else 0)
for p, text in targets.items():
    p.write_text(text)
print('cases', len(e.ins))
