#!/usr/bin/env python3
"""Regenerate src/recovered/overlay_u38_070f_cpu.c and its header with emit_raw_cpu_c.py (generator, not a row checker).

python3 tools/overlay/emit_ovl038_070f.py [--check]   (--check compares instead of writing)
Needs the owner START.EXE and GAME.OVR under resources/ (read only; nothing is executed). Prints the span facts, the two far
call sites with their fixup status, the CS:0703 source string and the undefined-flag report (empty: no SHL/SHR/MUL).
The emitted body runs on flat RAM (the 009E dispatcher's handler convention)."""
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
START, END, NEXT = 0x070F, 0x0764, 0x0775
CALLS = {0x0B0B: 'call_strload_0b0b', 0x16EC: 'stop_res_16ec'}

e = Emitter(code[START:END], START, END, CALLS)
body = code[START:END]
assert e.ins[-1].mnemonic == 'retf' and e.ins[-1].op_str == '0xa' and [i.mnemonic for i in e.ins].count('retf') == 1
print(f'span +[{START:04X},{END:04X}) bytes {len(body)} insns {len(e.ins)} sha256 {hashlib.sha256(body).hexdigest()}')
print(f'file offset 0x{unit["ovr_offset"] + START:05X}; public entry index {unit["entries"].index(START)}; unit segment 0x{unit["memory_segment"]:02X}')
print(f'trailing constant +[{END:04X},{NEXT:04X}) {code[END:NEXT]!r} (length-prefixed, not code; the ledger bound 102 includes it)')
src = code[0x0703:START]
print(f'source string CS:0703 {src!r} length byte {src[0]} = {len(src) - 1} chars; local buffer SS:BP-0Ch is 12 bytes')
for i in e.ins:
    if i.mnemonic == 'lcall':
        seg_word = i.address + 3
        print(f'CALL {i.address:04X} {i.mnemonic} {i.op_str}; segment word CS:{seg_word:04X} fixup={seg_word in unit["fixups"]}')
und = e.undefined_report()
for a, mn, ops, u, vis in und:
    print(f'UNDEF {a:04X} {mn} {ops}: {u} -> {vis if vis else "never observed (overwritten on every path)"}')
print('UNDEF shift/mul producers: none' if not und else f'UNDEF producers {len(und)}')
cases = '\n'.join(e.cases())

header = '''#ifndef POOLRAD_RECOVERED_OVERLAY_U38_070F_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U38_070F_CPU_H
#include <stddef.h>
#include <stdint.h>
#include "poolrad/recovered/system_string_leaves_cpu.h"

/* Raw-CPU model of GAME.OVR unit 38h +[070F,0764) (85 bytes, 30 instructions, RETF 0Ah): public entry 19, stub
 * 0041:007F, the 009E dispatcher's slot-21 handler. The ledger's 102-byte bound (98 decoded) runs into the next entry's
 * length-prefixed constant "dies from poison" at +0764, which is not code. Entered after a far call: [SP] IP, [SP+2] CS,
 * then five argument words of which only [BP+0Ch]/[BP+0Eh] (far pointer B) are read; [BP+6] and the far pointer at
 * [BP+8] are never read. PUSH BP; MOV BP,SP; SUB SP,0Ch (a 12-byte local L at SS:BP-0Ch).
 *   1. C := the far pointer at B+108h (B and C reread with LES at every use). CMP byte C+2, 0; equal: step 4.
 *   2. Push B; push SS:L; push CS:0703; LCALL 0AF8:0B0B (SYSTEM StrLoad; segment word CS:0737 is a loader fixup):
 *      L := the 11-character Pascal string at CS:0703 ("is silenced", 12 bytes), through the accepted leaf model
 *      pr_system_strload_0b0b (RETF 4 pops the source and leaves SS:L on the stack; AL := 0Bh, CX := 0, DX := DS,
 *      BX := its entry SP, DF cleared, flags XOR CH,CH's; AH keeps the entry AH).
 *   3. Push AX with AL = 0Ah (row), push AX with AL = 1 (wait); LCALL 00BA:16EC (segment word CS:0742 is a fixup): the
 *      resident window message with (wait 1, row 0Ah, message SS:L, B's two words for 1816), RETF 0Ch. The model
 *      STOPS here (STOP_16EC) with the CPU exactly as the LCALL left it: the return frame (CS, 0744) pushed under the
 *      six argument words, CS the live word at CS:0742, IP 16ECh. A merged bounded bus-domain model of 16EC exists
 *      (pr_res_00ba_window_16ec_cpu); composing it is a separate bounded step, not done here. Nothing is rolled back.
 *   4. (Reached directly when C+2 = 0; after 16EC otherwise, unreachable in this model.) C+2 := 0; C+1 := 0 (B and C
 *      reread before each store). MOV SP,BP; POP BP; RETF 0Ah.
 * CS is the unit's load segment. Flat 1 MiB A20-off RAM: offsets wrap at 16 bits, physical addresses at 1 MiB, word
 * operands are physically contiguous. No backward branch, so no instruction budget of its own.
 * Composition profile (full CPU14 + RAM state is claimed INSIDE it only): the CPU and diagnostic objects are distinct
 * from RAM; loaded code and bindings (CS:0703..070E, CS:0737, CS:0742) are read live and immutable during the run;
 * StrLoad's accepted contract applies to the copy (SI/DI wrap at 16 bits, L and the source may alias); the 16EC entry
 * SP precondition of that model ([0800h,FFF0h]) is NOT checked here because 16EC is not entered; interrupts, faults
 * and self-modifying code are outside this synchronous profile. Flags: SUB SP,0Ch at 0712 is overwritten by CMP at 071D
 * before any read; on the OK path the return flags are CMP C+2,0's; at STOP_16EC they are StrLoad's. No flag of this
 * body is architecturally undefined (SUB/CMP only). See docs/re/OVL_0038_070F_CPU.md. */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u38_070f_cpu;

typedef enum {
    PR_U38_070F_OK = 0,         /* RETF 0Ah executed (the C+2 = 0 path) */
    PR_U38_070F_ARGUMENT,       /* null pointers, RAM not exactly 1 MiB, IP not 070Fh; nothing changed */
    PR_U38_070F_CALLEE,         /* StrLoad did not return (its status in diagnostic.callee_status); state as it left it */
    PR_U38_070F_TRANSFER,       /* StrLoad returned somewhere other than CS:0739 */
    PR_U38_070F_STOP_16EC       /* the LCALL at 073F executed; CPU at the callee entry (RES_00BA_16EC not composed) */
} pr_u38_070f_status;

typedef struct {
    int callee;                 /* -1 none; 1 = SYSTEM 0B0B StrLoad; 2 = 00BA:16EC */
    int callee_status;          /* StrLoad's status when callee is 1, otherwise -1 */
    uint16_t site;              /* IP of the last LCALL entered, 0 when none */
} pr_u38_070f_diagnostic;

/* diagnostic may be NULL. */
pr_u38_070f_status pr_u38_070f_cpu_run(uint8_t *ram, size_t ram_size, pr_u38_070f_cpu *cpu, pr_u38_070f_diagnostic *diagnostic);
#endif
'''

source = '''/* GAME.OVR unit 38h +070F (009E slot 21, public entry 19), raw CPU on flat RAM (docs/re/OVL_0038_070F_CPU.md).
 * One case per original instruction; operands are reread from RAM in source order. Generated by
 * tools/overlay/emit_ovl038_070f.py; do not edit by hand. */
#include "poolrad/recovered/overlay_u38_070f_cpu.h"
#include <string.h>
#define RAM_SIZE ((size_t)1u << 20)
#define MASK (RAM_SIZE - 1u)
#define STATUS(s) PR_U38_070F_##s
typedef pr_u38_070f_status pr_status_t;
typedef pr_u38_070f_cpu pr_cpu_t;
typedef struct {
    pr_u38_070f_diagnostic *d;
} pr_run;
_Static_assert(sizeof(pr_cpu_t) == sizeof(pr_system_leaf_cpu), "CPU layout");
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
/* SYSTEM 0AF8:0B0B StrLoad through its accepted leaf model, on the same RAM. A non-RETURNED status stops here. */
static int call_strload_0b0b(uint8_t *b, pr_cpu_t *c, pr_run *r)
{
    pr_system_leaf_cpu in; int s;
    memcpy(&in, c, sizeof in);
    s = (int)pr_system_strload_0b0b(b, RAM_SIZE, &in);
    memcpy(c, &in, sizeof in);
    r->d->callee = 1; r->d->callee_status = s; r->d->site = 0x0734u;
    return s == PR_SYSTEM_LEAF_RETURNED ? 0 : STATUS(CALLEE);
}
/* LCALL 00BA:16EC at 073F: the resident window message. Not composed: stop with the CPU at the callee entry. */
static int stop_res_16ec(uint8_t *b, pr_cpu_t *c, pr_run *r)
{
    (void)b; (void)c; r->d->callee = 2; r->d->callee_status = -1; r->d->site = 0x073Fu;
    return STATUS(STOP_16EC);
}

pr_u38_070f_status pr_u38_070f_cpu_run(uint8_t *b, size_t ram_size, pr_u38_070f_cpu *c, pr_u38_070f_diagnostic *diagnostic)
{
    pr_u38_070f_diagnostic local;
    pr_run run, *dg = &run;
    uint16_t caller;
    run.d = diagnostic != NULL ? diagnostic : &local;
    run.d->callee = -1; run.d->callee_status = -1; run.d->site = 0u;
    if (b == NULL || c == NULL || ram_size != RAM_SIZE || c->ip != 0x070Fu)
        return STATUS(ARGUMENT);
    caller = c->cs;
    for (;;) {
        switch (c->ip) {
CASES
        default: return STATUS(TRANSFER);
        }
    }
}
'''.replace('CASES', cases)
targets = {ROOT / 'include/poolrad/recovered/overlay_u38_070f_cpu.h': header,
           ROOT / 'src/recovered/overlay_u38_070f_cpu.c': source}
if '--check' in sys.argv:
    bad = [str(p) for p, text in targets.items() if not p.exists() or p.read_text() != text]
    print('EMIT_CHECK', 'MISMATCH ' + ', '.join(bad) if bad else 'OK'); sys.exit(1 if bad else 0)
for p, text in targets.items():
    p.write_text(text)
print('cases', len(e.ins))
