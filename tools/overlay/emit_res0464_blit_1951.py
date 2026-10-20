#!/usr/bin/env python3
"""Regenerate src/recovered/resident_0464_blit_1951_cpu.c and its header with emit_raw_cpu_c.py.

python3 tools/overlay/emit_res0464_blit_1951.py [--check]   (--check compares instead of writing)
Needs the owner START.EXE under resources/. Prints the undefined-flag observability report."""
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
from resident_loader_map import unpack
from emit_raw_cpu_c import Emitter

ROOT = Path(__file__).resolve().parents[2]
img = bytes(unpack((ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes())[0])
START, END = 0x1951, 0x1B52
e = Emitter(img[0x4640 + START:0x4640 + END], START, END, {0x1685: 'call_move'})
for a, mn, ops, und, vis in e.undefined_report():
    print(f'UNDEF {a:04X} {mn} {ops}: {und} -> {vis if vis else "never observed (overwritten on every path)"}')
cases = '\n'.join(e.cases())

header = '''#ifndef POOLRAD_RECOVERED_RESIDENT_0464_BLIT_1951_CPU_H
#define POOLRAD_RECOVERED_RESIDENT_0464_BLIT_1951_CPU_H
#include <stddef.h>
#include <stdint.h>
#include "poolrad/recovered/memory_bus.h"

/* Raw-CPU, full-state model of START.EXE resident 0464:[1951,1B52) (513 bytes,
 * 201 instructions, RETF 8): a bitmap blit selected by the video-mode byte
 * DS:4A1E (0: row table DS:4EC0 and SYSTEM Move; 1: EGA sequencer map-mask
 * OUTs 3C4/3C5 with A000 stores through row table DS:5050 and plane masks
 * DS:25F6; 2: row table DS:51E0 and SYSTEM Move; other values return).
 * Entered after a far call: [SP] IP, [SP+2] CS, [SP+4] far bitmap pointer
 * (null returns at once), [SP+8] row argument, [SP+0Ah] column argument.
 * See docs/re/RES_0464_BLIT_1951_CPU.md for the contract and evidence. */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_r0464_blit_cpu;

typedef enum {
    PR_R0464_BLIT_CPU_OK = 0,
    PR_R0464_BLIT_CPU_ARGUMENT,  /* host arguments or entry state rejected; nothing changed */
    PR_R0464_BLIT_CPU_CALLEE,    /* SYSTEM Move returned a non-RETURNED status; state as it left it */
    PR_R0464_BLIT_CPU_PORT,      /* the bus refused an OUT; stopped before it, IP at the OUT */
    PR_R0464_BLIT_CPU_TRANSFER,  /* Move returned somewhere other than the call's next IP */
    PR_R0464_BLIT_CPU_LIMIT,     /* max_loopbacks backward jumps taken; stopped before the next */
    PR_R0464_BLIT_CPU_CONTRACT   /* a Move source or destination meets A0000..AFFFF with an EGA attached; stopped at Move entry */
} pr_r0464_blit_cpu_status;

typedef struct {
    int move_calls;   /* completed or attempted SYSTEM Move calls */
    int move_status;  /* last Move leaf status, -1 when none */
} pr_r0464_blit_cpu_diagnostic;

pr_r0464_blit_cpu_status pr_r0464_blit_1951_cpu(pr_bus *bus, size_t ram_size, pr_r0464_blit_cpu *cpu,
                                                 size_t max_loopbacks, pr_r0464_blit_cpu_diagnostic *diagnostic);
#endif
'''

source = '''/* START.EXE resident 0464:1951 bitmap blit, raw CPU (docs/re/RES_0464_BLIT_1951_CPU.md).
 * One case per original instruction; operands are reread from the bus in source order. */
#include "poolrad/recovered/resident_0464_blit_1951_cpu.h"
#include "poolrad/recovered/system_string_move_leaves_cpu.h"
#include <string.h>
#define MASK (PR_BUS_RAM_SIZE - 1u)
#define STATUS(s) PR_R0464_BLIT_CPU_##s
typedef pr_r0464_blit_cpu_status pr_status_t;
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
static void push(pr_bus *b, pr_r0464_blit_cpu *c, uint16_t v) { c->sp = (uint16_t)(c->sp - 2u); wr16(b, c->ss, c->sp, v); }
static uint16_t pop(pr_bus *b, pr_r0464_blit_cpu *c) { uint16_t v = rd16(b, c->ss, c->sp); c->sp = (uint16_t)(c->sp + 2u); return v; }
static void szp(pr_r0464_blit_cpu *c, uint16_t r, unsigned bits)
{
    uint8_t p = (uint8_t)r;
    p ^= (uint8_t)(p >> 4u); p ^= (uint8_t)(p >> 2u); p ^= (uint8_t)(p >> 1u);
    if ((p & 1u) == 0u) c->flags |= 0x04u;
    if ((r & (bits == 8u ? 0xFFu : 0xFFFFu)) == 0u) c->flags |= 0x40u;
    if (r & (bits == 8u ? 0x80u : 0x8000u)) c->flags |= 0x80u;
}
static uint16_t arith(pr_r0464_blit_cpu *c, uint16_t l, uint16_t r, unsigned carry, unsigned bits, unsigned sub)
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
static uint16_t logic(pr_r0464_blit_cpu *c, uint16_t v, unsigned bits)
{
    v = (uint16_t)(v & (bits == 8u ? 0xFFu : 0xFFFFu)); c->flags &= (uint16_t)~0x08D5u; szp(c, v, bits); return v;
}
static uint16_t incdec(pr_r0464_blit_cpu *c, uint16_t v, unsigned bits, unsigned dec)
{
    uint16_t cf = c->flags & 1u; v = arith(c, v, 1u, 0u, bits, dec);
    c->flags = (uint16_t)((c->flags & ~1u) | cf); return v;
}
/* SHL/SHR, count masked to 5 bits; a zero count changes nothing. CF is the last bit out; OF is
 * MSB(v<<(n-1)) ^ MSB(result) for SHL and MSB(v>>(n-1)) ^ MSB(result) for SHR (architectural for n=1,
 * Unicorn's rule otherwise); AF := 0 (Unicorn). The report in the doc shows where these are observed. */
static uint16_t shift(pr_r0464_blit_cpu *c, uint16_t v, uint8_t n, unsigned bits, unsigned right)
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
/* MUL r/m16: DX:AX := AX * src; CF = OF = (DX != 0); SF/ZF/PF from AX (PF from its low byte); AF := 0. */
static void mul16(pr_r0464_blit_cpu *c, uint16_t src)
{
    uint32_t p = (uint32_t)c->ax * src;
    c->ax = (uint16_t)p; c->dx = (uint16_t)(p >> 16u);
    c->flags &= (uint16_t)~0x08D5u;
    if (c->dx) c->flags |= 0x801u;
    szp(c, c->ax, 16u);
}
static int meets_aperture(uint16_t seg, uint16_t off, uint16_t count)
{
    size_t lo = physical(seg, off), n;
    for (n = 0; n < count; ++n) {
        size_t at = (lo + n) & MASK;
        if (at >= 0xA0000u && at < 0xB0000u) return 1;
    }
    return 0;
}
/* SYSTEM Move 0AF8:1685 on the flat RAM (its accepted raw model). Entered with the far frame pushed. */
static int call_move(pr_bus *b, pr_r0464_blit_cpu *c, pr_r0464_blit_cpu_diagnostic *diag)
{
    pr_system_leaf_cpu inner; int status;
    _Static_assert(sizeof inner == sizeof *c, "CPU layout");
    if (b->ega != NULL) {
        uint16_t count = rd16(b, c->ss, (uint16_t)(c->sp + 4u));
        if (meets_aperture(rd16(b, c->ss, (uint16_t)(c->sp + 8u)), rd16(b, c->ss, (uint16_t)(c->sp + 6u)), count) ||
            meets_aperture(rd16(b, c->ss, (uint16_t)(c->sp + 12u)), rd16(b, c->ss, (uint16_t)(c->sp + 10u)), count))
            return STATUS(CONTRACT);
    }
    memcpy(&inner, c, sizeof inner);
    status = (int)pr_system_move_1685(b->ram, PR_BUS_RAM_SIZE, &inner);
    memcpy(c, &inner, sizeof inner);
    diag->move_calls += 1; diag->move_status = status;
    return status != 0 ? STATUS(CALLEE) : 0;
}

pr_r0464_blit_cpu_status pr_r0464_blit_1951_cpu(pr_bus *b, size_t ram_size, pr_r0464_blit_cpu *c,
                                                 size_t max_loopbacks, pr_r0464_blit_cpu_diagnostic *d)
{
    pr_r0464_blit_cpu_diagnostic fallback = {0, -1};
    uint16_t caller; size_t loops = 0;
    if (b == NULL || b->ram == NULL || c == NULL || ram_size != PR_BUS_RAM_SIZE || c->ip != 0x1951u)
        return STATUS(ARGUMENT);
    pr_r0464_blit_cpu_diagnostic *dg = d != NULL ? d : &fallback;
    *dg = fallback;
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
targets = {ROOT / 'include/poolrad/recovered/resident_0464_blit_1951_cpu.h': header,
           ROOT / 'src/recovered/resident_0464_blit_1951_cpu.c': source}
if '--check' in sys.argv:
    bad = [str(p) for p, text in targets.items() if p.read_text() != text]
    print('EMIT_CHECK', 'MISMATCH ' + ', '.join(bad) if bad else 'OK'); sys.exit(1 if bad else 0)
for p, text in targets.items():
    p.write_text(text)
print('cases', len(e.ins))
