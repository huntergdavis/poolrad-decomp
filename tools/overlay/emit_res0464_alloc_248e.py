#!/usr/bin/env python3
"""Regenerate src/recovered/resident_0464_alloc_248e_cpu.c and its header with emit_raw_cpu_c.py.

python3 tools/overlay/emit_res0464_alloc_248e.py [--check]   (--check compares instead of writing)
Needs the owner START.EXE under resources/. Prints the undefined-flag observability report."""
import re
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
from resident_loader_map import unpack
from emit_raw_cpu_c import Emitter

ROOT = Path(__file__).resolve().parents[2]
img = bytes(unpack((ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes())[0])
START, END = 0x248E, 0x2595
e = Emitter(img[0x4640 + START:0x4640 + END], START, END, {0x023F: 'call_getmem', 0x16A8: 'call_fill'})
for a, mn, ops, und, vis in e.undefined_report():
    print(f'UNDEF {a:04X} {mn} {ops}: {und} -> {vis if vis else "never observed (overwritten on every path)"}')
cases = '\n'.join(e.cases())

header = '''#ifndef POOLRAD_RECOVERED_RESIDENT_0464_ALLOC_248E_CPU_H
#define POOLRAD_RECOVERED_RESIDENT_0464_ALLOC_248E_CPU_H
#include <stddef.h>
#include <stdint.h>
#include "poolrad/recovered/memory_bus.h"

/* Raw-CPU, full-state model of START.EXE resident 0464:[248E,2595) (263 bytes,
 * 95 instructions, RETF 0Ch): the bitmap allocator paired with the release at
 * 0464:2595. Entered after a far call: [SP] IP, [SP+2] CS, [SP+4] the far
 * address of a pointer variable P, [SP+8] a flag byte (bit 0 asks for the
 * plane buffer), [SP+0Ah] a byte multiplier, [SP+0Ch] and [SP+0Eh] the two
 * size words stored as the header's +0 and +2. It GetMems a 17h-byte-headed
 * block of the computed size, clears it with FillChar, fills the header, and,
 * when bit 0 is set, GetMems and clears a second buffer stored at header +13h
 * (a quarter of the size in video mode 1). SYSTEM GetMem 0AF8:023F and
 * FillChar 0AF8:16A8 run as their accepted raw models.
 * See docs/re/RES_0464_ALLOC_248E_CPU.md for the contract and evidence. */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_r0464_alloc_cpu;

typedef enum {
    PR_R0464_ALLOC_CPU_OK = 0,
    PR_R0464_ALLOC_CPU_ARGUMENT,  /* host arguments or entry state rejected; nothing changed */
    PR_R0464_ALLOC_CPU_CALLEE,    /* GetMem or FillChar did not return normally; diagnostic names which and its status */
    PR_R0464_ALLOC_CPU_PORT,      /* unused by this body (no OUT); kept for the shared status numbering */
    PR_R0464_ALLOC_CPU_TRANSFER,  /* a callee returned somewhere other than the call's next IP */
    PR_R0464_ALLOC_CPU_LIMIT      /* max_loopbacks backward jumps taken (this body has none; always 0 taken) */
} pr_r0464_alloc_cpu_status;

typedef struct {
    int getmem_calls;   /* GetMem calls entered */
    int getmem_status;  /* last pr_system_getmem_status, -1 when none */
    int fill_calls;     /* FillChar calls entered */
    int fill_status;    /* last FillChar leaf status, -1 when none */
} pr_r0464_alloc_cpu_diagnostic;

/* getmem_steps caps GetMem's free-list walk (its LIMIT stop). */
pr_r0464_alloc_cpu_status pr_r0464_alloc_248e_cpu(pr_bus *bus, size_t ram_size, pr_r0464_alloc_cpu *cpu,
                                                   size_t max_loopbacks, unsigned getmem_steps,
                                                   pr_r0464_alloc_cpu_diagnostic *diagnostic);
#endif
'''

source = '''/* START.EXE resident 0464:248E bitmap allocator, raw CPU (docs/re/RES_0464_ALLOC_248E_CPU.md).
 * One case per original instruction; operands are reread from the bus in source order. */
#include "poolrad/recovered/resident_0464_alloc_248e_cpu.h"
#include "poolrad/recovered/system_getmem_023f_cpu.h"
#include "poolrad/recovered/system_fillchar_bus_16a8_cpu.h"
#include <string.h>
#define MASK (PR_BUS_RAM_SIZE - 1u)
#define STATUS(s) PR_R0464_ALLOC_CPU_##s
typedef pr_r0464_alloc_cpu_status pr_status_t;
typedef pr_r0464_alloc_cpu pr_cpu_t;
typedef struct { pr_r0464_alloc_cpu_diagnostic *d; unsigned getmem_steps; } ctx_t;
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
 * MSB(v<<(n-1)) ^ MSB(result) for SHL and MSB(v>>(n-1)) ^ MSB(result) for SHR (architectural for n=1).
 * AF is architecturally undefined; it follows pinned DOSBox-X normal/386 (AF := 1 for a nonzero count),
 * because in this body SHR +2555's AF survives FillChar to the RETURN. Unicorn sets AF := 0: a declared split. */
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
    c->flags |= 0x10u;
    szp(c, (uint16_t)res, bits); return (uint16_t)res;
}
/* MUL r/m16: DX:AX := AX * src; CF = OF = (DX != 0); SF/ZF/PF from AX (PF from its low byte); AF := 0. */
static void mul16(pr_cpu_t *c, uint16_t src)
{
    uint32_t p = (uint32_t)c->ax * src;
    c->ax = (uint16_t)p; c->dx = (uint16_t)(p >> 16u);
    c->flags &= (uint16_t)~0x08D5u;
    if (c->dx) c->flags |= 0x801u;
    szp(c, c->ax, 16u);
}
/* SYSTEM GetMem 0AF8:023F on the flat RAM (its accepted raw model), entered with the far frame pushed. */
static int call_getmem(pr_bus *b, pr_cpu_t *c, ctx_t *dg)
{
    pr_system_getmem_cpu inner; int status;
    _Static_assert(sizeof inner == sizeof *c, "CPU layout");
    memcpy(&inner, c, sizeof inner);
    status = (int)pr_system_getmem_023f(b->ram, PR_BUS_RAM_SIZE, &inner, dg->getmem_steps);
    memcpy(c, &inner, sizeof inner);
    dg->d->getmem_calls += 1; dg->d->getmem_status = status;
    return status != 0 ? STATUS(CALLEE) : 0;
}
/* SYSTEM FillChar 0AF8:16A8 on the bus (its accepted raw model), entered with the far frame pushed. */
static int call_fill(pr_bus *b, pr_cpu_t *c, ctx_t *dg)
{
    pr_system_leaf_cpu inner; int status;
    _Static_assert(sizeof inner == sizeof *c, "CPU layout");
    memcpy(&inner, c, sizeof inner);
    status = (int)pr_system_fillchar_bus_16a8(b, &inner);
    memcpy(c, &inner, sizeof inner);
    dg->d->fill_calls += 1; dg->d->fill_status = status;
    return status != 0 ? STATUS(CALLEE) : 0;
}
static int ordinary_segment(uint16_t s)
{
    size_t start = (size_t)s << 4u;
    return start > 0xAFFFFu || start + 0xFFFFu < 0xA0000u;
}

pr_r0464_alloc_cpu_status pr_r0464_alloc_248e_cpu(pr_bus *b, size_t ram_size, pr_r0464_alloc_cpu *c,
                                                   size_t max_loopbacks, unsigned getmem_steps,
                                                   pr_r0464_alloc_cpu_diagnostic *d)
{
    pr_r0464_alloc_cpu_diagnostic fallback = {0, -1, 0, -1};
    ctx_t context; ctx_t *dg = &context;
    uint16_t caller; size_t loops = 0;
    if (b == NULL || b->ram == NULL || c == NULL || ram_size != PR_BUS_RAM_SIZE || c->ip != 0x248Eu ||
        (b->ega != NULL && !ordinary_segment(c->ds)))
        return STATUS(ARGUMENT);
    context.d = d != NULL ? d : &fallback; context.getmem_steps = getmem_steps;
    *context.d = fallback;
    caller = c->cs;
    (void)loops; (void)max_loopbacks;
    for (;;) {
        switch (c->ip) {
CASES
        default: return STATUS(TRANSFER);
        }
    }
}
'''.replace('CASES', cases)
for name in ('mul16', 'shift', 'incdec', 'logic'):
    if name + '(c,' not in cases:
        source = re.sub(r'(/\*[^/]*?\*/\n)?static [a-z0-9_ ]+\*?' + name + r'\(.*?\n}\n', '', source, count=1, flags=re.S)
targets = {ROOT / 'include/poolrad/recovered/resident_0464_alloc_248e_cpu.h': header,
           ROOT / 'src/recovered/resident_0464_alloc_248e_cpu.c': source}
if '--check' in sys.argv:
    bad = [str(p) for p, text in targets.items() if p.read_text() != text]
    print('EMIT_CHECK', 'MISMATCH ' + ', '.join(bad) if bad else 'OK'); sys.exit(1 if bad else 0)
for p, text in targets.items():
    p.write_text(text)
print('cases', len(e.ins))
