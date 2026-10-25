#!/usr/bin/env python3
"""Deliberate mutations of the OVL_0038_0CC3 model, each run on a fixed case subset of the differential (and the C test for survivors)."""
# NOT_A_ROW_CHECKER: mutation helper imported by ovl_0038_0cc3_cpu_audit.py --mutate; the audit registers the row
import subprocess, sys, tempfile, time
from pathlib import Path
import ovl_0038_0cc3_cpu_audit as A
from res0709_printer_0626_cpu_audit import ROOT, check

MODEL = ROOT / 'src/recovered/overlay_u38_0cc3_cpu.c'
CASES = ('empty-list', 'no-key', 'key-wrong-type', 'key-high-word-differs', 'rec3-low-f', 'kind-f', 'unlink-head', 'unlink-middle', 'unlink-tail',
         'live-1010', 'table-count-0', 'table-count-2', 'table-count-5', 'table-zero-dword-on-cell', 'table-low-word-only', 'table-high-word-only', 'stop-after-shr', 'stop-after-cbw', 'stop-after-mul', 'grid-in-aperture', 'coords--1--1',
         'coords--128--128', 'dxy-signed', 'steps-high', 'mode1', 'runerror-offset-4', 'freemem-offset-8', 'cycle-no-key', 'grid-on-node+04',
         'grid-on-frame+04', 'flags-0a87', 'sp-0200') + tuple(f'budget-{b}' for b in (0, 3, 30, 60, 120, 201, 240))
MUTANTS = (
    ('type nibble shifted by 3', [('    STEP(); c->cx = (uint16_t)((c->cx & 0xFF00u) | ((uint16_t)(uint8_t)(0x04u) << 0u)); c->ip = 0x0CD4u;',
                                   '    STEP(); c->cx = (uint16_t)((c->cx & 0xFF00u) | ((uint16_t)(uint8_t)(0x03u) << 0u)); c->ip = 0x0CD4u;')]),
    ('key high word not compared', [('if (!(c->flags & 0x0040u)) { c->ip = 0x0D1Eu; goto L_0D1E; } c->ip = 0x0D07u;', 'c->ip = 0x0D07u;')]),
    ('the node type read at +13h', [('(uint16_t)(c->di + 0x0012u))) << 0u)); c->ip = 0x0D13u;', '(uint16_t)(c->di + 0x0013u))) << 0u)); c->ip = 0x0D13u;')]),
    ('the next link read at +2', [('STEP(); c->ax = brd16(bus, c->es, (uint16_t)(c->di + 0x0004u)); c->ip = 0x0D25u;',
                                   'STEP(); c->ax = brd16(bus, c->es, (uint16_t)(c->di + 0x0002u)); c->ip = 0x0D25u;')]),
    ('the message at 0CA9h', [('0x0CA8u', '0x0CA9u')]),
    ('the live flags at +0Ah+i', [('STEP(); (void)arith(c, brd8(bus, c->es, (uint16_t)(c->di + 0x000Bu)), 0x00u, 1, 0, 8u); c->ip = 0x0D66u;',
                                   'STEP(); (void)arith(c, brd8(bus, c->es, (uint16_t)(c->di + 0x000Au)), 0x00u, 1, 0, 8u); c->ip = 0x0D66u;')]),
    ('dx table zero-extended', [('STEP(); c->ax = (uint16_t)(int16_t)(int8_t)(uint8_t)c->ax; c->ip = 0x0D7Fu;', 'STEP(); c->ax = (uint16_t)(uint8_t)c->ax; c->ip = 0x0D7Fu;')]),
    ('the overlap table stride 8', [('    STEP(); c->dx = 0x0007u; c->ip = 0x0DD2u;', '    STEP(); c->dx = 0x0008u; c->ip = 0x0DD2u;')]),
    ('the overlap test ignores the high word', [('    STEP(); c->ax = (uint16_t)(c->ax | rd16(ram, c->ds, (uint16_t)(c->di + 0x66D8u))); logic(c, c->ax, 16u); c->ip = 0x0DDEu;',
                                                 '    STEP(); logic(c, c->ax, 16u); c->ip = 0x0DDEu;')]),
    ('the loop bound JA as JAE', [('if (!(c->flags & 0x0001u) && !(c->flags & 0x0040u)) { c->ip = 0x0E24u; goto L_0E24; }',
                                   'if (!(c->flags & 0x0001u)) { c->ip = 0x0E24u; goto L_0E24; }')]),
    ('the overlap mark 1Eh', [('bwr8(bus, c->es, (uint16_t)(c->di + 0x0007u), 0x1Fu)', 'bwr8(bus, c->es, (uint16_t)(c->di + 0x0007u), 0x1Eu)')]),
    ('the map row width 51', [('    STEP(); c->dx = 0x0032u; c->ip = 0x0E39u;', '    STEP(); c->dx = 0x0033u; c->ip = 0x0E39u;')]),
    ('the saved byte from +8+i', [('STEP(); c->bx = (uint16_t)((c->bx & 0xFF00u) | ((uint16_t)(uint8_t)(brd8(bus, c->es, (uint16_t)(c->di + 0x0007u))) << 0u)); c->ip = 0x0E58u;',
                                   'STEP(); c->bx = (uint16_t)((c->bx & 0xFF00u) | ((uint16_t)(uint8_t)(brd8(bus, c->es, (uint16_t)(c->di + 0x0008u))) << 0u)); c->ip = 0x0E58u;')]),
    ('three cells, not four', [('L_0E75: STEP(); (void)arith(c, rd8(ram, c->ss, (uint16_t)(c->bp + (-11))), 0x04u, 1, 0, 8u);',
                                 'L_0E75: STEP(); (void)arith(c, rd8(ram, c->ss, (uint16_t)(c->bp + (-11))), 0x03u, 1, 0, 8u);')]),
    ('the head unlink skips the head store', [('    STEP(); wr16(ram, c->ds, 0x6DBBu, c->ax); c->ip = 0x0EA9u;', '    STEP(); c->ip = 0x0EA9u;')]),
    ('FreeMem size 12h', [('    STEP(); c->ax = 0x0013u; c->ip = 0x0EF6u;', '    STEP(); c->ax = 0x0012u; c->ip = 0x0EF6u;')]),
    ('the re-stamp 1Fh', [('bwr8(bus, c->es, (uint16_t)(c->di + 0x0007u), 0x1Eu)', 'bwr8(bus, c->es, (uint16_t)(c->di + 0x0007u), 0x1Fu)')]),
    ('RETF 8', [('c->sp = (uint16_t)(c->sp + 10u);', 'c->sp = (uint16_t)(c->sp + 8u);')]),
    ('the return-address (TRANSFER) check dropped', [('    if (c->ip != ret_ip || c->cs != ret_cs) return PR_U38_0CC3_TRANSFER; \\', '    (void)ret_ip; (void)ret_cs; \\')]),
    ('SHR leaves AF clear', [('f = (uint16_t)(szp(r, 16u) | 0x0010u);', 'f = szp(r, 16u);')]),
    ('MUL clears SF', [('f = (uint16_t)(c->flags & 0x0090u);', 'f = (uint16_t)(c->flags & 0x0010u);')]),
    ('FreeMem RunError read as CALLEE', [('if (s == PR_SYSTEM_FREEMEM_RUNERROR) { d->callee_status = (int)s; return PR_U38_0CC3_RUNERROR; }', '(void)0;')]),
    ('the map write bypasses the bus', [('static void bwr8(pr_bus *bus, uint16_t seg, uint16_t off, uint8_t v) { pr_bus_write8(bus, physical(seg, off), v); }',
                                         'static void bwr8(pr_bus *bus, uint16_t seg, uint16_t off, uint8_t v) { bus->ram[physical(seg, off)] = v; }')]),
    ('the 1693 call is not charged to the budget', [('--*budget; d->callee = 2;', 'd->callee = 2;')]),
)


def c_test_verdict(tmp, path):
    exe = Path(tmp) / 'ctest'
    srcs = [str(path if s == 'overlay_u38_0cc3_cpu' else ROOT / f'src/recovered/{s}.c') for s in A.SOURCES if (ROOT / f'src/recovered/{s}.c').exists()]
    try:
        subprocess.run(['cc', '-std=c11', '-O1', '-I', str(ROOT / 'include'), str(ROOT / 'tests/recovered/test_overlay_u38_0cc3_cpu.c')] + srcs + ['-o', str(exe)],
                       check=True, capture_output=True)
    except subprocess.CalledProcessError:
        return ' (the C test did not build with it)'
    return ': CAUGHT BY THE C TEST' if subprocess.run([str(exe)], capture_output=True).returncode != 0 else ': the C test also passes'


def mutation_check(bodies, decoded):
    src = MODEL.read_text(); out = []
    mutants = [(n, e[0]) for n, e in MUTANTS]
    for k, (name, (old, new)) in enumerate(mutants):
        t0 = time.time(); print(f'[mutant {k + 1}/{len(mutants)}] START {name}', file=sys.stderr, flush=True)
        check(src.count(old) >= 1, f'mutant text occurs: {name}')
        text = src.replace(old, new) if src.count(old) > 1 and name in ('the message at 0CA9h',) else src.replace(old, new, 1)
        with tempfile.TemporaryDirectory(prefix='u38-0cc3-mut-') as tmp:
            path = Path(tmp) / 'mutant.c'; path.write_text(text)
            try:
                lib = A.library(tmp, override={'overlay_u38_0cc3_cpu': path})
            except subprocess.CalledProcessError:
                out.append((name, 'COMPILE-ERROR')); continue
            try:
                A.differential(bodies, decoded, lib, quick=True, only=CASES)
                out.append((name, 'SURVIVED the differential' + c_test_verdict(tmp, path)))
            except (ValueError, AssertionError) as e:
                out.append((name, 'CAUGHT: ' + str(e)[:100]))
        print(f'[mutant {k + 1}/{len(mutants)}] {out[-1][1][:80]} ({time.time() - t0:.0f}s)', file=sys.stderr, flush=True)
    d = sum(r.startswith('CAUGHT:') for _, r in out); c = sum(r.endswith('CAUGHT BY THE C TEST') for _, r in out)
    return dict(total=len(out), caught=d + c, caught_by_differential=d, caught_by_c_test=c, results=out, cases=list(CASES),
                survivors=[n for n, r in out if (r.startswith('SURVIVED') and not r.endswith('CAUGHT BY THE C TEST')) or r == 'COMPILE-ERROR'])
