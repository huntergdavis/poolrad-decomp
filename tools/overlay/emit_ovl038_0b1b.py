#!/usr/bin/env python3
"""Regenerate src/recovered/overlay_u38_0b1b_cpu.c and its header with emit_raw_cpu_c.py (generator, not a row checker).

python3 tools/overlay/emit_ovl038_0b1b.py [--check]   (--check compares instead of writing)
Needs the owner START.EXE and GAME.OVR under resources/ (read only; nothing is executed). Prints the span facts, the far
call site with its fixup status, the near call site, the CS:0B11 source string and the undefined-flag report (one SHR
producer, never observed). The emitted body runs on flat RAM (the 009E dispatcher's handler convention)."""
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
START, END = 0x0B1B, 0x0BAC
CALLS = {0x0B0B: 'call_strload_0b0b', 0x0058: 'stop_near_0058'}


class NearCallEmitter(Emitter):
    """emit_raw_cpu_c has no near CALL case: push the return offset, set IP, run the handler, then require the
    handler to leave the CPU at the return address in this segment (the handler stops here, so it never does)."""
    def case(self, i):
        if i.mnemonic == 'call':
            t, nx = int(i.op_str, 16), self.next[i.address]
            handler = self.calls[t]
            return (f'push(b,c,0x{nx:04X}u);c->ip=0x{t:04X}u;{{int s={handler}(b,c,dg);if(s)return (pr_status_t)s;}}'
                    f'if(c->cs!=caller||c->ip!=0x{nx:04X}u)return STATUS(TRANSFER);')
        return super().case(i)


e = NearCallEmitter(code[START:END], START, END, CALLS)
body = code[START:END]
assert e.ins[-1].mnemonic == 'retf' and e.ins[-1].op_str == '0xa' and [i.mnemonic for i in e.ins].count('retf') == 1
assert not any(int(i.op_str, 16) <= i.address for i in e.ins if i.mnemonic in ('jmp', 'jne', 'je')), 'backward branch'
print(f'span +[{START:04X},{END:04X}) bytes {len(body)} insns {len(e.ins)} sha256 {hashlib.sha256(body).hexdigest()}')
print(f'file offset 0x{unit["ovr_offset"] + START:05X}; public entry index {unit["entries"].index(START)}; unit segment 0x{unit["memory_segment"]:02X}')
print(f'next entry +{END:04X} starts with {code[END:END + 3].hex()} (PUSH BP; MOV BP,SP: code, no trailing constant; the ledger bound 145 is exact)')
src = code[0x0B11:START]
print(f'source string CS:0B11 {src!r} length byte {src[0]} = {len(src) - 1} chars; local buffer SS:BP-0Ah is 10 bytes')
for i in e.ins:
    if i.mnemonic == 'lcall':
        seg_word = i.address + 3
        print(f'CALL {i.address:04X} {i.mnemonic} {i.op_str}; segment word CS:{seg_word:04X} fixup={seg_word in unit["fixups"]}')
    if i.mnemonic == 'call':
        print(f'CALL {i.address:04X} near {i.op_str} (preceded by PUSH CS at {i.address - 1:04X}: a far frame for a RETF callee)')
und = e.undefined_report()
for a, mn, ops, u, vis in und:
    print(f'UNDEF {a:04X} {mn} {ops}: {u} -> {vis if vis else "never observed (overwritten on every path)"}')
assert len(und) == 1 and und[0][0] == 0x0B57 and not und[0][4], 'exactly one unobserved SHR producer expected'
cases = '\n'.join(e.cases())

header = '''#ifndef POOLRAD_RECOVERED_OVERLAY_U38_0B1B_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U38_0B1B_CPU_H
#include <stddef.h>
#include <stdint.h>
#include "poolrad/recovered/system_string_leaves_cpu.h"

/* Raw-CPU model of GAME.OVR unit 38h +[0B1B,0BAC) (145 bytes, 47 instructions, RETF 0Ah): public entry 27, stub
 * 0041:00A7, the 009E dispatcher's slot-32 handler. The ledger's 145-byte bound is exact (the next entry +0BAC begins
 * with code). Entered after a far call: [SP] IP, [SP+2] CS, then five argument words: [BP+6] (never read), the far
 * pointer A at [BP+8]/[BP+0Ah] and the far pointer B at [BP+0Ch]/[BP+0Eh]; A and B are reread with LES at every use.
 * PUSH BP; MOV BP,SP; SUB SP,0Ah (a 10-byte local L at SS:BP-0Ah).
 *   1. A+4 := 0.
 *   2. CMP byte DS:[681Fh], 0; not equal: step 4.
 *   3. Push B; AL := 6, push AX (the entry AH rides in the high byte); push SS:L; push CS:0B11; LCALL 0AF8:0B0B
 *      (SYSTEM StrLoad; segment word CS:0B46 is a loader fixup): L := the 9-character Pascal string at CS:0B11
 *      ("collapses", 10 bytes), through the accepted leaf model pr_system_strload_0b0b (RETF 4 pops the source pair and
 *      leaves SS:L; AL := 09h, CX := 0, DX := DS, BX := its entry SP, SI/DI advanced, DF cleared, flags XOR CH,CH's; AH
 *      kept). Then PUSH CS; CALL 0058 (near, with a far return frame): the unit-local routine +0058 (181 bytes, itself
 *      reaching 00BA:16EC, 00B0:0061/002F, 03E0:0075, 0709:0DBE, 00BA:17E3 and 00BA:0767; no accepted model) receives
 *      (B, the AX word, SS:L) and would balance them with its RETF 0Ah. The model STOPS here (STOP_0058) with the CPU
 *      exactly as the CALL left it: CS and 0B4C pushed under the five argument words, CS unchanged, IP 0058h. Nothing is
 *      rolled back; the post-call steps are not reached.
 *   4. (Directly when DS:[681Fh] != 0; after 0058 otherwise, unreachable in this model.) AL := byte A+3; AH := 0; CL :=
 *      4; SHR AX,CL; B+10Eh := AL; B+10Fh := 1; B+76h := 0; B+6Bh := byte B+98h; B+72h := 0Ch; CMP byte B+84h, 0B3h;
 *      equal: B+84h := 0; B+9Fh := 0. MOV SP,BP; POP BP; RETF 0Ah.
 * CS is the unit's load segment. Flat 1 MiB A20-off RAM: offsets wrap at 16 bits, physical addresses at 1 MiB, word
 * operands are physically contiguous. No backward branch, so no instruction budget of its own.
 * Composition profile (full CPU14 + RAM state is claimed INSIDE it only): the CPU and diagnostic objects are distinct
 * from RAM; loaded code and bindings (CS:0B11..0B1A, CS:0B46) are read live and immutable during the run; StrLoad's
 * accepted contract applies to the copy (SI/DI wrap at 16 bits, L and the source may alias); +0058's own entry
 * preconditions are NOT checked because it is not entered; interrupts, faults and self-modifying code are outside this
 * synchronous profile. Flags: SUB SP,0Ah (0B1E) is overwritten by CMP DS:[681Fh],0 (0B29), which the JNE reads; on the
 * STOP_0058 path the flags are StrLoad's; on the OK path XOR AH,AH (0B53) and SHR AX,CL (0B57; OF for a count of 4
 * and AF are architecturally undefined, pinned here as the emitter's rule with AF := 1) are overwritten by CMP
 * B+84h,0B3h (0B8C) before any read, and the return flags are that CMP's. See docs/re/OVL_0038_0B1B_CPU.md. */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u38_0b1b_cpu;

typedef enum {
    PR_U38_0B1B_OK = 0,         /* RETF 0Ah executed (the DS:[681Fh] != 0 path) */
    PR_U38_0B1B_ARGUMENT,       /* null pointers, RAM not exactly 1 MiB, IP not 0B1Bh; nothing changed */
    PR_U38_0B1B_CALLEE,         /* StrLoad did not return (its status in diagnostic.callee_status); state as it left it */
    PR_U38_0B1B_TRANSFER,       /* StrLoad returned somewhere other than CS:0B48 */
    PR_U38_0B1B_STOP_0058       /* the CALL at 0B49 executed; CPU at the callee entry (unit 38h +0058 not composed) */
} pr_u38_0b1b_status;

typedef struct {
    int callee;                 /* -1 none; 1 = SYSTEM 0B0B StrLoad; 2 = unit 38h +0058 */
    int callee_status;          /* StrLoad's status when callee is 1, otherwise -1 */
    uint16_t site;              /* IP of the last call entered (0B43 or 0B49), 0 when none */
} pr_u38_0b1b_diagnostic;

/* diagnostic may be NULL. */
pr_u38_0b1b_status pr_u38_0b1b_cpu_run(uint8_t *ram, size_t ram_size, pr_u38_0b1b_cpu *cpu, pr_u38_0b1b_diagnostic *diagnostic);
#endif
'''

source = '''/* GAME.OVR unit 38h +0B1B (009E slot 32, public entry 27), raw CPU on flat RAM (docs/re/OVL_0038_0B1B_CPU.md).
 * One case per original instruction; operands are reread from RAM in source order. Generated by
 * tools/overlay/emit_ovl038_0b1b.py; do not edit by hand. */
#include "poolrad/recovered/overlay_u38_0b1b_cpu.h"
#include <string.h>
#define RAM_SIZE ((size_t)1u << 20)
#define MASK (RAM_SIZE - 1u)
#define STATUS(s) PR_U38_0B1B_##s
typedef pr_u38_0b1b_status pr_status_t;
typedef pr_u38_0b1b_cpu pr_cpu_t;
typedef struct {
    pr_u38_0b1b_diagnostic *d;
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
/* AND/OR/XOR: CF, OF and AF cleared (AF := 0 is a deterministic convention, not a native-equality claim); SZP set. */
static uint16_t logic(pr_cpu_t *c, uint16_t v, unsigned bits)
{
    v = (uint16_t)(v & (bits == 8u ? 0xFFu : 0xFFFFu)); c->flags &= (uint16_t)~0x08D5u; szp(c, v, bits); return v;
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
/* SYSTEM 0AF8:0B0B StrLoad through its accepted leaf model, on the same RAM. A non-RETURNED status stops here. */
static int call_strload_0b0b(uint8_t *b, pr_cpu_t *c, pr_run *r)
{
    pr_system_leaf_cpu in; int s;
    memcpy(&in, c, sizeof in);
    s = (int)pr_system_strload_0b0b(b, RAM_SIZE, &in);
    memcpy(c, &in, sizeof in);
    r->d->callee = 1; r->d->callee_status = s; r->d->site = 0x0B43u;
    return s == PR_SYSTEM_LEAF_RETURNED ? 0 : STATUS(CALLEE);
}
/* PUSH CS; CALL 0058 at 0B48/0B49: the unit-local routine +0058. Not composed: stop with the CPU at the callee entry. */
static int stop_near_0058(uint8_t *b, pr_cpu_t *c, pr_run *r)
{
    (void)b; (void)c; r->d->callee = 2; r->d->callee_status = -1; r->d->site = 0x0B49u;
    return STATUS(STOP_0058);
}

pr_u38_0b1b_status pr_u38_0b1b_cpu_run(uint8_t *b, size_t ram_size, pr_u38_0b1b_cpu *c, pr_u38_0b1b_diagnostic *diagnostic)
{
    pr_u38_0b1b_diagnostic local;
    pr_run run, *dg = &run;
    uint16_t caller;
    run.d = diagnostic != NULL ? diagnostic : &local;
    run.d->callee = -1; run.d->callee_status = -1; run.d->site = 0u;
    if (b == NULL || c == NULL || ram_size != RAM_SIZE || c->ip != 0x0B1Bu)
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
targets = {ROOT / 'include/poolrad/recovered/overlay_u38_0b1b_cpu.h': header,
           ROOT / 'src/recovered/overlay_u38_0b1b_cpu.c': source}
if '--check' in sys.argv:
    bad = [str(p) for p, text in targets.items() if not p.exists() or p.read_text() != text]
    print('EMIT_CHECK', 'MISMATCH ' + ', '.join(bad) if bad else 'OK'); sys.exit(1 if bad else 0)
for p, text in targets.items():
    p.write_text(text)
print('cases', len(e.ins))
