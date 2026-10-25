#!/usr/bin/env python3
"""Regenerate src/recovered/overlay_u38_handler_0400_cpu.c and its header with emit_raw_cpu_c.py.

python3 tools/overlay/emit_ovl038_0400.py [--check]   (--check compares instead of writing)
Needs the owner START.EXE and GAME.OVR under resources/. Prints the undefined-flag observability report.
The emitted body runs on flat RAM (the 009E dispatcher's handler convention): its rd8/wr8/rd16/wr16 take the RAM pointer."""
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
from overlay_inventory import find_units, parse_mz
from emit_raw_cpu_c import Emitter

ROOT = Path(__file__).resolve().parents[2]
exe = (ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes()
ovr = (ROOT / 'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
unit = {u['stub_para']: u for u in find_units(exe[parse_mz(exe)['header_bytes']:], ovr)}[0x38]
code = ovr[unit['ovr_offset']:unit['ovr_offset'] + unit['code_size']]
START, END = 0x0400, 0x04BA
e = Emitter(code[START:END], START, END, {0x23DB: 'call_party_count_23db'})
for a, mn, ops, und, vis in e.undefined_report():
    print(f'UNDEF {a:04X} {mn} {ops}: {und} -> {vis if vis else "never observed (overwritten on every path)"}')
cases = '\n'.join(e.cases())

header = '''#ifndef POOLRAD_RECOVERED_OVERLAY_U38_HANDLER_0400_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U38_HANDLER_0400_CPU_H
#include <stddef.h>
#include <stdint.h>
#include "poolrad/recovered/resident_00ba_party_count_23db_cpu.h"

/* Raw-CPU, full-state model of GAME.OVR unit 38h +[0400,04BA) (186 bytes, 59 instructions, RETF 0Ah), the handler that
 * the 009E dispatcher reaches through slot 11 (stub 0041:0057). Arguments: [BP+6] a byte selector, [BP+8] a far pointer A,
 * [BP+0Ch] a far pointer B.
 *   [BP+6] non-zero: B+10Eh := (A+3 AND 40h) >> 6; when B+84h = B3h, B+84h := 0. Return.
 *   [BP+6] zero, A+3 bit 5 set: DS:6825 := 64h. Return.
 *   [BP+6] zero, A+3 bit 5 clear: A+3 := low byte of (A+3 + 20h + CBW(B+10Eh) << 6); B+10Eh := A+3 >> 7; B+10Fh := 1;
 *     when B+84h <= 7Fh (unsigned), B+84h := B3h; the far pointer at B+108h is followed and its words +0Ah/+0Ch := 0;
 *     then 00BA:23DB (the party count) through its merged model; then DS:6825 := 64h.
 * Every far pointer is reread with LES at each use, in source order. Entered after a far call: [SP] IP, [SP+2] CS, then
 * the five argument words; RETF 0Ah. CS is the unit's load segment; the far-call segment word CS:04AF is a loader
 * fixup, read live. Flat 1 MiB A20-off RAM: offsets wrap at 16 bits, physical addresses at 1 MiB, word operands are
 * physically contiguous. See docs/re/OVL_0038_0400_CPU.md for the contract and evidence. */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u38_0400_cpu;

typedef enum {
    PR_U38_0400_OK = 0,         /* RETF 0Ah executed */
    PR_U38_0400_ARGUMENT,       /* null pointers, RAM not exactly 1 MiB, IP not 0400h; nothing changed */
    PR_U38_0400_CALLEE,         /* 23DB stopped (its status in *callee_status); state as it left it */
    PR_U38_0400_TRANSFER        /* 23DB returned somewhere other than the instruction after its call */
} pr_u38_0400_status;

/* party_budget: 23DB's instruction budget (its cap on a cyclic party list). callee_status (optional) receives -1 when 23DB
 * did not run, otherwise its last status. */
pr_u38_0400_status pr_u38_handler_0400_cpu(uint8_t *ram, size_t ram_size, pr_u38_0400_cpu *cpu, size_t party_budget,
                                           int *callee_status);
#endif
'''

source = '''/* GAME.OVR unit 38h +0400 (009E slot 11), raw CPU on flat RAM (docs/re/OVL_0038_0400_CPU.md).
 * One case per original instruction; operands are reread from RAM in source order. */
#include "poolrad/recovered/overlay_u38_handler_0400_cpu.h"
#include <string.h>
#define RAM_SIZE ((size_t)1u << 20)
#define MASK (RAM_SIZE - 1u)
#define STATUS(s) PR_U38_0400_##s
typedef pr_u38_0400_status pr_status_t;
typedef pr_u38_0400_cpu pr_cpu_t;
typedef struct {
    size_t budget;
    int *callee_status;
} pr_run;
_Static_assert(sizeof(pr_cpu_t) == sizeof(pr_res_23db_cpu), "CPU layout");
static size_t physical(uint16_t seg, uint16_t off) { return (((size_t)seg << 4u) + off) & MASK; }
static uint16_t word_at(uint8_t *b, size_t at) { return (uint16_t)(b[at] | ((uint16_t)b[(at + 1u) & MASK] << 8u)); }
static uint8_t rd8(uint8_t *b, uint16_t seg, uint16_t off) { return b[physical(seg, off)]; }
static uint16_t rd16(uint8_t *b, uint16_t seg, uint16_t off) { return word_at(b, physical(seg, off)); }
static void wr8(uint8_t *b, uint16_t seg, uint16_t off, uint8_t v) { b[physical(seg, off)] = v; }
static void wr16(uint8_t *b, uint16_t seg, uint16_t off, uint16_t v)
{
    size_t at = physical(seg, off);
    b[at] = (uint8_t)v; b[(at + 1u) & MASK] = (uint8_t)(v >> 8u);
}
static void push(uint8_t *b, pr_cpu_t *c, uint16_t v) { c->sp = (uint16_t)(c->sp - 2u); wr16(b, c->ss, c->sp, v); }
static uint16_t pop(uint8_t *b, pr_cpu_t *c) { uint16_t v = rd16(b, c->ss, c->sp); c->sp = (uint16_t)(c->sp + 2u); return v; }
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
/* SHL/SHR, count masked to 5 bits; a zero count changes nothing. CF is the last bit out; OF is
 * MSB(v<<(n-1)) ^ MSB(result) for SHL (pinned t_SHLw: MSB(result) ^ CF) and MSB(v>>(n-1)) ^ MSB(result) for SHR (0 above
 * count 1, as pinned t_SHRw); AF := 1 (pinned DOSBox-X; Unicorn clears it). The emitter's report shows neither is observed. */
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
/* 00BA:23DB, the party count, on the same RAM (its merged raw model). A non-RETURNED status stops here. */
static int call_party_count_23db(uint8_t *b, pr_cpu_t *c, pr_run *r)
{
    pr_res_23db_cpu in; size_t budget = r->budget; int s;
    memcpy(&in, c, sizeof in);
    s = (int)pr_res_00ba_party_count_23db_cpu(b, RAM_SIZE, &in, &budget);
    memcpy(c, &in, sizeof in);
    if (r->callee_status != NULL) *r->callee_status = s;
    return s == PR_RES_23DB_RETURNED ? 0 : STATUS(CALLEE);
}

pr_u38_0400_status pr_u38_handler_0400_cpu(uint8_t *b, size_t ram_size, pr_u38_0400_cpu *c, size_t party_budget,
                                           int *callee_status)
{
    pr_run run, *dg = &run;
    uint16_t caller;
    if (b == NULL || c == NULL || ram_size != RAM_SIZE || c->ip != 0x0400u)
        return STATUS(ARGUMENT);
    run.budget = party_budget; run.callee_status = callee_status;
    if (callee_status != NULL) *callee_status = -1;
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
targets = {ROOT / 'include/poolrad/recovered/overlay_u38_handler_0400_cpu.h': header,
           ROOT / 'src/recovered/overlay_u38_handler_0400_cpu.c': source}
if '--check' in sys.argv:
    bad = [str(p) for p, text in targets.items() if p.read_text() != text]
    print('EMIT_CHECK', 'MISMATCH ' + ', '.join(bad) if bad else 'OK'); sys.exit(1 if bad else 0)
for p, text in targets.items():
    p.write_text(text)
print('cases', len(e.ins))
