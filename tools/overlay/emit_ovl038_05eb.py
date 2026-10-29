#!/usr/bin/env python3
"""Regenerate src/recovered/overlay_u38_05eb_cpu.c and its header with emit_raw_cpu_c.py (generator, not a row checker).

python3 tools/overlay/emit_ovl038_05eb.py [--check]   (--check compares instead of writing)
Needs the owner START.EXE and GAME.OVR under resources/ (read only; nothing is executed). Prints the span facts, the three
call sites with their fixup status and the undefined-flag observability report (empty: the body has no SHL/SHR/MUL).
The emitted body runs on flat RAM (the 009E dispatcher's handler convention). The shared emitter has no near-call case;
NearCallEmitter adds `CALL rel16` (push the return IP, enter the callee's model, require the return at the next IP)."""
import hashlib
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
from overlay_inventory import find_units, parse_mz
from emit_raw_cpu_c import Emitter

ROOT = Path(__file__).resolve().parents[2]
exe = (ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes()
ovr = (ROOT / 'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
assert hashlib.sha256(ovr).hexdigest() == '872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614', 'GAME.OVR identity'
assert hashlib.sha256(exe).hexdigest() == 'd825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd', 'START.EXE identity'
unit = {u['stub_para']: u for u in find_units(exe[parse_mz(exe)['header_bytes']:], ovr)}[0x38]
code = ovr[unit['ovr_offset']:unit['ovr_offset'] + unit['code_size']]
START, END = 0x05EB, 0x0647
CALLS = {0x001F: 'call_gated_append_001f', 0x007F: 'stop_u9e_1344', 0x0767: 'stop_res_0767'}


class NearCallEmitter(Emitter):
    def case(self, i):
        if i.mnemonic == 'call':
            t, nx = int(i.op_str, 16), self.next[i.address]
            handler = self.calls[t]
            return (f'push(b,c,0x{nx:04X}u);c->ip=0x{t:04X}u;{{int s={handler}(b,c,dg);if(s)return (pr_status_t)s;}}'
                    f'if(c->cs!=caller||c->ip!=0x{nx:04X}u)return STATUS(TRANSFER);')
        return super().case(i)


e = NearCallEmitter(code[START:END], START, END, CALLS)
body = code[START:END]
print(f'span +[{START:04X},{END:04X}) bytes {len(body)} insns {len(e.ins)} sha256 {hashlib.sha256(body).hexdigest()}')
print(f'file offset 0x{unit["ovr_offset"] + START:05X}; public entry index {unit["entries"].index(START)}; unit segment 0x{unit["memory_segment"]:02X}')
for i in e.ins:
    if i.mnemonic in ('call', 'lcall'):
        seg_word = i.address + 3 if i.mnemonic == 'lcall' else None
        fix = '' if seg_word is None else f'; segment word CS:{seg_word:04X} fixup={seg_word in unit["fixups"]}'
        print(f'CALL {i.address:04X} {i.mnemonic} {i.op_str}{fix}')
und = e.undefined_report()
for a, mn, ops, u, vis in und:
    print(f'UNDEF {a:04X} {mn} {ops}: {u} -> {vis if vis else "never observed (overwritten on every path)"}')
print('UNDEF none' if not und else f'UNDEF producers {len(und)}')
cases = '\n'.join(e.cases())

header = '''#ifndef POOLRAD_RECOVERED_OVERLAY_U38_05EB_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U38_05EB_CPU_H
#include <stddef.h>
#include <stdint.h>
#include "poolrad/recovered/overlay_u38_gated_append_001f_cpu.h"

/* Raw-CPU, full-state model of GAME.OVR unit 38h +[05EB,0647) (92 bytes, 35 instructions, RETF 0Ah): public entry 14,
 * stub 0041:0066, the 009E dispatcher's slot-15 handler. Entered after a far call: [SP] IP, [SP+2] CS, then five argument
 * words: [BP+6] a word this body never reads, [BP+8] far pointer A, [BP+0Ch] far pointer B.
 *   1. PUSH CS / CALL 001F: this unit's +001F with (a = 0Ah, c = byte A+3, b = 0Fh, record B), composed through its raw
 *      model pr_u38_gated_append_001f_cpu (DS:[681F] gates an append through the live stub 0052 / U9E +0E52 / GetMem).
 *      The two byte arguments are pushed as AX, so the entry AH sits in the high bytes of those stack words. The frame
 *      001F sees is exactly a far call's (PUSH CS supplies the segment; 001F's RETF 0Ah pops both and the five words).
 *   2. AL = 0 (001F stored result 0): return. B+11Bh <= 1 (unsigned, B reread with LES): return. Otherwise
 *      DS:[6819] := 0 and LCALL 00B0:007F with (B, 1, 0, 0), again as AX pushes with AH as 001F left it. The segment
 *      word CS:062D is a loader fixup (unit 9Eh's stub); stub offset 007F is entry 19, OVL_009E_1344, which has no model:
 *      the model STOPS there (STOP_1344) with the CPU exactly as the LCALL left it: the return frame (CS, 062F) pushed,
 *      CS the live word at CS:062D, IP 007F. Nothing is rolled back.
 *   3. (Unreachable while step 2 stops.) DS:[49F3] = 5: return; else LCALL 00BA:0767 (segment word CS:0641, a fixup) with
 *      the far pointer DS:[5D92]/[5D94]: STOP_0767, the same saved-state convention.
 * CS is the unit's load segment. Flat 1 MiB A20-off RAM: offsets wrap at 16 bits, physical addresses at 1 MiB, word
 * operands are physically contiguous. There is no backward branch, so no instruction budget of its own. See
 * docs/re/OVL_0038_05EB_CPU.md for the contract and evidence. */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u38_05eb_cpu;

typedef enum {
    PR_U38_05EB_OK = 0,         /* RETF 0Ah executed */
    PR_U38_05EB_ARGUMENT,       /* null pointers, RAM not exactly 1 MiB, IP not 05EBh; nothing changed */
    PR_U38_05EB_CALLEE,         /* 001F stopped (its status in diagnostic.callee_status); state as it left it */
    PR_U38_05EB_TRANSFER,       /* 001F returned somewhere other than CS:0607 */
    PR_U38_05EB_STOP_1344,      /* the LCALL at 062A executed; CPU at the callee entry (no model for OVL_009E_1344) */
    PR_U38_05EB_STOP_0767       /* the LCALL at 063E executed; CPU at the callee entry (no raw model for RES_00BA_0767) */
} pr_u38_05eb_status;

typedef struct {
    unsigned max_heap_steps;    /* 001F's GetMem walk cap */
    size_t max_links;           /* 001F's list-append link cap */
} pr_u38_05eb_limits;

typedef struct {
    int callee;                 /* -1 none; 1 = 001F (near); 2 = 00B0:007F (OVL_009E_1344); 3 = 00BA:0767 */
    int callee_status;          /* 001F's status when callee is 1, otherwise -1 */
    uint16_t site;              /* IP of the last call instruction entered, 0 when none */
    pr_u38_001f_diagnostic d001f;
} pr_u38_05eb_diagnostic;

/* limits may be NULL (both caps zero: 001F's dependency meanings apply). diagnostic may be NULL. */
pr_u38_05eb_status pr_u38_05eb_cpu_run(uint8_t *ram, size_t ram_size, pr_u38_05eb_cpu *cpu,
                                       const pr_u38_05eb_limits *limits, pr_u38_05eb_diagnostic *diagnostic);
#endif
'''

source = '''/* GAME.OVR unit 38h +05EB (009E slot 15, public entry 14), raw CPU on flat RAM (docs/re/OVL_0038_05EB_CPU.md).
 * One case per original instruction; operands are reread from RAM in source order. Generated by
 * tools/overlay/emit_ovl038_05eb.py; do not edit by hand. */
#include "poolrad/recovered/overlay_u38_05eb_cpu.h"
#include <string.h>
#define RAM_SIZE ((size_t)1u << 20)
#define MASK (RAM_SIZE - 1u)
#define STATUS(s) PR_U38_05EB_##s
typedef pr_u38_05eb_status pr_status_t;
typedef pr_u38_05eb_cpu pr_cpu_t;
typedef struct {
    pr_u38_05eb_limits limits;
    pr_u38_05eb_diagnostic *d;
} pr_run;
_Static_assert(sizeof(pr_cpu_t) == sizeof(pr_u38_001f_cpu), "CPU layout");
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
/* Unit 38h +001F through its raw model, on the same RAM. Entered with the near-call frame already pushed (PUSH CS at
 * 0603, the return IP at 0604), which is the far-call frame 001F's RETF 0Ah expects. A non-OK status stops here. */
static int call_gated_append_001f(uint8_t *b, pr_cpu_t *c, pr_run *r)
{
    pr_u38_001f_cpu in; int s;
    memcpy(&in, c, sizeof in);
    s = (int)pr_u38_gated_append_001f_cpu(b, RAM_SIZE, &in, r->limits.max_heap_steps, r->limits.max_links, &r->d->d001f);
    memcpy(c, &in, sizeof in);
    r->d->callee = 1; r->d->callee_status = s; r->d->site = 0x0604u;
    return s == PR_U38_001F_OK ? 0 : STATUS(CALLEE);
}
/* LCALL 00B0:007F at 062A: unit 9Eh's stub, entry 19 (OVL_009E_1344). No model: stop with the CPU at the callee entry. */
static int stop_u9e_1344(uint8_t *b, pr_cpu_t *c, pr_run *r)
{
    (void)b; (void)c; r->d->callee = 2; r->d->callee_status = -1; r->d->site = 0x062Au;
    return STATUS(STOP_1344);
}
/* LCALL 00BA:0767 at 063E: the resident load-pieces routine. No raw model: stop with the CPU at the callee entry. */
static int stop_res_0767(uint8_t *b, pr_cpu_t *c, pr_run *r)
{
    (void)b; (void)c; r->d->callee = 3; r->d->callee_status = -1; r->d->site = 0x063Eu;
    return STATUS(STOP_0767);
}

pr_u38_05eb_status pr_u38_05eb_cpu_run(uint8_t *b, size_t ram_size, pr_u38_05eb_cpu *c,
                                       const pr_u38_05eb_limits *limits, pr_u38_05eb_diagnostic *diagnostic)
{
    pr_u38_05eb_diagnostic local;
    pr_run run, *dg = &run;
    uint16_t caller;
    run.d = diagnostic != NULL ? diagnostic : &local;
    run.d->callee = -1; run.d->callee_status = -1; run.d->site = 0u;
    run.d->d001f.append_status = -1; run.d->d001f.getmem_status = -1;
    if (b == NULL || c == NULL || ram_size != RAM_SIZE || c->ip != 0x05EBu)
        return STATUS(ARGUMENT);
    run.limits.max_heap_steps = limits != NULL ? limits->max_heap_steps : 0u;
    run.limits.max_links = limits != NULL ? limits->max_links : 0u;
    caller = c->cs;
    for (;;) {
        switch (c->ip) {
CASES
        default: return STATUS(TRANSFER);
        }
    }
}
'''.replace('CASES', cases)
targets = {ROOT / 'include/poolrad/recovered/overlay_u38_05eb_cpu.h': header,
           ROOT / 'src/recovered/overlay_u38_05eb_cpu.c': source}
if '--check' in sys.argv:
    bad = [str(p) for p, text in targets.items() if not p.exists() or p.read_text() != text]
    print('EMIT_CHECK', 'MISMATCH ' + ', '.join(bad) if bad else 'OK'); sys.exit(1 if bad else 0)
for p, text in targets.items():
    p.write_text(text)
print('cases', len(e.ins))
