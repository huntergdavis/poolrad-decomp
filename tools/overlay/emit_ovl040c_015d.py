#!/usr/bin/env python3
"""Regenerate src/recovered/overlay_u40c_tile_015d_cpu.c and its header with emit_raw_cpu_c.py.

python3 tools/overlay/emit_ovl040c_015d.py [--check]   (--check compares instead of writing)
Needs the owner START.EXE and GAME.OVR under resources/. Prints the undefined-flag observability report."""
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
from overlay_inventory import find_units, parse_mz
from emit_raw_cpu_c import Emitter

ROOT = Path(__file__).resolve().parents[2]
exe = (ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes()
ovr = (ROOT / 'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
unit = {u['stub_para']: u for u in find_units(exe[parse_mz(exe)['header_bytes']:], ovr)}[0x40C]
code = ovr[unit['ovr_offset']:unit['ovr_offset'] + unit['code_size']]
START, END = 0x015D, 0x02AD
CALLS = {0x00FF: 'call_mode_00ff', 0x0919: 'call_write_0919', 0x084A: 'call_writeln_084a', 0x0059: 'call_readkey_0059',
         0x0000: 'call_quit_0000', 0x08D4: 'call_compose_08d4', 0x1685: 'call_move_1685', 0x1951: 'call_blit_1951'}
e = Emitter(code[START:END], START, END, CALLS)
for a, mn, ops, und, vis in e.undefined_report():
    print(f'UNDEF {a:04X} {mn} {ops}: {und} -> {vis if vis else "never observed (overwritten on every path)"}')
cases = '\n'.join(e.cases())

header = '''#ifndef POOLRAD_RECOVERED_OVERLAY_U40C_TILE_015D_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U40C_TILE_015D_CPU_H
#include <stddef.h>
#include <stdint.h>
#include "poolrad/recovered/memory_bus.h"
#include "poolrad/recovered/resident_0464_blit_1951_cpu.h"
#include "poolrad/recovered/resident_0464_compose_08d4_cpu.h"
#include "poolrad/recovered/resident_0464_video_mode_cpu.h"
#include "poolrad/recovered/resident_0899_readkey_0059_cpu.h"
#include "poolrad/recovered/system_text_write_cpu.h"

/* Raw-CPU, full-state model of GAME.OVR unit 40Ch +[015D,02AD) (336 bytes, 127 instructions, RETF 0Ah): place tile
 * number [BP+0Ah] (1..11Eh) from its tile-set band. The number selects band 0..4 (1..2Dh, 2Eh..73h, 74h..B9h, BAh..FFh,
 * 100h..11Eh) into [BP-1]. 0 or 11Fh..7FFFh takes the error branch: restore the video mode (0464:00FF),
 * Write(Output, the CS:013A message), Writeln(Output), ReadKey (0899:0059) into [BP-2], then QUIT (0899:0000), which
 * never returns. 8000h..FFFFh matches no test and leaves [BP-1] as it was on the stack. Then, when the band's far
 * pointer DS:5E3A[band] is non-null, the number is rebased by the word DS:2722[band] and
 *   [BP+8] byte non-zero: 0464:08D4 (tile compositor) into the object DS:4A20 with item [BP+0Ah], flag [BP+6] and the
 *                         sign-extended bytes [BP+0Ch] and [BP+0Eh];
 *   [BP+8] byte zero:     SYSTEM Move(tile set + 17h + item * row size, DS:5E4E object + 17h, row size (+11h)), then
 *                         0464:1951 (blit) of the DS:5E4E object at the sign-extended bytes [BP+0Ch], [BP+0Eh].
 * Entered after a far call: [SP] IP, [SP+2] CS, then the five argument words; RETF 0Ah. CS is the unit's load segment;
 * the eight far-call segment words (CS:01C8, 01DA, 01DF, 01E4, 01EC, 024C, 028E, 02A5) are loader fixups, read live.
 * Each callee runs through its merged model on the same RAM. Operands are reread in source order; offsets wrap at 16
 * bits and physical addresses at 1 MiB. See docs/re/OVL_040C_015D_CPU.md for the contract and evidence. */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_ovl040c_015d_regs;

typedef struct {
    pr_r0464_video_env video;   /* 00FF: the INT 10h handler behind Dos.Intr */
    pr_system_text_env text;    /* Write/Writeln: Output's InOutFunc and FlushFunc */
    pr_r0899_key_env key;       /* ReadKey: INT 16h, the 08AE driver and QUIT; QUIT itself takes key.quit */
    size_t max_loopbacks;       /* passed to 08D4 and 1951 */
} pr_ovl040c_015d_env;

typedef enum {
    PR_OVL040C_015D_OK = 0,     /* RETF 0Ah executed */
    PR_OVL040C_015D_ARGUMENT,   /* null pointers, RAM not exactly 1 MiB, IP not 015Dh; nothing changed */
    PR_OVL040C_015D_CALLEE,     /* a callee stopped (diagnostic: site and its status); state as it left it */
    PR_OVL040C_015D_TRANSFER,   /* a callee returned somewhere other than the instruction after its call */
    PR_OVL040C_015D_CONTRACT,   /* EGA attached and a flat-RAM callee (error branch) or Move meeting A0000..AFFFF; stopped at its entry */
    PR_OVL040C_015D_EXIT        /* QUIT reached Halt, or ReadKey exited (its key 03 QUIT or the Crt Ctrl-Break path) */
} pr_ovl040c_015d_status;

typedef struct {
    uint16_t site;              /* the far-call site of the stopping (or exiting) callee, 0 when none */
    int status;                 /* that callee's own status, -1 when none */
    int service;                /* 00FF: Dos.Intr status; 0919/084A: the driver result; otherwise -1 */
    unsigned calls;             /* far calls entered */
    pr_r0899_key_diagnostic key;
    pr_r0899_quit_diagnostic quit;
    pr_r0464_compose_cpu_diagnostic compose;
    pr_r0464_blit_cpu_diagnostic blit;
} pr_ovl040c_015d_diagnostic;

pr_ovl040c_015d_status pr_ovl040c_015d_cpu(pr_bus *bus, size_t ram_size, pr_ovl040c_015d_regs *cpu,
                                           const pr_ovl040c_015d_env *env, pr_ovl040c_015d_diagnostic *diagnostic);
#endif
'''

source = '''/* GAME.OVR unit 40Ch +015D tile placement, raw CPU (docs/re/OVL_040C_015D_CPU.md).
 * One case per original instruction; operands are reread from the bus in source order. */
#include "poolrad/recovered/overlay_u40c_tile_015d_cpu.h"
#include "poolrad/recovered/system_string_move_leaves_cpu.h"
#include <string.h>
#define MASK (PR_BUS_RAM_SIZE - 1u)
#define STATUS(s) PR_OVL040C_015D_##s
typedef pr_ovl040c_015d_status pr_status_t;
typedef pr_ovl040c_015d_regs pr_cpu_t;
typedef struct {
    const pr_ovl040c_015d_env *env;
    pr_ovl040c_015d_diagnostic *d;
} pr_run;
_Static_assert(sizeof(pr_cpu_t) == sizeof(pr_r0464_video_cpu) && sizeof(pr_cpu_t) == sizeof(pr_system_text_cpu) &&
               sizeof(pr_cpu_t) == sizeof(pr_r0899_cpu) && sizeof(pr_cpu_t) == sizeof(pr_r0464_compose_cpu) &&
               sizeof(pr_cpu_t) == sizeof(pr_r0464_blit_cpu) && sizeof(pr_cpu_t) == sizeof(pr_system_leaf_cpu), "CPU layout");
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
/* MUL r/m16: DX:AX := AX * src; CF = OF = (DX != 0); SF/ZF/PF from AX (PF from its low byte); AF := 0. */
static void mul16(pr_cpu_t *c, uint16_t src)
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
/* Every hook is entered with the far frame pushed and CS:IP at the callee. A callee's non-success status becomes CALLEE
 * (or EXIT for QUIT's Halt and ReadKey's exits) with the CPU as the callee left it and the site recorded. */
static int stop(pr_run *r, uint16_t site, int status, int result)
{
    r->d->site = site; r->d->status = status; return result;
}
/* The error-branch callees are flat-RAM models: with an EGA attached they would bypass it, so the model stops first. */
static int flat_only(pr_bus *b, pr_run *r, uint16_t site)
{
    r->d->calls += 1u;
    return b->ega != NULL ? stop(r, site, -1, STATUS(CONTRACT)) : 0;
}
static int call_mode_00ff(pr_bus *b, pr_cpu_t *c, pr_run *r)
{
    pr_r0464_video_cpu in; int s, intr = -1;
    if (flat_only(b, r, 0x01C5u)) return STATUS(CONTRACT);
    memcpy(&in, c, sizeof in);
    s = (int)pr_r0464_restore_mode_00ff_cpu(b->ram, PR_BUS_RAM_SIZE, &in, &r->env->video, &intr);
    memcpy(c, &in, sizeof in); r->d->service = intr;
    return s != PR_R0464_VIDEO_OK ? stop(r, 0x01C5u, s, STATUS(CALLEE)) : 0;
}
static int call_write_0919(pr_bus *b, pr_cpu_t *c, pr_run *r)
{
    pr_system_text_cpu in; int s, svc = -1;
    if (flat_only(b, r, 0x01D7u)) return STATUS(CONTRACT);
    memcpy(&in, c, sizeof in);
    s = (int)pr_system_write_str_0919(b->ram, PR_BUS_RAM_SIZE, &in, &r->env->text, &svc);
    memcpy(c, &in, sizeof in); r->d->service = svc;
    return s != PR_SYSTEM_TEXT_RETURNED ? stop(r, 0x01D7u, s, STATUS(CALLEE)) : 0;
}
static int call_writeln_084a(pr_bus *b, pr_cpu_t *c, pr_run *r)
{
    pr_system_text_cpu in; int s, svc = -1;
    if (flat_only(b, r, 0x01DCu)) return STATUS(CONTRACT);
    memcpy(&in, c, sizeof in);
    s = (int)pr_system_writeln_084a(b->ram, PR_BUS_RAM_SIZE, &in, &r->env->text, &svc);
    memcpy(c, &in, sizeof in); r->d->service = svc;
    return s != PR_SYSTEM_TEXT_RETURNED ? stop(r, 0x01DCu, s, STATUS(CALLEE)) : 0;
}
static int call_readkey_0059(pr_bus *b, pr_cpu_t *c, pr_run *r)
{
    pr_r0899_cpu in; int s;
    if (flat_only(b, r, 0x01E1u)) return STATUS(CONTRACT);
    memcpy(&in, c, sizeof in);
    s = (int)pr_r0899_readkey_0059_cpu(b->ram, PR_BUS_RAM_SIZE, &in, &r->env->key, &r->d->key);
    memcpy(c, &in, sizeof in);
    if (s == PR_R0899_KEY_RETURN) return 0;
    return stop(r, 0x01E1u, s, s == PR_R0899_KEY_EXIT || s == PR_R0899_KEY_BREAK ? STATUS(EXIT) : STATUS(CALLEE));
}
static int call_quit_0000(pr_bus *b, pr_cpu_t *c, pr_run *r)
{
    pr_r0899_cpu in; int s;
    if (flat_only(b, r, 0x01E9u)) return STATUS(CONTRACT);
    memcpy(&in, c, sizeof in);
    s = (int)pr_r0899_quit_0000_cpu(b->ram, PR_BUS_RAM_SIZE, &in, &r->env->key.quit, &r->d->quit);
    memcpy(c, &in, sizeof in);
    return stop(r, 0x01E9u, s, s == PR_R0899_QUIT_EXIT ? STATUS(EXIT) : STATUS(CALLEE));
}
static int call_compose_08d4(pr_bus *b, pr_cpu_t *c, pr_run *r)
{
    pr_r0464_compose_cpu in; int s;
    r->d->calls += 1u;
    memcpy(&in, c, sizeof in);
    s = (int)pr_r0464_compose_08d4_cpu(b, PR_BUS_RAM_SIZE, &in, r->env->max_loopbacks, &r->d->compose);
    memcpy(c, &in, sizeof in);
    return s != PR_R0464_COMPOSE_CPU_OK ? stop(r, 0x0249u, s, STATUS(CALLEE)) : 0;
}
/* SYSTEM Move 0AF8:1685 on the flat RAM (its accepted raw model), with the 08D4/1951 aperture contract. */
static int call_move_1685(pr_bus *b, pr_cpu_t *c, pr_run *r)
{
    pr_system_leaf_cpu in; int s;
    r->d->calls += 1u;
    if (b->ega != NULL) {
        uint16_t count = rd16(b, c->ss, (uint16_t)(c->sp + 4u));
        if (meets_aperture(rd16(b, c->ss, (uint16_t)(c->sp + 8u)), rd16(b, c->ss, (uint16_t)(c->sp + 6u)), count) ||
            meets_aperture(rd16(b, c->ss, (uint16_t)(c->sp + 12u)), rd16(b, c->ss, (uint16_t)(c->sp + 10u)), count))
            return stop(r, 0x028Bu, -1, STATUS(CONTRACT));
    }
    memcpy(&in, c, sizeof in);
    s = (int)pr_system_move_1685(b->ram, PR_BUS_RAM_SIZE, &in);
    memcpy(c, &in, sizeof in);
    return s != 0 ? stop(r, 0x028Bu, s, STATUS(CALLEE)) : 0;
}
static int call_blit_1951(pr_bus *b, pr_cpu_t *c, pr_run *r)
{
    pr_r0464_blit_cpu in; int s;
    r->d->calls += 1u;
    memcpy(&in, c, sizeof in);
    s = (int)pr_r0464_blit_1951_cpu(b, PR_BUS_RAM_SIZE, &in, r->env->max_loopbacks, &r->d->blit);
    memcpy(c, &in, sizeof in);
    return s != PR_R0464_BLIT_CPU_OK ? stop(r, 0x02A2u, s, STATUS(CALLEE)) : 0;
}

pr_ovl040c_015d_status pr_ovl040c_015d_cpu(pr_bus *b, size_t ram_size, pr_ovl040c_015d_regs *c,
                                           const pr_ovl040c_015d_env *env, pr_ovl040c_015d_diagnostic *d)
{
    pr_ovl040c_015d_diagnostic fallback;
    pr_run run, *dg = &run;
    uint16_t caller;
    if (b == NULL || b->ram == NULL || c == NULL || env == NULL || ram_size != PR_BUS_RAM_SIZE || c->ip != 0x015Du)
        return STATUS(ARGUMENT);
    run.env = env; run.d = d != NULL ? d : &fallback;
    memset(run.d, 0, sizeof *run.d);
    run.d->status = -1; run.d->service = -1;
    run.d->compose.move_status = -1; run.d->blit.move_status = -1;
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
targets = {ROOT / 'include/poolrad/recovered/overlay_u40c_tile_015d_cpu.h': header,
           ROOT / 'src/recovered/overlay_u40c_tile_015d_cpu.c': source}
if '--check' in sys.argv:
    bad = [str(p) for p, text in targets.items() if p.read_text() != text]
    print('EMIT_CHECK', 'MISMATCH ' + ', '.join(bad) if bad else 'OK'); sys.exit(1 if bad else 0)
for p, text in targets.items():
    p.write_text(text)
print('cases', len(e.ins))
