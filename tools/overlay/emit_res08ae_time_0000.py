#!/usr/bin/env python3
"""Regenerate src/recovered/resident_08ae_time_0000_cpu.c and its header with emit_raw_cpu_c.py.

python3 tools/overlay/emit_res08ae_time_0000.py [--check]   (--check compares instead of writing)
Needs the owner START.EXE under resources/. Prints the undefined-flag observability report."""
import re
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
from resident_loader_map import unpack
from emit_raw_cpu_c import Emitter

ROOT = Path(__file__).resolve().parents[2]
img = bytes(unpack((ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes())[0])
START, END = 0x0000, 0x006C
e = Emitter(img[0x8AE0 + START:0x8AE0 + END], START, END, {0x00A2: 'call_gettime', 0x0A5B: 'call_longmul'})
for a, mn, ops, und, vis in e.undefined_report():
    print(f'UNDEF {a:04X} {mn} {ops}: {und} -> {vis if vis else "never observed (overwritten on every path)"}')
cases = '\n'.join(e.cases())

header = '''#ifndef POOLRAD_RECOVERED_RESIDENT_08AE_TIME_0000_CPU_H
#define POOLRAD_RECOVERED_RESIDENT_08AE_TIME_0000_CPU_H
#include <stddef.h>
#include <stdint.h>
#include "poolrad/recovered/memory_bus.h"
#include "poolrad/recovered/dos_gettime_00a2_cpu.h"

/* Raw-CPU, full-state model of START.EXE resident 08AE:[0000,006C) (108 bytes,
 * 48 instructions, RETF), a DRIVER_INTERNAL member candidate (never a credit,
 * docs/re/DRIVER_INTERNAL_CLASS.md): the time since midnight in hundredths of a
 * second. It calls Dos.GetTime 0A32:00A2 with four SS locals (hour [BP-6],
 * minute [BP-8], second [BP-0Ah], sec100 [BP-0Ch]), then forms DX:AX :=
 * LongMul(hour, 360000) + minute*6000 + second*100 + sec100 (the SYSTEM LongInt
 * multiply 0AF8:0A5B; 16-bit MULs for the other terms, each zero-extended and
 * added with carry into DX), stores it in [BP-4]/[BP-2] and returns it.
 * GetTime's INT 21h/2Ch is the caller-supplied declared service (passed through).
 * See docs/re/RES_08AE_TIME_0000_CPU.md for the contract and evidence. */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_r08ae_time_cpu;

typedef enum {
    PR_R08AE_TIME_CPU_OK = 0,
    PR_R08AE_TIME_CPU_ARGUMENT,  /* host arguments or entry state rejected (incl. an attached EGA); nothing changed */
    PR_R08AE_TIME_CPU_CALLEE,    /* GetTime or LongMul did not return; the diagnostic names which and its status */
    PR_R08AE_TIME_CPU_PORT,      /* unused (no OUT); shared numbering */
    PR_R08AE_TIME_CPU_TRANSFER,  /* a callee returned somewhere other than the call's next IP */
    PR_R08AE_TIME_CPU_LIMIT      /* unused (no backward jump); shared numbering */
} pr_r08ae_time_cpu_status;

typedef struct {
    int gettime_status;  /* last pr_dos_gettime_status, -1 when not called */
    int longmul_status;  /* last pr_system_longmul_status, -1 when not called */
} pr_r08ae_time_cpu_diagnostic;

pr_r08ae_time_cpu_status pr_r08ae_time_0000_cpu(pr_bus *bus, size_t ram_size, pr_r08ae_time_cpu *cpu,
                                                pr_dos_gettime_service service, void *service_context,
                                                pr_r08ae_time_cpu_diagnostic *diagnostic);
#endif
'''

source = '''/* START.EXE resident 08AE:0000 time in hundredths, raw CPU (docs/re/RES_08AE_TIME_0000_CPU.md).
 * One case per original instruction; operands are reread from the bus in source order. */
#include "poolrad/recovered/resident_08ae_time_0000_cpu.h"
#include "poolrad/recovered/system_longmul_0a5b_cpu.h"
#include <string.h>
#define MASK (PR_BUS_RAM_SIZE - 1u)
#define STATUS(s) PR_R08AE_TIME_CPU_##s
typedef pr_r08ae_time_cpu_status pr_status_t;
typedef pr_r08ae_time_cpu pr_cpu_t;
typedef struct { pr_r08ae_time_cpu_diagnostic *d; pr_dos_gettime_service service; void *context; } ctx_t;
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
/* Dos.GetTime 0A32:00A2 (runtime coverage), entered with the far frame pushed; the INT 21h service is passed through. */
static int call_gettime(pr_bus *b, pr_cpu_t *c, ctx_t *dg)
{
    pr_dos_gettime_cpu inner; int status;
    _Static_assert(sizeof inner == sizeof *c, "CPU layout");
    memcpy(&inner, c, sizeof inner);
    status = (int)pr_dos_gettime_00a2(b->ram, PR_BUS_RAM_SIZE, &inner, dg->service, dg->context);
    memcpy(c, &inner, sizeof inner);
    dg->d->gettime_status = status;
    return status != 0 ? STATUS(CALLEE) : 0;
}
/* SYSTEM LongMul 0AF8:0A5B on the flat RAM (its accepted raw model), entered with the far frame pushed. */
static int call_longmul(pr_bus *b, pr_cpu_t *c, ctx_t *dg)
{
    pr_system_longmul_cpu inner; int status;
    _Static_assert(sizeof inner == sizeof *c, "CPU layout");
    memcpy(&inner, c, sizeof inner);
    status = (int)pr_system_longmul_0a5b(b->ram, PR_BUS_RAM_SIZE, &inner);
    memcpy(c, &inner, sizeof inner);
    dg->d->longmul_status = status;
    return status != 0 ? STATUS(CALLEE) : 0;
}
pr_r08ae_time_cpu_status pr_r08ae_time_0000_cpu(pr_bus *b, size_t ram_size, pr_r08ae_time_cpu *c,
                                                pr_dos_gettime_service service, void *service_context,
                                                pr_r08ae_time_cpu_diagnostic *d)
{
    pr_r08ae_time_cpu_diagnostic fallback = {-1, -1};
    ctx_t context; ctx_t *dg = &context;
    uint16_t caller; size_t loops = 0, max_loopbacks = 0;
    if (b == NULL || b->ram == NULL || b->ega != NULL || c == NULL || service == NULL || ram_size != PR_BUS_RAM_SIZE ||
        c->ip != 0x0000u)
        return STATUS(ARGUMENT);
    context.d = d != NULL ? d : &fallback; context.service = service; context.context = service_context;
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
for name in ('rd8', 'wr8'):  # one-line helpers: remove exactly their own line when the cases never call them
    if name + '(b,' not in cases:
        source = re.sub(r'(?m)^static [^\n]*\b' + name + r'\([^\n]*\n', '', source, count=1)
targets = {ROOT / 'include/poolrad/recovered/resident_08ae_time_0000_cpu.h': header,
           ROOT / 'src/recovered/resident_08ae_time_0000_cpu.c': source}
if '--check' in sys.argv:
    bad = [str(p) for p, text in targets.items() if p.read_text() != text]
    print('EMIT_CHECK', 'MISMATCH ' + ', '.join(bad) if bad else 'OK'); sys.exit(1 if bad else 0)
for p, text in targets.items():
    p.write_text(text)
print('cases', len(e.ins))
